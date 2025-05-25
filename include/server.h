#ifndef SERVER_H
#define SERVER_H

#include "http_types.h" // For ClientRequest, ServerResponse
// #include <netinet/in.h> // For sockaddr_in (if not included elsewhere)
#include <sys/types.h> // For socklen_t

// A function to setup the server socket
int setup_server_socket(int port);

// New function to handle accepting a client connection
// Returns client_fd on success, -1 on signal interrupt, or other negative on
// error
int handle_accept(int server_fd);

// A function to handle the client
void handle_client(int client_fd, char *serverResponse[]);

#endif // SERVER_H
