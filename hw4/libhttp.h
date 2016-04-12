/*
 * A simple HTTP library for parsing and sending HTTP messages.
 */

#ifndef LIBHTTP_H
#define LIBHTTP_H

#include <stddef.h>
#include "kvconstants.h"

typedef enum {
  INVALID,
  GET,
  POST,
  PUT,
  DELETE,
  /* NOTE: There are other methods, but these are the only ones we care about. */
} http_method_t;

/*--- RECIEVING AND PARSING ---*/

typedef struct {
  http_method_t method;
  char path[HTTP_MSG_MAX_SIZE + 1];
} http_request_t;

typedef struct {
  int status;
  char body[HTTP_MSG_MAX_SIZE + 1];
} http_response_t;

bool http_request_receive(http_request_t *, int sockfd);
bool http_response_receive(http_response_t *, int sockfd);

/*--- SENDING ---*/

/* Represents an under-construction, outbound HTTP message. */
typedef struct {
  int fd;
  int end;
  char body[HTTP_MSG_MAX_SIZE + 1];
} http_outbound_t;

/* These two functions initialize an under-construction, outbound HTTP request
 * and response,
 * respecitvely. MSG should be further built upon using the other functions
 * below, and finally
 * sent with http_outbound_send.
 */
bool http_outbound_init_request(http_outbound_t *msg, int sockfd, http_method_t method, char *url);
bool http_outbound_init_response(http_outbound_t *msg, int sockfd, int status_code);

void http_outbound_add_header(http_outbound_t *, char *key, char *value);
void http_outbound_end_headers(http_outbound_t *);
void http_outbound_add_string(http_outbound_t *, char *data);
void http_outbound_add_data(http_outbound_t *, char *data, size_t size);

/* Sends off the http_outbound message and returns bytes send, or -1 on error.
 */
int http_outbound_send(http_outbound_t *);

#endif
