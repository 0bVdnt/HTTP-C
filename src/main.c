#include <stdio.h>  // For printf, fprintf, setvbuf, NULL, _IONBF
#include <stdlib.h> // For EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // For strlen
#include <unistd.h> // For close()
// Include your custom headers
#include "../include/config.h"
#include "../include/http_response.h"
#include "../include/server.h"
#include "../include/signal_handler.h"

int main() {
  // Disable output buffering so that the values sent to buffer are immediately
  // sent to respective streams
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // Setup signal handler for graceful shutdown
  if (setup_signal_handler() == -1) {
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
    int client_fd = handle_accept(server_fd);
    if (!keep_running) {
      fprintf(stderr, "Server is shutting down. Exiting accept loop.\n");
      break; // Exit the loop if a signal was received
    }

    if (client_fd == -2) { // Custom code for EINTR, continue loop
      continue;
    }
    if (client_fd < 0) { // Other fatal accept errors
      // Error message already printed by handle_accept
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
