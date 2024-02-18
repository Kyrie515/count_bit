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
    int group_count;
    int timestamp;
    struct Bucket* next;
    struct Bucket* prev;
    struct Bucket* group_head;
    struct Bucket* group_tail;
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

Bucket* malloc_bucket(Memory_Pool* pool) {
    for (int i = 0; i < pool -> size; i++) {
        if (pool->bucket_pool[i].used == false) {
            pool->bucket_pool[i].used = true;
            pool->bucket_pool[i].count = 0;
            pool->bucket_pool[i].group_count = 0;
            pool->bucket_pool[i].timestamp = 0;
            pool->bucket_pool[i].next = NULL;
            pool->bucket_pool[i].prev = NULL;
            pool->bucket_pool[i].group_head = NULL;
            pool->bucket_pool[i].group_tail = NULL;
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
    int time;
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
    self -> time = 0;
    self -> head = NULL;
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
    self -> time = 0;
    self -> wnd_size = 0;
    free(self -> pool);
}


void wnd_bit_count_apx_print(StateApx* self) {
    // This is useful for debugging.
    /*
     * print a bucket-shaped structure, including the time stamp and the count
     * like below:
     * {timestamp1, count1} -> {timestamp2, count2} -> ... -> {timestampN, countN}
     *
     */
    Bucket *current = self->head;
    while (current != NULL) {
        printf("{%lu, %d}", current->timestamp, current->count);
        if (current->next != NULL) {
            printf(" -> ");
        }
        current = current->next;
    }
    printf("\n");
}

/*
 * Traverse the list of buckets, for every k + 2 group of buckets
 * merge the oldest two into a new bucket and double the count
 */
void add_bucket_to_group(Bucket* bucket, Bucket* group_head) {
    bucket -> next = group_head;
    if (group_head != NULL) {
        group_head -> prev = bucket;
        bucket -> group_count = group_head -> group_count + 1;
        bucket -> group_tail = group_head -> group_tail;
        group_head -> group_count = 0;
        group_head -> group_tail = NULL;
    } else {
        bucket -> group_count = 1;
        bucket -> group_tail = bucket;
    }
    Bucket* group_tail = bucket -> group_tail;
    group_tail -> group_head = bucket;
}

void merge_buckets(StateApx* self, Bucket* current) {
    while (current->group_count > self->k + 1) {
        Bucket *group_tail = current->group_tail;
        Bucket *new_head = group_tail->prev;

        current->group_tail = new_head->prev;
        current->group_count -= 2;

        new_head->count *= 2;

        Bucket *prev_head = group_tail->next;
        add_bucket_to_group(new_head, prev_head);
        free_bucket(group_tail);
        N_MERGES++;
        current = new_head;
    }
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    // TODO: Fill me.
    if (item) {
        Bucket *new_bucket = malloc_bucket(self -> pool);
        new_bucket -> timestamp = self -> time;
        new_bucket -> count = 1;
        new_bucket -> prev = NULL;
        add_bucket_to_group(new_bucket, self -> head);
        self -> head = new_bucket;

        Bucket *current = new_bucket;
        merge_buckets(self, current);
    }
    wnd_bit_count_apx_print(self);
    int count = 0;
    int time_min = self->time - self->wnd_size + 1;
    Bucket *current = self->head;
    while (current != NULL) {
        Bucket *next = current->next;
        if (next != NULL && next->timestamp <= time_min) {
            count++;

            if (next->group_head != NULL) {
                Bucket *group_head = next->group_head;
                Bucket *group_tail = next->prev;
                group_head->group_count--;
                group_head->group_tail = group_tail;
                group_tail->group_head = group_head;
            }
            current->next = NULL;
            free_bucket(next);
            break;
        }
        else {
             count += current->count;
        }
        current = next;
    }
    self->time++;
    return count;
}

#endif // _WINDOW_BIT_COUNT_APX_
