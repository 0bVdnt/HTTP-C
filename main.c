#define _POSIX_C_SOURCE 200809L
// For POSIX.1-2008 functions like sigaction

#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RESPONSE_CODES 600
#define PORT 42069
#define BACKLOG 5
#define BUFFER_SIZE 256
#define ARGS_BUFFER_SIZE 256  // For C.args
#define ROUTE_BUFFER_SIZE 128 // For C.route

struct RequestStruct {
  char *http_method;
  char *route;
  char *http_version;
  char *args;
};

struct ResponseStruct {
  char *status_line;
  char *headers;
  char *response_body;
};

// Global atomic flag to control server loop
volatile sig_atomic_t keep_running = 1;

// Signal handler function
void signal_handler(int signum) {
  if (signum == SIGINT) {
    fprintf(stderr, "\nSIGINT received. Shutting down server...\n");
    keep_running = 0; // Set the flag to stop the main loop
  }
}

typedef struct RequestStruct ClientRequest;
typedef struct ResponseStruct ServerResponse;

// A function to initialize the server responses
int initializeServerResponses(char *serverResponse[], int response_code,
                              const char *response_text,
                              size_t response_length);

// A function to free the server responses
void freeServerResponses(char *serverResponse[], size_t count);

// A function to setup the server socket
int setup_server_socket(int port);

// A function to handle the client
void handle_client(int client_fd, char *serverResponse[]);

// A function to parse the client request into a structured field
ClientRequest parseRequest(const char *client_req_str, ssize_t req_size);

// A function to return an appropriate response
ServerResponse echoResponse(ClientRequest C, char **serverResponse);

// Helper function to free ClientRequest
void freeClientRequest(ClientRequest *C) {
  if (C->http_method)
    free(C->http_method);
  if (C->route)
    free(C->route);
  if (C->http_version)
    free(C->http_version);
  if (C->args)
    free(C->args);
  C->http_method = C->route = C->http_version = C->args = NULL;
}

// Helper function to free ServerResponse
void freeServerResponse(ServerResponse *S) {
  if (S->status_line)
    free(S->status_line);
  if (S->headers)
    free(S->headers);
  if (S->response_body)
    free(S->response_body);
  S->status_line = S->headers = S->response_body = NULL;
}

int main() {
  // Disable output buffering so that the values sent to buffer are immediately
  // sent to respective streams

  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // Setup signal handler for graceful shutdown
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler; // Set our custom handler
  sigemptyset(&sa.sa_mask);       // Clear mask of blocked signals
  sa.sa_flags = 0;                // No special flags
  if (sigaction(SIGINT, &sa, NULL) ==
      -1) { // Register handler for SIGINT (Ctrl+C)
    perror("sigaction failed");
    return EXIT_FAILURE;
  }

  char *serverResponse[RESPONSE_CODES] = {0};
  if (initializeServerResponses(serverResponse, 200, "HTTP/1.1 200 OK\r\n",
                                strlen("HTTP/1.1 200 OK\r\n")) < 0 ||
      initializeServerResponses(serverResponse, 404,
                                "HTTP/1.1 404 Not Found\r\n",
                                strlen("HTTP/1.1 404 Not Found\r\n")) < 0) {
    fprintf(stderr, "Failed to initialize server responses.\n");
    freeServerResponses(serverResponse, RESPONSE_CODES);
    return EXIT_FAILURE;
  }

  int server_fd = setup_server_socket(PORT);
  if (server_fd < 0) {
    freeServerResponses(serverResponse, RESPONSE_CODES);
    return EXIT_FAILURE;
  }

  printf("Logs from the program will appear here.\n");
  printf("Waiting for a client to connect on port %d...\n", PORT);
  printf("Press Ctrl+C to stop the server.\n");

  while (keep_running) {
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
      break; // Exit the loop if a signal was received
    }

    if (client_fd < 0) {
      // If accept was interrupted by a signal, errno might be EINTR.
      // EINTR should not be treated as a fatal error, but continue the loop to
      // check keep_running.
      if (errno == EINTR) {
        fprintf(stderr,
                "Accept interrupted by signal. Checking shutdown flag.\n");
        continue;
      }
      fprintf(stderr, "Connection failed: %s\n", strerror(errno));
      continue;
    }
    printf("Client Connected.\n");
    handle_client(client_fd, serverResponse);
    printf("Client Disconnected.\n");
    close(client_fd);
  }

  close(server_fd);
  freeServerResponses(serverResponse, RESPONSE_CODES);
  printf("Server shut down successfully.\n");
  return EXIT_SUCCESS;
}

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

int initializeServerResponses(char *serverResponse[], int response_code,
                              const char *response_text,
                              size_t response_length) {
  if (response_code >= RESPONSE_CODES || response_code < 0) {
    fprintf(stderr, "Invalid response_code: %d\n", response_code);
    return -1;
  }
  if (serverResponse[response_code] != NULL) {
    free(serverResponse[response_code]);
  }
  serverResponse[response_code] = malloc(response_length + 1);
  if (!serverResponse[response_code]) {
    perror("malloc failed for serverResponse");
    return -1;
  }
  memcpy(serverResponse[response_code], response_text, response_length);
  serverResponse[response_code][response_length] = '\0';
  return 0;
}

void freeServerResponses(char *serverResponse[], size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (serverResponse[i]) {
      free(serverResponse[i]);
      serverResponse[i] = NULL;
    }
  }
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

ClientRequest parseRequest(const char *client_req_str, ssize_t req_size) {
  ClientRequest C = {NULL, NULL, NULL, NULL}; // Initialize all members to NULL
  if (req_size <= 0 || client_req_str == NULL)
    return C;

  const char *line_end = memchr(client_req_str, '\n', req_size);
  if (!line_end) {
    fprintf(stderr, "Request line too long or no newline found.\n");
    return C;
  }

  // Calculate length of the first line, ensuring it's not too long for buffer
  size_t line_len = line_end - client_req_str;
  if (line_len >= BUFFER_SIZE) {
    fprintf(stderr, "Request line exceeds internal buffer: %zu vs %d\n",
            line_len, BUFFER_SIZE);
    line_len = BUFFER_SIZE - 1; // Truncate to fit
  }

  char line[BUFFER_SIZE]; // Stack buffer for the request line
  memcpy(line, client_req_str, line_len);
  line[line_len] = '\0';

  C.http_method = malloc(16);
  C.route = malloc(ROUTE_BUFFER_SIZE); // Use defined size
  C.http_version = malloc(16);
  C.args = NULL; // Will be allocated only if /echo/ route

  if (!C.http_method || !C.route || !C.http_version) {
    perror("malloc failed in parseRequest");
    freeClientRequest(&C); // Free what was allocated
    return C;              // Return the partially (or fully) NULLed struct
  }

  // Use ROUTE_BUFFER_SIZE - 1 for sscanf field width
  int n =
      sscanf(line, "%15s %127s %15s", C.http_method, C.route, C.http_version);
  if (n != 3) {
    fprintf(stderr,
            "sscanf failed to parse request line."
            "Items matched: %d. Line: '%s'\n",
            n, line);
    freeClientRequest(&C);
    return C;
  }

  const char *echo_prefix = "/echo/";
  size_t echo_prefix_len = strlen(echo_prefix);
  if (strncmp(C.route, echo_prefix, echo_prefix_len) == 0) {
    C.args = malloc(ARGS_BUFFER_SIZE);
    if (!C.args) {
      perror("malloc failed for C.args in parseRequest");
      freeClientRequest(&C); // free method, route, version
      return C;
    }
    C.args[0] = '\0'; // Initialize args
    const char *arg_start = C.route + echo_prefix_len;
    strncpy(C.args, arg_start, ARGS_BUFFER_SIZE - 1);
    C.args[ARGS_BUFFER_SIZE - 1] = '\0';

    // Stop at the first space if present in the echoed argument
    char *space_in_arg = strchr(C.args, ' ');
    if (space_in_arg) {
      *space_in_arg = '\0';
    }
  }

  return C;
}

ServerResponse echoResponse(ClientRequest C, char **serverResponse) {
  ServerResponse S = {NULL, NULL, NULL}; // Initialize all to NULL

  // contentLen is temporary and its content is copied by snprintf
  char contentLen_str[12]; // Stack buffer for string representation of length
  contentLen_str[0] = '\0';

  if (C.args == NULL) { // Should not happen if parseRequest initialized C.args
                        // correctly for /echo/
    fprintf(stderr,
            "Error: C.args is NULL in echoResponse for /echo/ route.\n");
    return S; // Return empty ServerResponse
  }

  snprintf(contentLen_str, sizeof(contentLen_str), "%d", (int)strlen(C.args));

  // Allocate memory for response parts
  S.status_line = malloc(BUFFER_SIZE);
  S.headers = malloc(BUFFER_SIZE);
  S.response_body = malloc(strlen(C.args) + 1); // Allocate exact size for body

  if (!S.status_line || !S.headers || !S.response_body) {
    perror("malloc failed in echoResponse");
    freeServerResponse(&S); // free whatever was allocated
    return S;               // Return partially or fully NULLed ServerResponse
  }

  snprintf(S.status_line, BUFFER_SIZE, "%s", serverResponse[200]);
  snprintf(S.headers, BUFFER_SIZE,
           "Content-Type: text/plain\r\nContent-Length: %s\r\n\r\n",
           contentLen_str);
  snprintf(S.response_body, strlen(C.args) + 1, "%s", C.args); // Use exact size

  return S;
}
