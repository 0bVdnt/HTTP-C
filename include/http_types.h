#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#include <stdlib.h>

typedef struct RequestStruct ClientRequest;
typedef struct ResponseStruct ServerResponse;

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

// A function to free the server response
void freeServerResponse(ServerResponse *S);

// A function to free the client request
void freeClientRequest(ClientRequest *C);

#endif // HTTP_TYPES_H
