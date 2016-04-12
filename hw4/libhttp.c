#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdbool.h>

#include "kvconstants.h"
#include "libhttp.h"

#define FULLMSG_MAX_SIZE (HTTP_MSG_MAX_SIZE + 32)
#define METHOD_MAX_SIZE 6 // "DELETE"

#define min(a, b) (((a) < (b)) ? (a) : (b))

static char *http_get_response_message(int status_code);
static http_method_t http_method_from_string(char *method_buf);

bool http_request_receive(http_request_t *req, int fd) {
  char read_buffer[FULLMSG_MAX_SIZE + 1];
  int bytes_read = read(fd, read_buffer, FULLMSG_MAX_SIZE - 1);
  if (bytes_read <= 0)
    goto error;
  read_buffer[bytes_read] = '\0'; /* Always null-terminate. */

  char method_buf[METHOD_MAX_SIZE + 1];

  char *read_init, *read_end;
  size_t read_size;

  /* Read in the HTTP method: "[A-Z]*" */
  read_init = read_end = read_buffer;
  read_end = strchr(read_init, ' ');
  if (!read_end)
    goto error;
  read_size = min(read_end - read_init, METHOD_MAX_SIZE);
  memcpy(method_buf, read_init, read_size);
  method_buf[read_size] = '\0';
  req->method = http_method_from_string(method_buf);
  if (req->method == INVALID)
    goto error;
  read_end++;

  /* Read in the path. */
  if (*read_end != '/')
    goto error;
  read_init = read_end;
  while (*read_end != ' ' && *read_end != '\n') {
    if (*read_end == '\0')
      goto error;
    read_end++;
  }
  read_size = min(read_end - read_init, HTTP_MSG_MAX_SIZE);
  if (read_size == 0)
    goto error;
  memcpy(req->path, read_init, read_size);
  req->path[read_size] = '\0';

  return true;

error:
  return false;
}

bool http_response_receive(http_response_t *res, int fd) {
  char read_buffer[FULLMSG_MAX_SIZE + 1];
  int bytes_read = read(fd, read_buffer, FULLMSG_MAX_SIZE - 1);
  if (bytes_read <= 0)
    goto error;
  read_buffer[bytes_read] = '\0'; /* Always null-terminate. */

  char *read_init, *read_end, *read_limit;
  size_t read_size;
  size_t content_length = -1;
  int status = -1;

  /* Read in HTTP version: "HTTP/1.[01]" */
  read_init = read_end = read_buffer;
  read_end = strchr(read_init, ' ');
  if (!read_end || read_init + 8 != read_end)
    goto error;
  if (strncmp(read_init, "HTTP/1.1", 8) && strncmp(read_init, "HTTP/1.0", 8))
    goto error;
  read_end++;
  read_init = read_end;

  /* Read in the response status: "[0-9]{3}" */
  status = strtol(read_init, &read_end, 10);
  if (!status || read_init + 3 != read_end || *read_end != ' ')
    goto error;
  res->status = status;
  read_end++;
  read_init = read_end;

  /* Read in respone status text until \r\n */
  read_end = strstr(read_init, "\r\n");
  if (!read_end) {
    read_end = strstr(read_init, "\n");
    if (!read_end) {
      goto error;
    }
  }
  if (strncmp(read_init, http_get_response_message(res->status), read_end - read_init))
    goto error;

  /* Loop through headers until Content-Length, or bust. */
  while (strncmp(read_end, "\n\n", 2) && strncmp(read_end, "\r\n\r\n", 4)) {
    read_end += 2;
    read_init = read_end;
    read_limit = strstr(read_init, "\r\n");
    if (!read_limit) {
      read_limit = strstr(read_init, "\n");
      if (!read_limit) {
        goto error;
      }
    }
    read_end = strstr(read_init, ": ");
    if (!read_end || read_end > read_limit)
      goto error;                                                      /* Malformed header */
    if (!strncmp(read_init, "Content-Length", read_end - read_init)) { /* Found it! */
      read_init = read_end + 2;
      content_length = strtol(read_init, &read_end, 10);
      while (*read_end == ' ')
        read_end++;
      if (read_end != read_limit)
        goto error;
    }
    read_end = read_limit;
  }
  read_end += 4;
  read_init = read_end;

  /* Read body */
  read_size = min(content_length, HTTP_MSG_MAX_SIZE);
  memcpy(res->body, read_init, read_size);
  res->body[read_size] = '\0';

  return true;

error:
  return false;
}

static http_method_t http_method_from_string(char *method_buf) {
  if (!strcmp(method_buf, "GET"))
    return GET;
  if (!strcmp(method_buf, "POST"))
    return POST;
  if (!strcmp(method_buf, "PUT"))
    return PUT;
  if (!strcmp(method_buf, "DELETE"))
    return DELETE;
  return INVALID;
}

static char *http_method_to_string(http_method_t method) {
  switch (method) {
  case GET:
    return "GET";
  case POST:
    return "POST";
  case PUT:
    return "PUT";
  case DELETE:
    return "DELETE";
  default:
    return NULL;
  }
}

static char *http_get_response_message(int status_code) {
  switch (status_code) {
  case 100:
    return "Continue";
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 304:
    return "Not Modified";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 500:
    return "Internal Server Error";
  default:
    return NULL;
  }
}

bool http_outbound_init_request(http_outbound_t *msg, int fd, http_method_t method, char *url) {
  char *method_str = http_method_to_string(method);
  if (!method_str)
    return false;
  msg->end = sprintf(msg->body, "%s %s HTTP/1.1\r\n", method_str, url);
  msg->fd = fd;
  return true;
}

bool http_outbound_init_response(http_outbound_t *msg, int fd, int status_code) {
  char *status = http_get_response_message(status_code);
  if (!status)
    return false;
  msg->end = sprintf(msg->body, "HTTP/1.1 %d %s\r\n", status_code, status);
  msg->fd = fd;
  return true;
}

void http_outbound_add_header(http_outbound_t *msg, char *key, char *value) {
  msg->end += sprintf(msg->body + msg->end, "%s: %s\r\n", key, value);
}

void http_outbound_end_headers(http_outbound_t *msg) {
  msg->end += sprintf(msg->body + msg->end, "\r\n");
}

void http_outbound_add_string(http_outbound_t *msg, char *data) {
  if (!data)
    return;
  http_outbound_add_data(msg, data, strlen(data));
}

void http_outbound_add_data(http_outbound_t *msg, char *data, size_t size) {
  memcpy(msg->body + msg->end, data, size);
  msg->end += size;
}

int http_outbound_send(http_outbound_t *msg) {
  msg->body[msg->end] = '\0';

  char *send = msg->body;
  int bytes_left = msg->end;

  ssize_t bytes_sent;
  while (bytes_left > 0) {
    bytes_sent = write(msg->fd, send, bytes_left);
    if (bytes_sent < 0)
      return bytes_sent;
    bytes_left -= bytes_sent;
    send += bytes_sent;
  }

  return msg->end;
}
