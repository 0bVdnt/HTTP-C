#include "../include/http_response.h"
#include "../include/config.h" // For RESPONSE_CODES, BUFFER_SIZE
#include <stdio.h>             // For fprintf, perror, snprintf
#include <stdlib.h>            // For malloc, free
#include <string.h>            // For size_t
#include <string.h>            // For strlen, memcpy

// A function to initialize the server responses
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

// A function to free the server responses
void freeServerResponses(char *serverResponse[], size_t count) {
  for (size_t i = 0; i < count; ++i) {
    if (serverResponse[i]) {
      free(serverResponse[i]);
      serverResponse[i] = NULL;
    }
  }
}

// A function to generate server responses to the client requests at the echo/
// endpoint
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
