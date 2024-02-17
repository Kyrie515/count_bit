#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

uint64_t N_MERGES = 0; // keep track of how many bucket merges occur

/*
    TODO: You can add code here.
*/
typedef struct Bucket {
    int count;
    unsigned long timestamp;
    struct Bucket* next;
    stuuct Bucket* prev;
    bool used;
}Bucket;

/*
 * Memory_Pool is a struct that holds the memory pool for the buckets
 * size: the size of the memory pool
 * bucket_pool: the memory pool
 */
typedef struct Memory_Pool {
    int size;
    Bucket* bucket_pool;
}Memory_Pool;

/*
 * init_memory_pool initializes the memory pool
 * pool: the memory pool
 * size: the size of the memory pool
 * returns: the size of the memory pool
 */
int init_memory_pool(Memory_Pool *pool, int size) {
   pool->size = size;
   pool->bucket_pool = (Bucket*)malloc(size * sizeof(Bucket));
   return size * sizeof(Bucket);
}

Bucket* new_bucket(Memory_Pool* pool) {
    for (int i = 0; i < pool -> size; i++) {
        if (pool->bucket_pool[i].used == false) {
            pool->bucket_pool[i].used = true;
            pool->current++;
            return &pool->bucket_pool[i];
        }
    }
    printf("Memory pool is full\n");
    return NULL;
}

void free_bucket(Bucket* bucket) {
    bucket->used = false;
}

typedef struct {
    // TODO: Fill me.
    u_int32_t wnd_size;
    u_int64_t k;
    Bucket *head;
    Memory_Pool *pool;
} StateApx;

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx* self, uint32_t wnd_size, uint32_t k) {
    assert(wnd_size >= 1);
    assert(k >= 1);

    // TODO: Fill me.
    self -> wnd_size = wnd_size;
    self -> k = k;
    self -> pool = (Memory_Pool*)malloc(sizeof(Memory_Pool));
    int mem_size = init_memory_pool(self -> pool, wnd_size);
    // TODO:
    // The function should return the total number of bytes allocated on the heap.
    return mem_size + sizeof(Memory_Pool);
}

void wnd_bit_count_apx_destruct(StateApx* self) {
    // TODO: Fill me.
    // Make sure you free the memory allocated on the heap.
    self -> head = NULL;
    self -> k = 0;
    self -> wnd_size = 0;
    free(self -> pool);
}

void wnd_bit_count_apx_print(StateApx* self) {
    // This is useful for debugging.
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    // TODO: Fill me.
    return 0;
}

#endif // _WINDOW_BIT_COUNT_APX_
