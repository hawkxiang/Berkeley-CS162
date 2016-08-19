#include "util.h"
const int BLOCK_SIZE = 40;
t_block base = NULL;

t_block find_block(t_block *last, size_t s) {
    t_block b = base;
    while (b && !(b->free && b->size >= s)) {
        *last = b;
        b = b->next;
    }
    return b;
}

t_block extend_heap(t_block last, size_t s) {
    t_block brk = (t_block)sbrk(0);
    if (sbrk(BLOCK_SIZE + s) == (void*)-1)
        return NULL;
    brk->size = s;
    brk->next = NULL;
    if (last) 
        last->next = brk;
    brk->prev = last;
    brk->free = 0;
    brk->ptr = brk->data;
    return brk;
}

void split_block(t_block b, size_t s) {
    t_block new_b = (t_block)(b->data + s);
    new_b->size = b->size - BLOCK_SIZE - s;
    new_b->next = b->next;
    new_b->prev = b;
    new_b->free = 1;
    new_b->ptr = new_b->data;
    b->size = s;
    b->next = new_b;
    if (new_b->next)
        new_b->next->prev = new_b;
    
}

size_t align8(size_t s) {
    if ((s & 0x7) == 0) return s;
    return ((s >> 3) + 1) << 3;
}

t_block get_block(void* p) {
    char *tmp = (char*)p;
    p = (tmp -= BLOCK_SIZE);
    return (t_block)p;
}

int valid_addr(void *p) {
    if (base) {
        if (p > (void*)base && p < sbrk(0)){
            void *tmp = get_block(p)->ptr;
            return p == tmp;
        }
    }
    return 0;
}

t_block fusion(t_block b) {
    if (b->next && b->next->free) {
        b->size += b->next->size + BLOCK_SIZE;
        b->next = b->next->next;
        if (b->next)
            b->next->prev = b;
    }
    return b;
}

void copy_block(t_block src, t_block dst) {
    size_t *sdata, *ddata, i;
    sdata = (size_t*)src->ptr;
    ddata = (size_t*)dst->ptr;
    for (i = 0; (i*8) < src->size && (i*8) < dst->size; i++)
        ddata[i] = sdata[i];
}
