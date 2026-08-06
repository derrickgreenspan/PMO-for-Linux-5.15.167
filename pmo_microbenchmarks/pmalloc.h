#ifndef POOL_ALLOCATOR_H
#define POOL_ALLOCATOR_H

#ifdef __cplusplus
extern "C"{
#endif
//#define NO_PMO_HW

#include <stddef.h>
//#include <stdatomic.h>
#include <libpmem.h>
#include <pmo.h>
#include <pthread.h>

typedef struct Block {
    // 1 means it is free and 0 means it is allocated
    int is_free;

    // Size of the memory block
    size_t size;

    // Points to the next free block when in the free list
    struct Block *next;
} Block;

typedef struct Pool {
    // Number of threads using this pool
    int *nthreads;

    int pool_id;
    
    // Free list of memory blocks
    Block *free_blocks;
    
    // Ptr to start of usable memory
    void* start;

    // Size of memory managed by the pool
    size_t size;

    // Pool Lock
    pthread_mutex_t lock;
} Pool;

void dump_times(struct timeval diff1);
extern size_t POOL_SIZE;
#define GB 1000000000
//#define POOL_SIZE ((size_t)1*GB)
//#define POOL_SIZE 67223632
#define PMO_KEY "KEY"
#define DO_PSYNC 1


// Function to initialize the memory pool
//__attribute__((constructor)) void p_init();

//
void p_destroy();

// Force memory to be persistent
void p_persist();

// Function to allocate memory from the pool
void *p_malloc(size_t size);

// Function to allocate memory from the pool
void *p_calloc(size_t nmemb, size_t size);

// Function to allocate memory from the pool
void *p_realloc(void *ptr, size_t size);

// Function to deallocate memory back to the pool
void p_free(void *ptr);

Pool* p_pool_create(void *addr, size_t size, int zeroed);

int p_pool_delete(Pool *pool);

void* p_pool_malloc(Pool *pool, size_t size);

void p_pool_free(Pool *pool, void *ptr);

// 
void p_defrag();

// Returns the size of the allocated memory
unsigned long p_sizeof(void *ptr);

char *make_pmo_name(void);
extern Pool *pcache;
// Function to deinitialize and free the pool
//void pool_deinit(Pool *pool);
#ifdef __cplusplus
}
#endif
#endif //POOL_ALLOCATOR_H
