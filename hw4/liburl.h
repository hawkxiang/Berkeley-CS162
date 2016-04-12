/*
 * Helpers for parsing URL parameters.
 */

#ifndef LIBURL_H
#define LIBURL_H

#include <stdlib.h>
#include "kvconstants.h"

/*
 * URL path plus accepted query parameters from an HTTP request to our servers.
 */
typedef struct {
  char path[PATH_MAX_SIZE + 1];
  char key[MAX_KEYLEN + 1];
  char val[MAX_VALLEN + 1];
} url_params_t;

/* Helper method to zero out all fields in a url_params_t struct */
void zero_params(url_params_t *params);

/* Unmarshalls the valid paramters within URLSTR into PARAMS. */
bool url_decode(url_params_t *params, char *url);

/* Marshalls the non-null params from PARAMS into an HTTP-compatible URL string
 */
void url_encode(char *url, url_params_t *params);

#endif
