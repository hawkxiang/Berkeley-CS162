#ifndef __TPC_FOLLOWER__
#define __TPC_FOLLOWER__

#include <stdbool.h>
#include "kvstore.h"
#include "kvmessage.h"
#include "tpclog.h"

/* TPCFollower defines a server which will be used to store <key, value> pairs.  A TPCFollower is
 * orchestrated via a TPCLeader server.
 *
 * Ideally, each TPCFollower would be running on its own machine with its own file storage.
 *
 * A TPCFollower accepts incoming messages on a socket using the HTTP API described in the spec,
 * and responds accordingly on the same socket. There is one generic entrypoint,
 * tpcfollower_handle, which takes in a socket that has already been connected to a leader or
 * client and handles all further communication.
 *
 * A TPCFollower has an associated KVStore.
 *
 * Because the KVStore stores all data in persistent file storage, a non-TPC TPCFollower can be
 * reinitialized using a DIRNAME which contains a previous TPCFollower and all old entries will be
 * available, enabling easy crash recovery.
 *
 * A TPCFollower maintains state beyond the current KVStore entries, so a TPCLog is used to log
 * incoming requests and can be used to recreate the state of the server upon crash recovery.
 */
struct tpcfollower;

/* A TPCFollower. Stores the associated KVStore. */
typedef struct tpcfollower {
  kvstore_t store; /* The store this server will use. */
  tpclog_t log;    /* The log this server will use. */
  tpc_state_t state;
  msgtype_t pending_msg;
  char pending_key[MAX_KEYLEN + 1];
  char pending_value[MAX_VALLEN + 1];
  int max_threads;   /* The max threads this server will run on. */
  int listening;     /* 1 if this server is currently listening for requests, else 0. */
  int sockfd;        /* The socket fd this server is currently listening on (if any).  */
  int port;          /* The port this server should listen on. */
  char hostname[64]; /* The host this server should listen on. */
} tpcfollower_t;

int tpcfollower_init(tpcfollower_t *, char *dirname, unsigned int max_threads, const char *hostname,
                     int port);

bool tpcfollower_register_leader(tpcfollower_t *server, int sockfd);

void tpcfollower_handle(tpcfollower_t *server, int sockfd);

void tpcfollower_handle_tpc(tpcfollower_t *, kvrequest_t *, kvresponse_t *);

int tpcfollower_get(tpcfollower_t *, char *key, char *value);
int tpcfollower_put(tpcfollower_t *, char *key, char *value);
int tpcfollower_del(tpcfollower_t *, char *key);

int tpcfollower_rebuild_state(tpcfollower_t *);

int tpcfollower_clean(tpcfollower_t *);

#endif
