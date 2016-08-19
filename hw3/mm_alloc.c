/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include <stdlib.h>
#include "mm_alloc.h"

#include "util.h"

void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    t_block b, last;
    size_t s = align8(size);
    if (base) {
        last = base;
        b = find_block(&last, s);
        if (b) {
            if ((b->size - s) >= BLOCK_SIZE+8)
                split_block(b, s);
            b->free = 0;
        } else {
            b = extend_heap(last, s);
            if (!b) return NULL;
        }
    } else {
        b = extend_heap(NULL, s);
        if (!b) return NULL;
        base = b;
    }
    return b->data;
}

void *mm_calloc(size_t number, size_t size) {
    size_t *new_b;
    size_t s8, i;
    new_b = (size_t*)mm_malloc(number * size);
    if (new_b) {
        s8 = align8(number*size) >> 3;
        for (i = 0; i < s8; i++)
            new_b[i] = 0;
    }
    return new_b;
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    t_block b;
    if (valid_addr(ptr)) {
        b = get_block(ptr);
        b->free = 1;
        if (b->prev && b->prev->free)
            fusion(b->prev);
        if (b->next)
            fusion(b);
        else {
            if (b->prev)
                b->prev->next = NULL;
            else
                base = NULL;
            brk(b);
        }
    }
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    size_t s;
    t_block b, new_b;
    void *new_ptr;
    if (!ptr) return mm_malloc(size);

    if (valid_addr(ptr)) {
        s = align8(size);
        b = get_block(ptr);
        if (b->size >= s) {
            if (b->size - s >= (BLOCK_SIZE + 8))
                split_block(b, s);
        } else {
            if (b->next && b->next->free &&
                     (b->next->size + b->size + BLOCK_SIZE) >= s){
                fusion(b);
                if (b->size - s >= (BLOCK_SIZE + 8))
                    split_block(b, s);
            } else {
                new_ptr = mm_malloc(s);
                if (new_ptr) return NULL;
                new_b = get_block(new_ptr);
                copy_block(b, new_b);
                free(b);
                return new_ptr;
            }
        }
        return ptr;
    }
    return NULL;
}
