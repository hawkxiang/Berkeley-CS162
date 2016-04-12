#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include "socket_server.h"
#include "tpcleader.h"

const char *USAGE = "Usage: tpcleader [port (default=16200)] [followers "
                    "(default=1)] [redundancy (default=1)]\n\t";

int main(int argc, char **argv) {
  int port = 16200;
  int followers = 1;
  int redundancy = 1;
  server_t server;

  if (argc > 4) {
    printf("%s\n", USAGE);
    return 1;
  }
  if (argc > 1) {
    port = atoi(argv[1]);
  }
  if (argc > 2) {
    followers = atoi(argv[2]);
  }
  if (argc > 3) {
    redundancy = atoi(argv[3]);
  }

  server.leader = 1;
  server.max_threads = 3;
  tpcleader_init(&server.tpcleader, followers, redundancy);
  printf("TPCLeader server started listening on port %d...\n", port);
  server_run("127.0.0.1", port, &server);
}
