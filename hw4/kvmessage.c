#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "libhttp.h"
#include "liburl.h"
#include "kvmessage.h"

/* Receives an HTTP request from socket SOCKFD and decodes it into KVREQ.
 * Returns false if there is an error. */
bool kvrequest_receive(kvrequest_t *kvreq, int sockfd) {
  bool success = false;
  kvreq->type = EMPTY;

  http_request_t req;
  url_params_t params;
  zero_params(&params);

  success = http_request_receive(&req, sockfd);
  if (!success)
    goto error;
  success = url_decode(&params, req.path);
  if (!success)
    goto error;

  switch (req.method) {
  case GET: {
    kvreq->type = is_empty_str(params.key) ? INDEX : GETREQ;
    break;
  }
  case PUT: {
    if (is_empty_str(params.key) || is_empty_str(params.val))
      goto error;
    kvreq->type = PUTREQ;
    break;
  }
  case DELETE: {
    if (is_empty_str(params.key))
      goto error;
    kvreq->type = DELREQ;
    break;
  }
  case POST: {
    if (is_empty_str(params.path))
      goto error;
    if (!strcmp(params.path, REGISTER_PATH)) {
      if (is_empty_str(params.key) || is_empty_str(params.val))
        goto error;
      kvreq->type = REGISTER;
    } else if (!strcmp(params.path, COMMIT_PATH)) {
      kvreq->type = COMMIT;
    } else if (!strcmp(params.path, ABORT_PATH)) {
      kvreq->type = ABORT;
    }
    break;
  }
  default:
    goto error;
  }
  strcpy(kvreq->key, params.key);
  strcpy(kvreq->val, params.val);

  return true;

error:
  return false;
}

/* Maps HTTP response codes to their corresponding msgtype_t for KVMessages.
 * Returns EMPTY if the status code isn't supported. */
static msgtype_t kvresponse_get_status_code(short status) {
  switch (status) {
  case 200:
    return GETRESP;
  case 201:
    return SUCCESS;
  case 500:
    return ERROR;
  case 202:
    return VOTE;
  case 204:
    return ACK;
  default:
    return EMPTY;
  }
}

/* Receives an HTTP response from socket SOCKFD and decodes into KVRES.
 * Returns false if there is an error. */
bool kvresponse_receive(kvresponse_t *kvres, int sockfd) {
  bool success = false;

  http_response_t res;
  success = http_response_receive(&res, sockfd);
  if (!success)
    goto error;

  kvres->type = kvresponse_get_status_code(res.status);
  if (kvres->type == EMPTY)
    goto error;
  strcpy(kvres->body, res.body);

  return true;

error:
  return false;
}

http_method_t http_method_for_request_type(msgtype_t type) {
  switch (type) {
  case GETREQ:
    return GET;
  case PUTREQ:
    return PUT;
  case DELREQ:
    return DELETE;
  case REGISTER:
  case COMMIT:
  case ABORT:
    return POST;
  default:
    return INVALID;
  }
}

char *path_for_request_type(msgtype_t type) {
  switch (type) {
  case REGISTER:
    return "register";
  case COMMIT:
    return "commit";
  case ABORT:
    return "abort";
  default:
    return "";
  }
}

/* Sends REQ on socket SOCKFD. Returns the number of bytes which were sent, and
 * 1 on error. */
int kvrequest_send(kvrequest_t *kvreq, int sockfd) {
  http_method_t method = http_method_for_request_type(kvreq->type);
  if (method == INVALID)
    return -1;

  url_params_t params;
  strcpy(params.path, path_for_request_type(kvreq->type));
  strcpy(params.key, kvreq->key);
  strcpy(params.val, kvreq->val);

  char url[HTTP_MSG_MAX_SIZE + 1];
  url_encode(url, &params);

  http_outbound_t msg;
  if (!http_outbound_init_request(&msg, sockfd, method, url))
    return -1;

  http_outbound_end_headers(&msg);
  return http_outbound_send(&msg);
}

int http_code_for_response_type(msgtype_t type) {
  switch (type) {
  case GETRESP:
    return 200;
  case SUCCESS:
    return 201;
  case ERROR:
    return 500;
  case VOTE:
    return 202;
  case ACK:
    return 204;
  default:
    return -1;
  }
}

int kvresponse_send(kvresponse_t *kvres, int sockfd) {
  int code = http_code_for_response_type(kvres->type);
  if (code < 0)
    return -1;

  http_outbound_t msg;
  if (!http_outbound_init_response(&msg, sockfd, code))
    return -1;

  char lenbuf[10] = "0";
  if (strlen(kvres->body) > 0)
    sprintf(lenbuf, "%ld", strlen(kvres->body));
  http_outbound_add_header(&msg, "Content-Length", lenbuf);
  http_outbound_end_headers(&msg);
  http_outbound_add_string(&msg, kvres->body);

  return http_outbound_send(&msg);
}

void kvrequest_clear(kvrequest_t *req) {
  req->type = EMPTY;
  memset(req->key, 0, MAX_KEYLEN + 1);
  memset(req->val, 0, MAX_VALLEN + 1);
}

void kvresponse_clear(kvresponse_t *res) {
  res->type = EMPTY;
  memset(res->body, 0, KVRES_BODY_MAX_SIZE + 1);
}
