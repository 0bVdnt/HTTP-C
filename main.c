#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RESPONSE_CODES 600
#define PORT 4221
#define BACKLOG 5
#define BUFFER_SIZE 256

struct RequestStruct {
  char *http_method;
  char *route;
  char *http_version;
};

typedef struct RequestStruct ClientRequest;

// A function to initialize the server responses
int initializeServerResponses(char *serverResponse[], int response_code,
                              const char *response, size_t response_length);

// A function to free the server responses
void freeServerResponses(char *serverResponse[], size_t count);

// A function to setup the server socket
int setup_server_socket(int port);

// A function to handle the client
void handle_client(int client_fd, char *serverResponse[]);

ClientRequest parseRequest(char *client_req, ssize_t req_size);

int main() {
  // Disable output buffering so that the values sent to buffer are immediately
  // sent to respective streams
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  char *serverResponse[RESPONSE_CODES] = {0};
  if (initializeServerResponses(serverResponse, 200, "HTTP/1.1 200 OK\r\n\r\n",
                                strlen("HTTP/1.1 200 OK\r\n\r\n")) < 0 ||
      initializeServerResponses(serverResponse, 404,
                                "HTTP/1.1 404 Not Found\r\n\r\n",
                                strlen("HTTP/1.1 404 Not Found\r\n\r\n")) < 0) {
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
  while (1) {
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
    if (client_fd < 0) {
      fprintf(stderr, "Connection failed: %s\n", strerror(errno));
      continue;
    }
    printf("Client Connected.\n");
    handle_client(client_fd, serverResponse);
    close(client_fd);
  }

  close(server_fd);
  freeServerResponses(serverResponse, RESPONSE_CODES);
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
                              const char *response, size_t response_length) {
  if (response_code >= RESPONSE_CODES || response_code < 0)
    return -1;
  serverResponse[response_code] = malloc(response_length + 1);
  if (!serverResponse[response_code])
    return -1;
  memcpy(serverResponse[response_code], response, response_length);
  serverResponse[response_code][response_length] = '\0';
  return (int)response_length + 1;
}

void freeServerResponses(char *serverResponse[], size_t count) {
  for (size_t i = 0; i < count; ++i) {
    free(serverResponse[i]);
  }
}

void handle_client(int client_fd, char *serverResponse[]) {
  char client_req[BUFFER_SIZE] = {0};
  ssize_t bytes_read = read(client_fd, client_req, BUFFER_SIZE - 1);
  printf("%s", client_req);
  if (bytes_read < 0) {
    fprintf(stderr, "Failed to read from client: %s\n", strerror(errno));
    return;
  }
  client_req[BUFFER_SIZE - 1] = '\0'; // Ensure null-termination

  parseRequest(client_req, bytes_read);
  if (memcmp(client_req, "GET / ", 6) == 0) {
    if (send(client_fd, serverResponse[200], strlen(serverResponse[200]), 0) <
        0)
      fprintf(stderr, "Failed Sending Response: %s\n", strerror(errno));
    else
      printf("Response Sent.\n");
  } else {
    if (send(client_fd, serverResponse[404], strlen(serverResponse[404]), 0) <
        0)
      fprintf(stderr, "Failed Sending Response: %s\n", strerror(errno));
    else
      printf("Response Sent.\n");
  }
}

ClientRequest parseRequest(char *client_req, ssize_t req_size) {
  ClientRequest C = {NULL, NULL, NULL};
  if (req_size <= 0 || client_req == NULL)
    return C;

  // Find the end of the first line (request line)
  char *line_end = memchr(client_req, '\n', req_size);
  if (!line_end)
    return C;
  size_t line_len = line_end - client_req;
  if (line_len >= BUFFER_SIZE)
    line_len = BUFFER_SIZE - 1;

  char line[BUFFER_SIZE];
  memcpy(line, client_req, line_len);
  line[line_len] = '\0';

  // Allocate memory for struct fields
  C.http_method = malloc(16);
  C.route = malloc(128);
  C.http_version = malloc(16);
  if (!C.http_method || !C.route || !C.http_version) {
    free(C.http_method);
    free(C.route);
    free(C.http_version);
    C.http_method = C.route = C.http_version = NULL;
    return C;
  }

  // Parse the request line: METHOD PATH VERSION
  // Example: GET /echo/abc HTTP/1.1
  int n =
      sscanf(line, "%15s %127s %15s", C.http_method, C.route, C.http_version);
  if (n != 3) {
    free(C.http_method);
    free(C.route);
    free(C.http_version);
    C.http_method = C.route = C.http_version = NULL;
    return C;
  }

  // // If route starts with /echo/, extract the argument
  // const char *prefix = "/echo/";
  // size_t prefix_len = strlen(prefix);
  // if (strncmp(C.route, prefix, prefix_len) == 0) {
  //   strncpy(C.args, C.route + prefix_len, 127);
  //   C.args[127] = '\0';
  // }

  // printf("", C.http_method, C.http_version, C.route)
  return C;
}
