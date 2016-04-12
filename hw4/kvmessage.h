#ifndef __KV_MESSAGE__
#define __KV_MESSAGE__

#include "kvconstants.h"

/* Structs and methods for KVRequest and KVResponse, our internal
 * representation of API messages.  */

typedef struct {
  msgtype_t type;
  char key[MAX_KEYLEN + 1]; // May be NULL, depending on type.
  char val[MAX_VALLEN + 1]; // May be NULL, depending on type.
} kvrequest_t;

typedef struct {
  msgtype_t type;
  char body[KVRES_BODY_MAX_SIZE + 1]; // May be NULL, depending on type.
} kvresponse_t;

/* Recieves an HTTP request on SOCKFD and unmarshalls it into a KVRequest. */
bool kvrequest_receive(kvrequest_t *, int sockfd);

/* Recieves an HTTP response on SOCKFD and unmarshalls it into a KVResponse. */
bool kvresponse_receive(kvresponse_t *, int sockfd);

/* Marshalls a KVRequest or KVResponse into a HTTP message, respectively, and
 * sends it on SOCKFD. */
int kvrequest_send(kvrequest_t *, int sockfd);
int kvresponse_send(kvresponse_t *, int sockfd);

/* Helper methods to clear a KVRequest and KVResponse, respectively. */
void kvrequest_clear(kvrequest_t *);
void kvresponse_clear(kvresponse_t *);

#endif
