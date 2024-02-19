#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "queue.h"

uint64_t N_MERGES = 0; // keep track of how many bucket merges occur

typedef struct
{
  int time_stamp;

  // size of this bucket
  int size;
  // the pointer to next bigger bucket
  struct Bucket *next;
  // the pointer to the previous smaller bucket
  struct Bucket *prev;

  // the pointer to the last bucket in the group
  struct Bucket *group_tail;
  // the pointer to the first bucket in the group
  struct Bucket *group_head;

  // the number of buckets in the group
  int group_count;

  // used in our memory pool
  // if the bucket is in use, we cannot free it
  bool in_use;
} Bucket;

typedef struct
{
  // the real pool
  Bucket *data;
  // to find the next available memory
  int current;
  // the size of the pool
  int size;
} MemoryPool;

uint64_t init_memory_pool(MemoryPool *self, int size)
{
  self->data = (Bucket *)malloc(size * sizeof(Bucket));
  self->current = 0;
  self->size = size;
  for (int i = 0; i < size; i++)
  {
    self->data[i].in_use = false;
  }
  return size * sizeof(Bucket);
}

Bucket *malloc_memory(MemoryPool *self)
{
  for (int i = 0; i < self->size; i++)
  {
    if (!self->data[self->current].in_use)
    {
      // reset the bucket to
      self->data[self->current].next = NULL;
      self->data[self->current].prev = NULL;
      self->data[self->current].group_tail = NULL;
      self->data[self->current].group_head = NULL;
      self->data[self->current].group_count = 0;
      self->data[self->current].size = 0;
      self->data[self->current].time_stamp = 0;
      self->data[self->current].in_use = true;

      return &self->data[self->current];
    }
    self->current = (self->current + 1) % self->size;
  }

  printf("Memory pool is full\n");
  return NULL;
}

// we simply mark the bucket as not in use
void free_memory(MemoryPool *self, Bucket *bucket)
{
  bucket->in_use = false;
}

void destroy_memory_pool(MemoryPool *self)
{
  free(self->data);
  self->data = NULL;
  self->current = 0;
  self->size = 0;
}

typedef struct
{
  uint32_t wnd_size;
  uint32_t k;

  // the current time
  int time;

  // the place to store the first bucket
  Bucket *head;
  // the place to store the last bucket, to clean up the memory
  Bucket *tail;
  // the memory pool
  MemoryPool memory_pool;
} StateApx;

void print_bucket(Bucket *bucket)
{
  printf("|\n");
  if (bucket->in_use == false)
  {
    printf("MEMORY LEAK!!! THIS BUCKET IS FREED\n");
  }
  printf("---Bucket: time %d, size %d---\n", bucket->time_stamp, bucket->size);
  if (bucket->group_head != NULL)
  {
    Bucket *group_head = bucket->group_head;
    printf("---Group head: time %d, size %d\n", group_head->time_stamp, group_head->size);
  }
  if (bucket->group_tail != NULL)
  {
    Bucket *group_tail = bucket->group_tail;
    printf("---Group tail: time %d, size %d\n", group_tail->time_stamp, group_tail->size);
  }
}

void wnd_bit_count_apx_print(StateApx *self)
{
  // This is useful for debugging.
  printf("StateApx:\n");
  Bucket *current = self->head;
  while (current != NULL)
  {
    print_bucket(current);
    current = current->next;
  }

  printf("k: %d\n", self->k);
  printf("wnd_size: %d\n", self->wnd_size);
  printf("time: %d\n", self->time);
  if (self->head != NULL)
  {
    printf("Head:\n");
    print_bucket(self->head);
  }
  if (self->tail != NULL)
  {
    printf("Tail:\n");
    print_bucket(self->tail);
  }
}

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx *self, uint32_t wnd_size, uint32_t k)
{
  assert(wnd_size >= 1);
  assert(k >= 1);

  self->wnd_size = wnd_size;
  self->k = k;
  self->time = 0;
  self->head = NULL;
  // self->tail = NULL;

  // The function should return the total number of bytes allocated on the heap
  // in the most severe case, n + 1 is enough, where n buckets hold 1 bits, and a new bucket is added
  return init_memory_pool(&self->memory_pool, wnd_size + 1);
}

void wnd_bit_count_apx_destruct(StateApx *self)
{
  // Make sure you free the memory allocated on the heap.
  destroy_memory_pool(&self->memory_pool);
  self->head = NULL;
  self->tail = NULL;
  self->k = 0;
  self->wnd_size = 0;
  self->time = 0;
}

// add the bucket to become the new head of the group
void add_bucket_to_group(Bucket *bucket, Bucket *group_head)
{
  bucket->next = group_head;
  if (group_head != NULL)
  {
    group_head->prev = bucket;

    bucket->group_count = group_head->group_count + 1;
    bucket->group_tail = group_head->group_tail;

    // only let group head take the information of the group
    group_head->group_count = 0;
    group_head->group_tail = NULL;
  }
  else
  {
    bucket->group_count = 1;
    bucket->group_tail = bucket;
  }

  Bucket *group_tail = bucket->group_tail;
  group_tail->group_head = bucket;
}

uint32_t wnd_bit_count_apx_next(StateApx *self, bool item)
{
  if (item)
  {
    // add a new bucket
    Bucket *new_bucket = malloc_memory(&self->memory_pool);
    new_bucket->time_stamp = self->time;
    new_bucket->size = 1;
    new_bucket->prev = NULL;

    add_bucket_to_group(new_bucket, self->head);
    self->head = new_bucket;

    // potentially add the tail
    if (self->tail == NULL)
    {
      self->tail = new_bucket;
    }

    // collapse the buckets
    Bucket *current = new_bucket;

    while (current->group_count > self->k + 1)
    {

      Bucket *group_tail = current->group_tail;

      // we will merge the last two buckets
      // now it is the head of the next group
      Bucket *new_head = group_tail->prev;

      // new tail of the last group if the previous of the new head
      // which is the kth in the group
      current->group_tail = new_head->prev;
      // the group count is reduced by 2, since we merge 2 buckets away
      current->group_count -= 2;

      new_head->size *= 2;

      Bucket *prev_head = group_tail->next;
      add_bucket_to_group(new_head, prev_head);

      if (self->tail == group_tail)
      {
        // the tail of the group is the tail of the list
        // we need to update the tail
        self->tail = new_head;
      }

      // the last bucket in the group can be freed
      free_memory(&self->memory_pool, group_tail);
      N_MERGES++;
      current = new_head;
    }
  }

  int time_min = self->time - self->wnd_size + 1;

  // we can first check if the tail can be removed
  Bucket *tail = self->tail;
  if (tail != NULL && tail->time_stamp <= time_min)
  {
    printf("Removing tail\n");
    // count += tail->size;
    // self->tail = tail->prev;
    // free_memory(&self->memory_pool, tail);
    Bucket *group_head = tail->group_head;
    if (group_head == tail)
    {
      // this means the tail is the only bucket in the group

      Bucket *prev_group_tail = tail->prev;
      if (prev_group_tail != NULL)
      {
        // now disconnect the previous group from this node
        prev_group_tail->next = NULL;
        // readjust the tail
        self->tail = prev_group_tail;
      }
      else
      {
        // this means the tail is the only bucket left
        // this mean our data structure is empty
        self->head = NULL;
        self->tail = NULL;
      }
    }
    else
    {
      printf("Removing tail from group\n");
      print_bucket(tail);
      // simpler case, just remove the tail
      Bucket *new_tail = tail->prev;

      // the group count is reduced by 1
      group_head->group_count--;
      // rebind the group
      group_head->group_tail = new_tail;
      new_tail->group_head = group_head;
      new_tail->next = NULL;

      self->tail = new_tail;
    }

    // say goodbye to the tail anyway
    free_memory(&self->memory_pool, tail);
  }

  wnd_bit_count_apx_print(self);

  int count = 0;
  // the head shall always be the first group head
  Bucket *current = self->head;

  while (current != NULL)
  {
    // now we simply add the size of the group times the group count
    // for the last group, we change the last count into 1
    Bucket *current_group_tail = current->group_tail;
    if (current_group_tail != self->tail)
    {
      count += current->size * current->group_count;
      current = current_group_tail->next;
    }
    else
    {
      count += current->size * current->group_count;
      count -= current->size;
      count++;
      break;
    }
  }

  self->time++;

  return count;

  // the idea is go to every group head, and add the size of the group
  // we need to treat the last group differently, we may potentially have to kill the tail

  // while (current != NULL)
  // {
  //   Bucket *group_tail = current->group_tail;
  //   if (group_head != NULL)
  //   {
  //     count += group_head->size;
  //     current = group_head->next;
  //   }
  //   else
  //   {
  //     count += current->size;
  //     current = current->next;
  //   }
  // }

  // while (current != NULL)
  // {

  //   // if the next time stamp is greater than the minimum time stamp
  //   // we can only add 1 to the count, then break
  //   // else we can add the full size
  //   Bucket *next = current->next;
  //   if (next != NULL && next->time_stamp <= time_min)
  //   {
  //     // we can only add 1 to the count
  //     count++;

  //     // this shall always happen
  //     if (next->group_head != NULL)
  //     {
  //       Bucket *group_head = next->group_head;
  //       Bucket *group_tail = next->prev;

  //       // the group count is reduced by 1
  //       group_head->group_count--;
  //       // rebind the group
  //       group_head->group_tail = group_tail;
  //       group_tail->group_head = group_head;
  //     }

  //     // say goodbye to the next bucket
  //     current->next = NULL;
  //     free_memory(&self->memory_pool, next);

  //     break;
  //   }
  //   else
  //   {
  //     count += current->size;
  //   }
  //   current = next;
  // }

  // self->time++;

  // return count;
}

#endif // _WINDOW_BIT_COUNT_APX_
