#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "httpproxy.h"
void *handle (void *vargp) {
    int client_fd = *(int *)vargp;
    int server_fd = 0;
    int rc, len = 0;
    struct sockaddr_in server_addr;
    //struct sockaddr_in *server_addr;
    struct addrinfo hint, *ailist, *aip;
    char buffer[8192];

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
                                                                      
    if ((rc = getaddrinfo(server_proxy_hostname, "http", &hint, &ailist)) != 0) {
        perror("get ip failed");
        close(client_fd);
        return (void*)0;
    }
    /*server_addr = (struct sockaddr_in *)aip;
    server_addr->sin_port = htons(server_proxy_port);*/

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_proxy_hostname);
    server_addr.sin_port = htons(server_proxy_port);
    connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    do{
        rc = recv(client_fd, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("recv() failed");
            break;
        }

        if (rc == 0) {
            printf("Connect closed \n");
            break;
        }
        len = rc;
        rc = send(server_fd, buffer, len, 0);
        if (rc < 0) {
            perror("send() to server failed");
            break;
        }
        if (rc < 8192) break;

    } while (1);
    

   if (len)
    do {
        rc = recv(server_fd, buffer, sizeof(buffer), 0);
        if (rc == -1) {
            perror("recv from server failed");
            break;
        }

        if (rc == 0) {
            printf("server connect closed \n");
            break;
        }

        len = rc;
        rc = send(client_fd, buffer, len, 0);
        if (rc < 0) {
            perror("send() to client failed");
            break;
        }
    } while (1);
    
    close(server_fd);
    close(client_fd);
    return ((void*)0);
}


void proxy() {
    int rc, on = 1;
    int listen_fd = -1, accept_fd = -1;
    struct sockaddr_in proxy_addr, client_addr;
    size_t client_addr_len = sizeof(client_addr);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("Failed to create a new proxy socket");
        exit(errno);
    }

    rc = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                (char *)&on, sizeof(on));
    if (rc < 0) {
        perror("setsockopt() failed");
        close(listen_fd);
        exit(errno);
    }

    memset(&proxy_addr, 0, sizeof(proxy_addr));
    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(server_port);
    
    rc = bind(listen_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr));
    if (rc == -1) {
        perror("bind() failed");
        close(listen_fd);
        exit(errno);
    }

    rc = listen(listen_fd, 512);
    if (rc == -1) {
        perror("listen() failed");
        close(listen_fd);
        exit(errno);
    }

    while (1) {
        accept_fd = accept(listen_fd, (struct sockaddr *)&client_addr, 
                (socklen_t *)&client_addr_len);
        if (accept_fd < 0) {
            perror("Error accepting socket");
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle, (void*)&accept_fd);
    }
}
