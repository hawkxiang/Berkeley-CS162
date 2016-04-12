#ifndef __SOCKETSERVER__
#define __SOCKETSERVER__

#include "tpcfollower.h"
#include "tpcleader.h"
#include "wq.h"

/* Socket Server defines helper functions for communicating over sockets.
 *
 * connect_to can be used to make a request to a listening host. You will not
 * need to modify this, but you will likely want to utilize it.
 *
 * server_run can be used to start a server (containing a TPCLeader or
 * TPCFollower) listening on a given port. See the comment above server_run for
 * more information.
 *
 * The server struct stores extra information on top of the stored TPCLeader or
 * TPCFollower.
 */

typedef struct server {
  int leader;      /* If this server represents a TPC Leader. */
  int listening;   /* If this server is currently listening. */
  int sockfd;      /* The socket fd this server is operating on. */
  int max_threads; /* The maximum number of concurrent jobs that can run. */
  int port;        /* The port this server will listen on. */
  char *hostname;  /* The hostname this server will listen on. */
  wq_t wq;         /* The work queue this server will use to process jobs. */
  union {          /* The tpcfollower OR tpcleader this server represents. */
    tpcfollower_t tpcfollower;
    tpcleader_t tpcleader;
  };
} server_t;

int connect_to(const char *host, int port, int timeout);
int server_run(const char *hostname, int port, server_t *server);
void server_stop(server_t *server);

#endif
