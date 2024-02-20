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
    int current;
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
   pool->current = 0;
    for (int i = 0; i < size; i++) {
         pool->bucket_pool[i].used = false;
    }
   return size * sizeof(Bucket);
}

/*
 * malloc_bucket allocates a bucket from the memory pool
 * pool: the memory pool
 * returns: a bucket
 *
 * In this function, the current pointer is used to keep track of the next available bucket,
 * in most cases, the current pointer indeed points to the next available bucket, but in some cases,
 * the next available bucket is not the next one in the array, so we need to loop through the array
 * using this pointer,we get the available bucket in O(1) in amortized time complexity.
 */
Bucket* malloc_bucket(Memory_Pool* pool) {
    for (int i = 0; i < pool -> size; i++) {
        if (!pool->bucket_pool[pool->current].used) {
            pool->bucket_pool[pool->current].used = true;
            pool->bucket_pool[pool->current].count = 0;
            pool->bucket_pool[pool->current].group_count = 0;
            pool->bucket_pool[pool->current].timestamp = 0;
            pool->bucket_pool[pool->current].next = NULL;
            pool->bucket_pool[pool->current].prev = NULL;
            pool->bucket_pool[pool->current].group_head = NULL;
            pool->bucket_pool[pool->current].group_tail = NULL;
            return &pool->bucket_pool[pool->current];
        }
        pool->current = (pool->current + 1) % pool->size;
    }
    printf("Memory pool is full\n");
    return NULL;
}

/*
 * free_bucket frees a bucket from the memory pool
 * bucket: the bucket to free
 *
 * we just need to set the used flag to false,
 * so that the bucket can be used again

 */
void free_bucket(Bucket* bucket) {
    bucket->used = false;
}

typedef struct {
    // TODO: Fill me.
    u_int32_t wnd_size;
    u_int64_t k;
    Bucket *head;
    Bucket *tail;
    int time;
    Memory_Pool *pool;
    int prev_count;
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
    self -> time = -1;
    self -> head = NULL;
    self -> tail = NULL;
    self -> prev_count = 0;
//    int mem_size = init_memory_pool(self -> pool, wnd_size);
//    // TODO:
//    // The function should return the total number of bytes allocated on the heap.
//    return mem_size + sizeof(Memory_Pool);
    int memory_size;
    // calculate the size of the memory pool
    if (wnd_size <= k + 1)
    {
        memory_size = wnd_size + 1;
    }
    else
    {
        int n = ceil(log2((double)wnd_size / (double)(k + 1) + 1) - 1);
        memory_size = (n + 1) * (k + 1) + 1;
    }
    return init_memory_pool(self->pool, memory_size) + sizeof(Memory_Pool);


}

void destroy_memory_pool(Memory_Pool *pool)
{
    free(pool->bucket_pool);
    pool -> bucket_pool = NULL;
    pool->current = 0;
    pool->size = 0;
}

void wnd_bit_count_apx_destruct(StateApx* self) {
    // TODO: Fill me.
    // Make sure you free the memory allocated on the heap.
    self -> head = NULL;
    self -> tail = NULL;
    self -> k = 0;
    self -> time = 0;
    self -> wnd_size = 0;
    self -> prev_count = 0;
    destroy_memory_pool(self -> pool);
}

/*
     * print a bucket-shaped structure, including the time stamp and the count
     * like below:
     * {timestamp1, count1} -> {timestamp2, count2} -> ... -> {timestampN, countN}
     *
     */
void wnd_bit_count_apx_print(StateApx* self) {
    // This is useful for debugging.
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
    //printf("Added bucket to group\n");
}

bool merge_buckets(StateApx* self, Bucket* current) {
    bool is_merged = false;
    while (current->group_count > self->k + 1) {
        is_merged = true;
        Bucket *group_tail = current->group_tail;
        Bucket *new_head_next = group_tail->prev;
        Bucket *new_tail_cur = new_head_next->prev;

        new_tail_cur->group_head = current;
        current->group_tail = new_tail_cur;
        current->group_count -= 2;

        new_head_next->count *= 2;

        Bucket *prev_head = group_tail->next;
        add_bucket_to_group(new_head_next, prev_head);
        if (self -> tail == group_tail) {
            self -> tail = new_head_next;
        }
        free_bucket(group_tail);
        N_MERGES++;
        current = new_head_next;
    }
    //printf("Merged buckets\n");
    return is_merged;
}

bool check_remove_tail(StateApx* self, Bucket* tail, int min_time) {
    bool is_removed = false;
    if (tail != NULL && tail->timestamp <= min_time) {
        is_removed = true;
        Bucket *group_head = tail->group_head;
        if (group_head == tail) {
            Bucket *prev_group_tail = tail->prev;
            if (prev_group_tail != NULL) {
                prev_group_tail->next = NULL;
                // readjust the tail
                self->tail = prev_group_tail;
            }
            else {
                self->head = NULL;
                self->tail = NULL;
            }
        }
        else {
            Bucket *new_tail = tail->prev;
            group_head->group_count--;
            group_head->group_tail = new_tail;
            new_tail->group_head = group_head;
            new_tail->next = NULL;
            self->tail = new_tail;
        }
        free_bucket(tail);
    }
    //printf("Removed tail\n");
    return is_removed;
}

int count_bits(StateApx* self, Bucket* current) {
    int count = 0;
    while (current != NULL) {
        Bucket *current_group_tail = current->group_tail;
        if (current_group_tail != self->tail) {
            count += current->count * current->group_count;
            current = current_group_tail->next;
        }
        else {
            count += current->count * current->group_count;
            count -= current->count;
            count++;
            break;
        }
    }
    //printf("Counted bits\n");
    return count;
}

/*
 * wnd_bit_count_apx_next updates the count of the bits in the window
 * self: the state of the algorithm
 * item: the next item in the stream
 * returns: the count of the bits in the window
 *
 * In this function, we add a new bucket to the head of the list, then we check
 * the buckets in the same group to see if I need to merge them, then we merge the last two
 * buckets in the group. We keep track of the size of the group, because that helps us recognize
 * when we need to merge the last two buckets in the same group.
 */
uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    // TODO: Fill me.
    self -> time++;
    bool is_merged = false;
    bool is_removed = false;
    if (item) {
        Bucket *new_bucket = malloc_bucket(self->pool);
        new_bucket->timestamp = self->time;
        new_bucket->count = 1;
        new_bucket->prev = NULL;
        add_bucket_to_group(new_bucket, self->head);
        self->head = new_bucket;
        if (self->tail == NULL) {
            self->tail = new_bucket;
        }
        Bucket *current = new_bucket;
        is_merged = merge_buckets(self, current);
    }
    //wnd_bit_count_apx_print(self);
    int min_time = self->time - self->wnd_size + 1;

    Bucket *tail = self->tail;
    is_removed = check_remove_tail(self, tail, min_time);

    if (! is_merged && ! is_removed) {
        if (item) {
            self->prev_count++;
        }
        return self->prev_count;
    }
    Bucket *current = self->head;
    self->prev_count = count_bits(self, current);
    //printf("Done with next\n");
    return self->prev_count;
}

#endif // _WINDOW_BIT_COUNT_APX_
