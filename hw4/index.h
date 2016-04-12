#ifndef __INDEX__
#define __INDEX__

#include <unistd.h>

extern const char index_payload[];
extern int index_payload_size;

#define index_send(sockfd, leader)                                                                 \
  do {                                                                                             \
    int bytes_left = index_payload_size, bytes_written;                                            \
    const char *payload = index_payload;                                                           \
    while (bytes_left > 0) {                                                                       \
      bytes_written = write(sockfd, payload, bytes_left);                                          \
      if (bytes_written < 0) {                                                                     \
        break;                                                                                     \
      }                                                                                            \
      bytes_left -= bytes_written;                                                                 \
      payload += bytes_written;                                                                    \
    }                                                                                              \
    if (leader)                                                                                    \
      write(sockfd, "<script>setServerType('leader');</script>", 41);                              \
    else                                                                                           \
      write(sockfd, "<script>setServerType('follower');</script>", 43);                            \
  } while (0)

#endif
