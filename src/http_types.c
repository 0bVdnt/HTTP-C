#include "../include/http_types.h"
#include <stdlib.h>

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
