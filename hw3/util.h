#pragma once
#include <stdio.h>
#include <unistd.h>
#include<sys/types.h>

extern const int BLOCK_SIZE;

typedef struct s_block *t_block;
//data meta
struct s_block {
    size_t size;
    t_block next;
    t_block prev;
    int free;
    int padding;
    void *ptr;
    char data[1];
};

extern t_block base;


t_block find_block(t_block* last, size_t s);
t_block extend_heap(t_block last, size_t s);
void split_block(t_block old, size_t s);
size_t align8(size_t s);

t_block get_block(void* p);
int valid_addr(void* p);
t_block fusion(t_block b);

void copy_block(t_block src, t_block dst);
