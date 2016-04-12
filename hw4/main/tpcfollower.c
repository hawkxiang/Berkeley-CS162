#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "socket_server.h"
#include "tpcfollower.h"

const char *USAGE = "Usage: tpcfollower "
                    "[follower_port (default=16201)] "
                    "[leader_port (default=16200)]";

int main(int argc, char **argv) {
  int follower_port = 16201, leader_port = 16200;
  char *follower_hostname = "127.0.0.1", *leader_hostname = "127.0.0.1";
  int index = 0;
  if (index < argc) {
    switch (argc - index - 1) {
    case 1:
      index += 1;
      if (argv[index][0] != '-') {
        follower_port = atoi(argv[index]);
        break;
      } else {
        goto usage;
      }
    case 2:
      index += 1;
      if (argv[index][0] != '-') {
        follower_port = atoi(argv[index]);
      } else {
        goto usage;
      }

      if (argv[index + 1][0] != '-') {
        leader_port = atoi(argv[index + 1]);
      } else {
        goto usage;
      }
      break;
    }
  }

  printf("Follower server started on port %d\n", follower_port);
  printf("Connecting to leader at %s:%d... \n", leader_hostname, leader_port);

  tpcfollower_t follower;
  server_t server;
  server.leader = 0;
  server.max_threads = 3;

  char follower_name[20];
  sprintf(follower_name, "follower-port%d", follower_port);

  tpcfollower_init(&follower, follower_name, 2, follower_hostname, follower_port);
  /* Need to send registration to the leader.*/
  int ret, sockfd = connect_to(leader_hostname, leader_port, 0);
  if (sockfd < 0) {
    printf("Error registering follower! "
           "Could not connect to leader on host %s at port %d\n",
           leader_hostname, leader_port);
    return 1;
  }
  ret = tpcfollower_register_leader(&follower, sockfd);
  if (ret < 0) {
    printf("Error registering follower with leader! "
           "Received an error message back from leader.\n");
    return 1;
  }
  close(sockfd);
  server.tpcfollower = follower;
  server_run(follower_hostname, follower_port, &server);
  return 0;

usage:
  printf("%s\n", USAGE);
  return 1;
}
