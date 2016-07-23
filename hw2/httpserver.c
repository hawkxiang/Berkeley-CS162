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


#include "libhttp.h"
#define NAME_LEN 1024
#define RESPONSE_SIZE 8192

/*
 * Global configuration variables.
 * You need to use these in your implementation of handle_files_request and
 * handle_proxy_request. Their values are set up in main() using the
 * command line arguments (already implemented for you).
 */
int server_port;
char *server_files_directory;
char *server_proxy_hostname;
int server_proxy_port;

/*
 * Reads an HTTP request from stream (fd), and writes an HTTP response
 * containing:
 *
 *   1) If user requested an existing file, respond with the file
 *   2) If user requested a directory and index.html exists in the directory,
 *      send the index.html file.
 *   3) If user requested a directory and index.html doesn't exist, send a list
 *      of files in the directory with links to each.
 *   4) Send a 404 Not Found response.
 */
void error_response(int fd);
void list_response(int fd, char *file, char* path);
void normal_response(int fd, int file_fd, int total_len);
void handle_files_request(int fd) {

  /* YOUR CODE HERE (Feel free to delete/modify the existing code below) */
  struct http_request *request = http_request_parse(fd);
  /* open the web server root directiory */
  char root_dir[NAME_LEN];
  if (getcwd(root_dir, NAME_LEN) == NULL)
      http_fatal_error("get web server root dir failed!"); 
  sprintf(root_dir+strlen(root_dir), "/%s%s", server_files_directory, request->path);
  root_dir[strlen(root_dir)] = 0;

  int root_fd, index_fd;
  if ((root_fd = open(root_dir, O_RDONLY)) == -1){
      error_response(fd);
      perror( root_dir);
  }
  /* chech this file is a directory or normal file */
  struct stat file_stat;
  if (fstat(root_fd, &file_stat) == -1)
        http_fatal_error("open request file stat failed!");

  if (S_ISDIR(file_stat.st_mode)) {
      if ((index_fd = openat(root_fd, "index.html", O_RDONLY)) == -1)
          list_response(fd, root_dir, request->path);
      else {
          /*  get the index.html */
          if (fstat(index_fd, &file_stat) < 0)
              http_fatal_error("get index.html stat failed");
          normal_response(fd, index_fd, file_stat.st_size);
      }
      close(root_fd);
      return;
  }
  normal_response(fd, root_fd, file_stat.st_size);
  close(root_fd);
}

void list_response(int fd, char *file, char *path) {
    DIR *dp;
    struct dirent *dirp;
    char buf[RESPONSE_SIZE+1];
    int clen = 0;
    if ((dp = opendir(file)) == NULL)
        http_fatal_error("open web source director failed");
    while ((dirp = readdir(dp)) != NULL) {
       struct stat child_stat;
       if (fstatat(dirfd(dp), dirp->d_name, &child_stat,AT_SYMLINK_NOFOLLOW) == -1) 
           http_fatal_error("fstatat error");
       if (S_ISDIR(child_stat.st_mode))
            clen += sprintf(buf+clen, "%s%s%s%s%s%s%s", "<a href=\"", path, "/", dirp->d_name, "/\">", dirp->d_name, "</a><br/>");
       else if(S_ISREG(child_stat.st_mode))
            clen += sprintf(buf+clen, "%s%s%s%s%s%s%s", "<a href=\"", path, "/", dirp->d_name, "\">", dirp->d_name, "</a><br/>");
    }
    buf[clen] = 0;
    closedir(dp);

    http_start_response(fd, 200);
    http_send_header(fd, "Content-type", "text/html");
    char snum[16]; 
    int term = sprintf(snum, "%d", clen);
    snum[term] = 0;
    http_send_header(fd, "Content-Length", snum);
    http_end_headers(fd);
    http_send_string(fd, buf);
}

void normal_response(int fd, int file_fd, int total_len) {
    http_start_response(fd, 200);
    http_send_header(fd, "Content-type", "text/html");
    char snum[16];
    int term = sprintf(snum, "%d", total_len);
    snum[term] = 0;
    http_send_header(fd, "Content-Length", snum);
    http_end_headers(fd);
    char buf[RESPONSE_SIZE+1];
    ssize_t rlen; 
    while(total_len) {
        rlen = read(file_fd, buf, RESPONSE_SIZE);
        total_len -= rlen;
        http_send_data(fd, buf, rlen);
    }
}

void error_response(int fd) {
    http_start_response(fd, 404);
    http_send_header(fd, "Content-type", "text/html");
    http_send_header(fd, "Content-Length", "0");
    http_end_headers(fd);
}

/*
 * Opens a connection to the proxy target (hostname=server_proxy_hostname and
 * port=server_proxy_port) and relays traffic to/from the stream fd and the
 * proxy target. HTTP requests from the client (fd) should be sent to the
 * proxy target, and HTTP responses from the proxy target should be sent to
 * the client (fd).
 *
 *   +--------+     +------------+     +--------------+
 *   | client | <-> | httpserver | <-> | proxy target |
 *   +--------+     +------------+     +--------------+
 */
void handle_proxy_request(int fd) {

  /* YOUR CODE HERE */

}

/*
 * Opens a TCP stream socket on all interfaces with port number PORTNO. Saves
 * the fd number of the server socket in *socket_number. For each accepted
 * connection, calls request_handler with the accepted fd number.
 */
void serve_forever(int *socket_number, void (*request_handler)(int)) {

  struct sockaddr_in server_address, client_address;
  size_t client_address_length = sizeof(client_address);
  int client_socket_number;
  pid_t pid;

  *socket_number = socket(PF_INET, SOCK_STREAM, 0);
  if (*socket_number == -1) {
    perror("Failed to create a new socket");
    exit(errno);
  }

  int socket_option = 1;
  if (setsockopt(*socket_number, SOL_SOCKET, SO_REUSEADDR, &socket_option,
        sizeof(socket_option)) == -1) {
    perror("Failed to set socket options");
    exit(errno);
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(server_port);

  if (bind(*socket_number, (struct sockaddr *) &server_address,
        sizeof(server_address)) == -1) {
    perror("Failed to bind on socket");
    exit(errno);
  }

  if (listen(*socket_number, 1024) == -1) {
    perror("Failed to listen on socket");
    exit(errno);
  }

  printf("Listening on port %d...\n", server_port);

  while (1) {

    client_socket_number = accept(*socket_number,
        (struct sockaddr *) &client_address,
        (socklen_t *) &client_address_length);
    if (client_socket_number < 0) {
      perror("Error accepting socket");
      continue;
    }

    printf("Accepted connection from %s on port %d\n",
        inet_ntoa(client_address.sin_addr),
        client_address.sin_port);

    pid = fork();
    if (pid > 0) {
      close(client_socket_number);
    } else if (pid == 0) {
      // Un-register signal handler (only parent should have it)
      signal(SIGINT, SIG_DFL);
      close(*socket_number);
      request_handler(client_socket_number);
      close(client_socket_number);
      exit(EXIT_SUCCESS);
    } else {
      perror("Failed to fork child");
      exit(errno);
    }
  }

  close(*socket_number);

}

int server_fd;
void signal_callback_handler(int signum) {
  printf("Caught signal %d: %s\n", signum, strsignal(signum));
  printf("Closing socket %d\n", server_fd);
  if (close(server_fd) < 0) perror("Failed to close server_fd (ignoring)\n");
  exit(0);
}

char *USAGE =
  "Usage: ./httpserver --files www_directory/ --port 8000\n"
  "       ./httpserver --proxy inst.eecs.berkeley.edu:80 --port 8000\n";

void exit_with_usage() {
  fprintf(stderr, "%s", USAGE);
  exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, signal_callback_handler);

  /* Default settings */
  server_port = 8000;
  void (*request_handler)(int) = NULL;

  int i;
  for (i = 1; i < argc; i++) {
    if (strcmp("--files", argv[i]) == 0) {
      request_handler = handle_files_request;
      free(server_files_directory);
      server_files_directory = argv[++i];
      if (!server_files_directory) {
        fprintf(stderr, "Expected argument after --files\n");
        exit_with_usage();
      }
    } else if (strcmp("--proxy", argv[i]) == 0) {
      request_handler = handle_proxy_request;

      char *proxy_target = argv[++i];
      if (!proxy_target) {
        fprintf(stderr, "Expected argument after --proxy\n");
        exit_with_usage();
      }

      char *colon_pointer = strchr(proxy_target, ':');
      if (colon_pointer != NULL) {
        *colon_pointer = '\0';
        server_proxy_hostname = proxy_target;
        server_proxy_port = atoi(colon_pointer + 1);
      } else {
        server_proxy_hostname = proxy_target;
        server_proxy_port = 80;
      }
    } else if (strcmp("--port", argv[i]) == 0) {
      char *server_port_string = argv[++i];
      if (!server_port_string) {
        fprintf(stderr, "Expected argument after --port\n");
        exit_with_usage();
      }
      server_port = atoi(server_port_string);
    } else if (strcmp("--help", argv[i]) == 0) {
      exit_with_usage();
    } else {
      fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
      exit_with_usage();
    }
  }

  if (server_files_directory == NULL && server_proxy_hostname == NULL) {
    fprintf(stderr, "Please specify either \"--files [DIRECTORY]\" or \n"
                    "                      \"--proxy [HOSTNAME:PORT]\"\n");
    exit_with_usage();
  }

  serve_forever(&server_fd, request_handler);

  return EXIT_SUCCESS;
}
