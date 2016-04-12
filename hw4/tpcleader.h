#ifndef __KV_LEADER__
#define __KV_LEADER__

#include <pthread.h>
#include <inttypes.h>
#include "kvmessage.h"

/* TPCLeader defines a leader server which will communicate with multiple
 * follower servers.
 *
 * The TPCLeader behaves like a TPCFollower from the client's perspective, but
 * actually handles requests quite differently. For every PUT and DELETE request
 * it receives, the TPCLeader polls the followers relevant to the key in
 * question, asking for a VOTE. If a consesus to commit is reached, the
 * TPCLeader notifies the followers to COMMIT, else it commands to ABORT.
 *
 * The TPCLeader also listens for registration requests from TPCFollowers acting
 * as its followers. It must fill up to its follower capacity before it can
 *handle
 * any client request.
 *
 * For this project, you can assume that the TPCLeader will never fail. Thus,
 * you don't need to maintain a TPCLog for it.
 */

/* A struct used to represent the followers which this TPC Leader is aware of. */
typedef struct follower {
  uint64_t id;           /* The unique ID for this follower. */
  char *host;            /* The host where this follower can be reached. */
  unsigned int port;     /* The port where this follower can be reached. */
  struct follower *next; /* The next follower in the list of followers. */
  struct follower *prev; /* The previous follower in the list of followers. */
} follower_t;

/* A TPC Leader. */
struct tpcleader;
typedef struct tpcleader {
  unsigned int follower_capacity; /* The number of followers this leader will use. */
  unsigned int follower_count;    /* The current number of followers this leader is aware of. */
  unsigned int redundancy;        /* The number of followers a single value will be stored on. */
  follower_t *followers_head;     /* The head of the list of followers. */
  pthread_rwlock_t follower_lock; /* A lock used to protect the list of followers. */
} tpcleader_t;

int tpcleader_init(tpcleader_t *leader, unsigned int follower_capacity, unsigned int redundancy);

void tpcleader_register(tpcleader_t *leader, kvrequest_t *, kvresponse_t *);
follower_t *tpcleader_get_primary(tpcleader_t *leader, char *key);
follower_t *tpcleader_get_successor(tpcleader_t *leader, follower_t *predecessor);

void tpcleader_handle(tpcleader_t *leader, int sockfd);

void tpcleader_handle_get(tpcleader_t *leader, kvrequest_t *, kvresponse_t *);
void tpcleader_handle_tpc(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res);

#endif
