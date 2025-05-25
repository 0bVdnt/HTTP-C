#include "../include/server.h"
#include "../include/config.h"        // For PORT, BACKLOG, BUFFER_SIZE
#include "../include/http_parser.h"   // For parseRequest
#include "../include/http_response.h" // For echoResponse
#include "../include/http_types.h" // For freeClientRequest, freeServerResponse
#include "../include/signal_handler.h" // For keep_running

#include <errno.h>      // For errno
#include <netinet/in.h> // For sockaddr_in, INADDR_ANY, htons, htonl
#include <netinet/ip.h> // For IPPROTO_TCP (though not directly used here)
#include <stdio.h>      // For fprintf, printf, perror
#include <stdlib.h>     // For EXIT_FAILURE, EXIT_SUCCESS, malloc
#include <string.h>     // For strerror, strlen, strcmp, memcmp
#include <sys/socket.h> // For socket, setsockopt, bind, listen, accept, send
#include <sys/types.h>  // For ssize_t
#include <unistd.h>     // For read, close

int setup_server_socket(int port) {
  // stores struct file descriptor
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  // server_fd is -1 in case of an error, otherwise store the descriptor of the
  // socket
  if (server_fd == -1) {
    fprintf(stderr, "Socket creation failed: %s\n", strerror(errno));
    return -1;
  }

  // the SO_REUSEADDR to 1 using reuse param, so that their is not waiting for
  // reusing the same address again in case of an error or updation.
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    // In case of an error setsockopt(/*...*/) return -1
    fprintf(stderr, "SO_REUSEADDR failed: %s\n", strerror(errno));
    close(server_fd);
    return -1;
  }
  // a struct to store values of parameters for the server
  struct sockaddr_in serv_addr = {
      // sockaddr_in is IPv4 specifically
      // This kind of initialization of a struct sets the values not initilized
      // here as NULL or 0
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {htonl(INADDR_ANY)},
  };

  // The bind(/*...*/) system call assigns local protocol addresses (IP Addr) to
  // a socket. Tells the os that the socket server_fd should listen on the addr.
  // and port in serv_addr
  // bind(/*...*/) returns 0 on success and -1 on failure
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    fprintf(stderr, "Bind failed: %s\n", strerror(errno));
    close(server_fd);
    return -1;
  }

  // BACKLOG determines the maximum number of pending connections
  // that can be queued for a server
  // listen(/*...*/) syscall makes the socket with the given server_fd passive,
  // i.e. It tells the OS that the socket is no longer making outgoing
  // connections, it should now queue up incoming connection requests which will
  // be accepted by the accept syscall
  if (listen(server_fd, BACKLOG) != 0) {
    fprintf(stderr, "Listen failed: %s\n", strerror(errno));
    close(server_fd);
    return -1;
  }
  return server_fd;
}

int handle_accept(int server_fd) {
  // structs to store the client address data
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  // The accept(/*...*/) syscall extracts the first connection request in the
  // queue of pending connections, it creates a new connected socket and
  // returns the file descriptor for the connected socket. accept(/*...*/)
  // will pause the execution here until a client attempts to connect (unless
  // the server is in non blocking mode which is not the case here)

  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

  if (!keep_running) {
    fprintf(stderr, "Server is shutting down. Exiting accept loop.\n");
    // Important: if accept is interrupted, it might return -1 and errno=EINTR.
    // We return a special value or let the caller check keep_running.
    // For now, -1 is fine, main.c will check keep_running after.
    return -1; // Or some specific error code for shutdown
  }

  if (client_fd < 0) {
    // If accept was interrupted by a signal, errno might be EINTR.
    // EINTR should not be treated as a fatal error, but continue the loop to
    // check keep_running.
    if (errno == EINTR) {
      fprintf(stderr,
              "Accept interrupted by signal. Checking shutdown flag.\n");
      return -2; // Custom code for EINTR, indicating non-fatal continue
    }
    fprintf(stderr, "Connection failed: %s\n", strerror(errno));
    return -3; // Generic error
  }
  return client_fd;
}

void handle_client(int client_fd, char *serverResponse[]) {
  char client_req_buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_read = read(client_fd, client_req_buffer, BUFFER_SIZE - 1);

  if (bytes_read < 0) {
    fprintf(stderr, "Failed to read from client: %s\n", strerror(errno));
    return;
  }
  if (bytes_read == 0) {
    printf("Client closed connection before sending data.\n");
    return;
  }
  client_req_buffer[bytes_read] = '\0';
  // Null terminate exactly after read bytes

  ClientRequest C = parseRequest(client_req_buffer, bytes_read);

  if (!C.http_method || !C.route || !C.http_version) {
    fprintf(stderr, "Failed to parse request or malformed request.\n");
    // Send a 400 Bad Request
    if (send(client_fd, "HTTP/1.1 400 Bad Request\r\n\r\n",
             strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) < 0) {
      fprintf(stderr, "Failed Sending 400 Response: %s\n", strerror(errno));
    }
    freeClientRequest(&C);
    return;
  }

  if (strcmp(C.http_method, "GET") == 0) {
    if (C.args != NULL && memcmp(C.route, "/echo/", strlen("/echo/")) == 0) {
      ServerResponse S = echoResponse(C, serverResponse);
      if (!S.status_line || !S.headers || !S.response_body) {
        fprintf(stderr, "Failed to create echo response.\n");
        if (send(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n",
                 strlen("HTTP/1.1 500 Internal Server Error\r\n\r\n"), 0) < 0) {
          fprintf(stderr, "Failed Sending 500 Response: %s\n", strerror(errno));
        }
        freeServerResponse(&S);
        freeClientRequest(&C);
        return;
      }

      // Allocate buffer for the full response string
      // Estimate size: status + headers + body + some slack
      size_t estimated_size = strlen(S.status_line) + strlen(S.headers) +
                              strlen(S.response_body) + 10;
      char *responseString =
          (char *)malloc(estimated_size); // Dynamic size estimation
      if (!responseString) {
        fprintf(stderr, "Failed to allocate memory for responseString.\n");
        // Send a 500 Internal Server Error
        if (send(client_fd, "HTTP/1.1 500 Internal Server Error\r\n\r\n",
                 strlen("HTTP/1.1 500 Internal Server Error\r\n\r\n"), 0) < 0) {
          fprintf(stderr, "Failed Sending 500 Response: %s\n", strerror(errno));
        }
        freeServerResponse(&S);
        freeClientRequest(&C);
        return;
      }

      int written = snprintf(responseString, estimated_size, "%s%s%s",
                             S.status_line, S.headers, S.response_body);

      if (written < 0 ||
          (size_t)written >= estimated_size) { // cast written to size_t
        fprintf(stderr, "Response string truncated or error occurred while "
                        "parsing Response string.\n");
      }

      if (send(client_fd, responseString, strlen(responseString), 0) < 0) {
        fprintf(stderr, "Failed Sending Response: %s\n", strerror(errno));
      } else {
        printf("Echo Response Sent.\n");
      }
      free(responseString);
      freeServerResponse(&S);

    } else if (strcmp(C.route, "/") == 0) {
      if (send(client_fd, serverResponse[200], strlen(serverResponse[200]), 0) <
          0) {
        fprintf(stderr, "Failed Sending 200 Response: %s\n", strerror(errno));
      } else {
        printf("200 OK Response Sent (root).\n");
      }
    } else {
      if (send(client_fd, serverResponse[404], strlen(serverResponse[404]), 0) <
          0) {
        fprintf(stderr, "Failed Sending 404 Response: %s\n", strerror(errno));
      } else {
        printf("404 Not Found Response Sent.\n");
      }
    }
  } else { // Handle non-GET methods (e.g., send 405 Method Not Allowed)
    const char *method_not_allowed =
        "HTTP/1.1 405 Method Not Allowed\r\nAllow: GET\r\n\r\n";
    if (send(client_fd, method_not_allowed, strlen(method_not_allowed), 0) <
        0) {
      fprintf(stderr, "Failed Sending 405 Response: %s\n", strerror(errno));
    } else {
      printf("405 Method Not Allowed sent.\n");
    }
  }
  freeClientRequest(&C);
}
