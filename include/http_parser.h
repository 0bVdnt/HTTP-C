#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "http_types.h" // For ClientRequest struct
#include <sys/types.h>  // For ssize_t

// A function to parse the client request into a structured field
ClientRequest parseRequest(const char *client_req_str, ssize_t req_size);
#endif // HTTP_PARSER_H
