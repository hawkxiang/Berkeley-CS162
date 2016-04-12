#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include "kvstore.h"
#include "kvconstants.h"

/* Initializes kvstore STORE. Uses DIRNAME as the directory in which to store
 * the entries of this store, creating the directory if necessary. Returns 0 if
 * successful, else a negative error code. */
int kvstore_init(kvstore_t *store, char *dirname) {
  struct stat st;
  if (stat(dirname, &st) == -1) {
    if (mkdir(dirname, 0700) == -1)
      return errno;
  }
  strcpy(store->dirname, dirname);
  pthread_rwlock_init(&store->lock, NULL);
  return 0;
}

/* Attempts to find an entry matching KEY within the store.
 *
 * Returns a nonnegative integer representing the location of the entry within
 * its hash chain (so, the entry's filename is "hash(key)-returnval.entry").
 *
 * Returns a negative error code if the entry is not found or an error
 * occurred.
 *
 * If VALUE is not NULL, the value of the entry will be placed into VALUE using
 * malloced memory which should be freed later. */
int find_entry(kvstore_t *store, char *key, char *value) {
  uint64_t hashval;
  unsigned int counter = 0;
  char currfile[MAX_FILENAME];
  size_t keylen = strlen(key);
  struct stat st;
  FILE *file;
  kventry_t header;
  if (keylen > MAX_KEYLEN)
    return ERR_KEYLEN;
  if (stat(store->dirname, &st) == -1)
    return ERR_FILACCESS;
  hashval = strhash64(key);
  pthread_rwlock_rdlock(&store->lock);
  sprintf(currfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter++, KVSTORE_FILETYPE);
  while (stat(currfile, &st) != -1) {
    if ((file = fopen(currfile, "r")) == NULL) {
      pthread_rwlock_unlock(&store->lock);
      return ERR_FILACCESS;
    }
    fread(&header, sizeof(kventry_t), 1, file);
    char entry_data[header.length + 1];
    fread(entry_data, sizeof(char), header.length, file);
    fclose(file);
    if (strcmp(key, entry_data) == 0) {
      if (value != NULL)
        strcpy(value, entry_data + strlen(entry_data) + 1);
      pthread_rwlock_unlock(&store->lock);
      return counter - 1;
    }
    sprintf(currfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter++, KVSTORE_FILETYPE);
  }
  pthread_rwlock_unlock(&store->lock);
  return ERR_NOKEY;
}

/* Returns true if STORE contains KEY, else false. */
bool kvstore_haskey(kvstore_t *store, char *key) { return find_entry(store, key, NULL) >= 0; }

/* Attempts to retrieve the entry denoted by KEY from STORE.
 * Returns 0 if successful, else a negative error code. The entry's value will
 * be placed into VALUE using malloc()d memory which should be free()d later. */
int kvstore_get(kvstore_t *store, char *key, char *value) {
  int ret = find_entry(store, key, value);
  if (ret < 0)
    return ret;
  else
    return 0;
}

/* Checks if STORE can successfully add the given KEY, VALUE pair.
 * Returns 0 if it can, else a negative error code indicating why it cannot. */
int kvstore_put_check(kvstore_t *store, char *key, char *value) {
  struct stat st;
  if (strlen(key) > MAX_KEYLEN)
    return ERR_KEYLEN;
  if (strlen(value) > MAX_VALLEN)
    return ERR_VALLEN;
  if (stat(store->dirname, &st) == -1)
    return ERR_FILACCESS;
  return 0;
}

/* Adds the given KEY, VALUE entry to STORE. Returns 0 if successful, else a
 * negative error code. See tpcfollower.h for a complete description of how
 * entries are stored. */
int kvstore_put(kvstore_t *store, char *key, char *value) {
  uint64_t hashval;
  int counter, check;
  size_t keylen = strlen(key), vallen = strlen(value);
  char filename[MAX_FILENAME];
  struct stat st;
  FILE *file;
  kventry_t *entry;
  if ((check = kvstore_put_check(store, key, value)) < 0)
    return check;
  hashval = strhash64(key);
  counter = find_entry(store, key, NULL);
  pthread_rwlock_wrlock(&store->lock);
  if (counter >= 0) {
    /* Entry already exists, just update it. */
    sprintf(filename, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter, KVSTORE_FILETYPE);
  } else {
    /* Search for the end of the hash chain to insert. */
    counter = 0;
    sprintf(filename, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter++, KVSTORE_FILETYPE);
    while (stat(filename, &st) != -1)
      sprintf(filename, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter++,
              KVSTORE_FILETYPE);
  }
  if ((file = fopen(filename, "w")) == NULL) {
    pthread_rwlock_unlock(&store->lock);
    return ERR_FILACCESS;
  }
  entry = malloc(sizeof(kventry_t) + keylen + vallen + 2);
  if (!entry)
    fatal_malloc();
  entry->length = keylen + vallen + 2;
  strcpy(entry->data, key);
  strcpy(entry->data + keylen + 1, value);
  fwrite(entry, sizeof(kventry_t) + entry->length, 1, file);
  fclose(file);
  pthread_rwlock_unlock(&store->lock);
  free(entry);
  return 0;
}

/* Checks if STORE can successfully remove the given KEY.
 * Returns 0 if it can, else a negative error code indicating why it cannot. */
int kvstore_del_check(kvstore_t *store, char *key) {
  struct stat st;
  if (strlen(key) > MAX_KEYLEN)
    return ERR_KEYLEN;
  if (stat(store->dirname, &st) == -1)
    return ERR_FILACCESS;
  if (!kvstore_haskey(store, key))
    return ERR_NOKEY;
  return 0;
}

/* Removes the given KEY entry from STORE. Returns 0 if successful, else a
 * negative error code. Any hash chains which are disrupted by the deletion of
 * KEY will be reconnected within this function. */
int kvstore_del(kvstore_t *store, char *key) {
  char delfile[MAX_FILENAME];
  int chainpos;
  uint64_t hashval;
  unsigned int counter;
  char currfile[MAX_FILENAME];
  struct stat st;
  chainpos = find_entry(store, key, NULL);
  if (chainpos < 0)
    return chainpos;
  counter = chainpos;
  hashval = strhash64(key);
  pthread_rwlock_wrlock(&store->lock);
  sprintf(delfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, chainpos, KVSTORE_FILETYPE);
  sprintf(currfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, ++counter, KVSTORE_FILETYPE);
  while (stat(currfile, &st) != -1) {
    sprintf(currfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, ++counter, KVSTORE_FILETYPE);
  }
  if (counter == chainpos + 1) {
    /* There were no elements in the chain after the element to be deleted. */
    if (remove(delfile) == -1) {
      pthread_rwlock_unlock(&store->lock);
      return errno;
    }
  } else {
    /* There were elements in the chain after the element to be deleted.
       Take the last element in the chain and swap it into the deletion
       location. */
    sprintf(currfile, "%s/%" PRIu64 "-%u%s", store->dirname, hashval, counter - 1,
            KVSTORE_FILETYPE);
    if (rename(currfile, delfile) == -1) {
      pthread_rwlock_unlock(&store->lock);
      return errno;
    }
  }
  pthread_rwlock_unlock(&store->lock);
  return 0;
}

/* Deletes all current entries in STORE and removes the store directory.
 * You will need to reinitialize STORE following this action to continue
 * using it. */
int kvstore_clean(kvstore_t *store) {
  struct dirent *dent;
  char filename[MAX_FILENAME];
  DIR *kvstoredir = opendir(store->dirname);
  if (kvstoredir == NULL)
    return 0;
  while ((dent = readdir(kvstoredir)) != NULL) {
    sprintf(filename, "%s/%s", store->dirname, dent->d_name);
    remove(filename);
  }
  closedir(kvstoredir);
  remove(store->dirname);
  return 0;
}
