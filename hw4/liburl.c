#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include "kvconstants.h"
#include "liburl.h"

void zero_params(url_params_t *params) {
  memset(params->path, 0, PATH_MAX_SIZE + 1);
  memset(params->key, 0, MAX_KEYLEN + 1);
  memset(params->val, 0, MAX_VALLEN + 1);
}

struct param {
  char *ptr;
  int max_size;
};

static bool get_param_ptr(struct param *p, url_params_t *params, char *key, int keylen) {
  if (!strncmp(key, "key", keylen)) {
    p->ptr = params->key;
    p->max_size = MAX_KEYLEN;
    return true;
  }
  if (!strncmp(key, "val", keylen)) {
    p->ptr = params->val;
    p->max_size = MAX_VALLEN;
    return true;
  }
  return false;
}

bool url_decode(url_params_t *params, char *url) {
  char *read_end;
  size_t read_size;

  /* Write path to url_params struct */
  read_end = strchr(url, '?');
  if (read_end == NULL) { /* No params found. */
    read_size = strlen(url) - 1;
  } else {
    read_size = read_end - url - 1;
  }
  read_size = min(read_size, PATH_MAX_SIZE);
  if (read_size > 0) {
    memcpy(params->path, url + 1, read_size);
    params->path[read_size] = '\0';
  }
  if (read_end == NULL) {
    goto end; /* No params to parse. */
  } else {
    url = read_end + 1;
  }

  /* Loop through parameters, pulling only those that we support (i.e., key,
   * val) */
  struct param param;
  bool found_param = false;
  char *key_end;
  while (*read_end != '\0') {
    key_end = strchr(url, '=');
    if (key_end == NULL)
      break;
    read_end = strchr(url, '&');
    if (read_end == NULL)
      read_end = url + strlen(url);

    found_param = get_param_ptr(&param, params, url, key_end - url);
    if (!found_param) {
      if (*url == '\0')
        goto end;
      url = read_end + 1;
      continue;
    }

    read_size = min(read_end - key_end - 1, param.max_size);
    memcpy(param.ptr, key_end + 1, read_size);
    param.ptr[read_size] = '\0';

    url = read_end + 1;
  }

end:
  return true;
}

void url_encode(char *url, url_params_t *params) {
  char buf[HTTP_MSG_MAX_SIZE];
  int end = 0;
  if (params->path)
    end += sprintf(buf + end, "/%s?", params->path);
  else
    end += sprintf(buf + end, "/?");
  if (params->key)
    end += sprintf(buf + end, "key=%s&", params->key);
  if (params->val)
    end += sprintf(buf + end, "val=%s&", params->val);
  buf[end - 1] = '\0';

  strcpy(url, buf);
}
