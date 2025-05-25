#include "../include/http_parser.h"
#include "../include/config.h" // For BUFFER_SIZE, ARGS_BUFFER_SIZE, ROUTE_BUFFER_SIZE
#include <stdio.h>             // For fprintf, perror, sscanf
#include <stdlib.h>            // For malloc
#include <string.h> // For memchr, memcpy, strncpy, strcmp, strncmp, strchr

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
