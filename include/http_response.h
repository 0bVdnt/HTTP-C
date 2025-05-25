#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include "http_types.h" // For ClientRequest, ServerResponse
#include <string.h>     // For size_t

// A function to initialize the server response messages
int initializeServerResponses(char *serverResponse[], int response_code,
                              const char *response_text,
                              size_t response_length);

// A function to free the server responses
void freeServerResponses(char *serverResponse[], size_t count);

// A function to return an appropriate response
ServerResponse echoResponse(ClientRequest C, char **serverResponse);

#endif // HTTP_RESPONSE_H
