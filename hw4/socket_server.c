#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tpcfollower.h"
#include "tpcleader.h"
#include "kvconstants.h"
#include "socket_server.h"
#include "wq.h"

/* Handles requests for SERVER. */
static void *handle(void *server_) {
  /* (Valgrind) Detach so thread frees its memory on completion, since we won't
   * be joining on it. */
  pthread_detach(pthread_self());
  server_t *server = (server_t *)server_;
  int sockfd;
  if (server->leader) {
    tpcleader_t *tpcleader = &server->tpcleader;
    while (server->listening) {
      sockfd = (intptr_t)wq_pop(&server->wq);
      tpcleader_handle(tpcleader, sockfd);
      close(sockfd);
    }
  } else {
    tpcfollower_t *tpcfollower = &server->tpcfollower;
    while (server->listening) {
      sockfd = (intptr_t)wq_pop(&server->wq);
      tpcfollower_handle(tpcfollower, sockfd);
      close(sockfd);
    }
  }
  return NULL;
}

/* Connects to the host given at HOST:PORT using a TIMEOUT second timeout.
 * Returns a socket fd which should be closed, else -1 if unsuccessful. */
int connect_to(const char *host, int port, int timeout) {
  struct sockaddr_in addr;
  struct hostent *ent;
  int sockfd;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  ent = gethostbyname(host);
  if (ent == NULL) {
    close(sockfd);
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr.s_addr, ent->h_addr, ent->h_length);
  addr.sin_port = htons(port);
  if (timeout > 0) {
    struct timeval t;
    t.tv_sec = timeout;
    t.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&t, sizeof(t));
  }
  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sockfd);
    return -1;
  }
  return sockfd;
}

/* Runs SERVER such that it indefinitely (until server_stop is called) listens
 * for incoming requests at HOSTNAME:PORT.
 *
 * As given, this function will synchronously handle only a single request
 * at a time. It is your task to modify it such that it can handle up to
 * SERVER->max_threads jobs at a time asynchronously. */
int server_run(const char *hostname, int port, server_t *server) {
  int sock_fd, client_sock, socket_option;
  struct sockaddr_in client_address;
  size_t client_address_length = sizeof(client_address);
  wq_init(&server->wq);
  server->listening = 1;
  server->port = port;
  server->hostname = (char *)malloc(strlen(hostname) + 1);
  if (!server->hostname)
    fatal_malloc();
  strcpy(server->hostname, hostname);

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  server->sockfd = sock_fd;
  if (sock_fd == -1) {
    fprintf(stderr, "Failed to create a new socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }
  socket_option = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof(socket_option)) == -1) {
    fprintf(stderr, "Failed to set socket options: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }
  memset(&client_address, 0, sizeof(client_address));
  client_address.sin_family = AF_INET;
  client_address.sin_addr.s_addr = INADDR_ANY;
  client_address.sin_port = htons(port);
  if (bind(sock_fd, (struct sockaddr *)&client_address, sizeof(client_address)) == -1) {
    fprintf(stderr, "Failed to bind on socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }
  if (listen(sock_fd, 1024) == -1) {
    fprintf(stderr, "Failed to listen on socket: error %d: %s\n", errno, strerror(errno));
    exit(errno);
  }

  pthread_t workers[server->max_threads];
  for (int i = 0; i < server->max_threads; i++) {
    pthread_create(&workers[i], NULL, handle, server);
  }

  while (server->listening) {
    client_sock =
        accept(sock_fd, (struct sockaddr *)&client_address, (socklen_t *)&client_address_length);
    if (client_sock > 0) {
      wq_push(&server->wq, (void *)(intptr_t)client_sock);
    }
  }
  shutdown(sock_fd, SHUT_RDWR);
  close(sock_fd);
  return 0;
}

/* Stops SERVER from continuing to listen for incoming requests. */
void server_stop(server_t *server) {
  server->listening = 0;
  shutdown(server->sockfd, SHUT_RDWR);
  close(server->sockfd);
}
