#include <stdlib.h>
#include "wq.h"
#include "kvconstants.h"
#include "utlist.h"

/* Initializes a work queue WQ. */
void wq_init(wq_t *wq) {
  pthread_mutex_init(&wq->mutex, NULL);
  pthread_cond_init(&wq->condvar, NULL);
  wq->size = 0;
  wq->head = NULL;
}

/* Remove an item from the WQ. This function will wait until the queue
 * contains at least one item, then remove that item from the list and
 * return it. */
void *wq_pop(wq_t *wq) {
  void *job;
  wq_item_t *wq_item;

  pthread_mutex_lock(&wq->mutex);
  while (wq->size == 0)
    pthread_cond_wait(&wq->condvar, &wq->mutex);
  wq_item = wq->head;
  job = wq->head->item;
  wq->size--;
  DL_DELETE(wq->head, wq->head);
  pthread_mutex_unlock(&wq->mutex);

  free(wq_item);
  return job;
}

/* Add ITEM to WQ. */
void wq_push(wq_t *wq, void *item) {
  pthread_mutex_lock(&wq->mutex);
  wq_item_t *wq_item = calloc(1, sizeof(wq_item_t));
  wq_item->item = item;
  DL_APPEND(wq->head, wq_item);
  wq->size++;
  pthread_cond_broadcast(&wq->condvar);
  pthread_mutex_unlock(&wq->mutex);
}
