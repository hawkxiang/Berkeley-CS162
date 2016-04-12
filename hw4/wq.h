#ifndef __WQ__
#define __WQ__

#include <pthread.h>

/* WQ defines a work queue which will be used to store jobs which are waiting to
 * be processed. */

typedef struct wq_item {
  /* The item which is being stored. */
  void *item;
  /* The next item in the queue. */
  struct wq_item *next;
  /* The previous item in the queue. */
  struct wq_item *prev;
} wq_item_t;

typedef struct wq {
  int size;
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  wq_item_t *head;
} wq_t;

void wq_init(wq_t *wq);

void wq_push(wq_t *wq, void *item);

void *wq_pop(wq_t *wq);

#endif
