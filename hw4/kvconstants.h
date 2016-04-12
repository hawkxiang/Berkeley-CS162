#ifndef __KV_CONSTANTS__
#define __KV_CONSTANTS__

/* KVConstants contains general purpose constants for use throughout the
 * project. */

#include <execinfo.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "md5.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))

#define fatal(msg, code)                                                                           \
  {                                                                                                \
    fprintf(stderr, "%s\n", msg);                                                                  \
    void *btarray[10];                                                                             \
    int btents = backtrace(btarray, 10);                                                           \
    backtrace_symbols_fd(btarray, btents, 2);                                                      \
    exit(code);                                                                                    \
  }

#define fatal_malloc() fatal("malloc failed", 55); /* 55 == ENOBUGS */

/* Default timeout (in seconds) for a socket */
#define TIMEOUT 10

/* Maximum length for keys and values. */
#define MAX_KEYLEN 1024
#define MAX_VALLEN 1024

/* Maximum size for a KVResponse body. */
#define KVRES_BODY_MAX_SIZE 40

/* Maximum size for a valid URL path (i.e. "register") */
#define PATH_MAX_SIZE 8

/* Maximum size for an HTTP message */
#define HTTP_MSG_MAX_SIZE (PATH_MAX_SIZE + MAX_KEYLEN + MAX_VALLEN + KVRES_BODY_MAX_SIZE + 20)

/* Maximum length for a file name. */
#define MAX_FILENAME 1024

/* Error messages to be used with KVMessage. */
#define MSG_COMMIT "commit"
#define ERRMSG_NO_KEY "error: no key"
#define ERRMSG_KEY_LEN "error: improper key length"
#define ERRMSG_VAL_LEN "error: value too long"
#define ERRMSG_INVALID_REQUEST "error: invalid request"
#define ERRMSG_NOT_IMPLEMENTED "error: not implemented"
#define ERRMSG_NOT_AT_CAPACITY "error: follower_capacity not yet full"
#define ERRMSG_FOLLOWER_CAPACITY "error: follower capacity already full"
#define ERRMSG_GENERIC_ERROR "error: unable to process request"

/* Error types/values */
/* Error for invalid key length. */
#define ERR_KEYLEN -11
/* Error for invalid value length. */
#define ERR_VALLEN -12
/* Error for invalid (not present) key. */
#define ERR_NOKEY -13
/* Error for invalid message type. */
#define ERR_INVLDMSG -14
/* Error returned if error was encountered accessing a file.
 * NOTE: You shouldn't have to use this one. */
#define ERR_FILACCESS -17

/* Convert an error code to an error message.
 * GETMSG(ERR_NOKEY) --> ERRMSG_NO_KEY --> "error: no key"
 **/
#define GETMSG(error)                                                                              \
  ((error == ERR_KEYLEN)                                                                           \
       ? ERRMSG_KEY_LEN                                                                            \
       : ((error == ERR_VALLEN) ? ERRMSG_VAL_LEN                                                   \
                                : ((error == ERR_NOKEY) ? ERRMSG_NO_KEY : ERRMSG_GENERIC_ERROR)))

/* Paths for API endpoints. */
#define COMMIT_PATH MSG_COMMIT
#define ABORT_PATH "abort"
#define REGISTER_PATH "register"

/* Message types for use by KVMessage. */
typedef enum {
  /* Requests */
  INDEX,
  GETREQ,
  PUTREQ,
  DELREQ,
  REGISTER,
  COMMIT,
  ABORT,
  /* Responses */
  GETRESP,
  SUCCESS,
  ERROR,
  VOTE,
  ACK,
  /* Empty (interal error) */
  EMPTY
} msgtype_t;

/* Possible TPC states. */
typedef enum { TPC_INIT, TPC_WAIT, TPC_READY, TPC_ABORT, TPC_COMMIT } tpc_state_t;

/* A 64-bit string hash, based on MD5.
 * Do NOT change this function. */
static inline uint64_t strhash64(const char *str) {
  unsigned char result[16];
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, str, strlen(str));
  MD5_Final(result, &ctx);
  /* This only works for little-endian machines. */
  return *(uint64_t *)result;
}

static inline bool is_empty_str(const char *str) { return str[0] == '\0'; }

#endif
