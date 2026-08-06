#define _GNU_SOURCE 1
#include <sys/resource.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
//#include <stdatomic.h>
#include <unistd.h>
#include <assert.h>
#include <libpmem.h>
#include "generic_alloc.h"
#include "generic_sync.h"

#include "pmalloc.h"

#ifdef NO_PMO_HW
	#include "pmo.h"
#else
	#include <pmo.h>
#endif

#define IMMEDIATE_BLOCK
#define PSYNC_NUM 1
#define TIMING

#ifdef TIMING
unsigned long long int attach_time = 0,
	      detach_time, psync_time, total_cycle_times = 0,
	      times_invoked = 0;
#endif
Pool *pools[100];
int npools = 0;
int num_psyncs = 0;
//unsigned long long int num_psyncs_counter = 0;
Pool *pcache = NULL;
pthread_mutex_t master_lock;
size_t POOL_SIZE;

void dump_thread_times(int tid)
{
	struct rusage usage;
	getrusage(RUSAGE_THREAD, &usage);
	printf("---------------------------------------\n");
	printf("THREAD %lld INFORMATION BELOW...\n", tid);
	printf("---------------------------------------\n");
	printf("Userspace time: %lld\nKernelspace time: %lld\n",
			usage.ru_utime.tv_sec*1e6 + usage.ru_utime.tv_usec,
			usage.ru_stime.tv_sec*1e6 + usage.ru_stime.tv_usec);
	printf("Maximum resident set sized used %lldKB\nMinor pagefaults: %lld\nMajor pagefaults: %lld\n",
			usage.ru_maxrss, usage.ru_minflt, usage.ru_majflt);

	printf("File system input operations:%lld\nFile system output operations: %lld\n",
			usage.ru_inblock, usage.ru_oublock);

	printf("Context switches by voluntary release of processor (due to blocking): %lld\nContext switch by preemeption: %lld\n",
			usage.ru_nvcsw, usage.ru_nivcsw);
	printf("---------------------------------------\n");
	printf("THREAD %lld INFORMATION ENDS......\n", tid);
	printf("---------------------------------------\n");


}
void dump_times(struct timeval diff1)
{
#ifdef TIMING
	struct rusage usage;
	size_t usertime, kerneltime, othertime, faulttime, totaltime;
	getrusage(RUSAGE_SELF, &usage);
	usertime = usage.ru_utime.tv_sec*1e6 + usage.ru_utime.tv_usec,
	kerneltime = usage.ru_stime.tv_sec*1e6 + usage.ru_stime.tv_usec,
	totaltime = diff1.tv_sec*1e6 + diff1.tv_usec,
	othertime = totaltime - (attach_time + detach_time + psync_time);
	//othertime = totaltime - usertime - (attach_time + detach_time + psync_time);

//	other_time = kerneltime - (attach_time + detach_time + psync_time);

	/* RUSAGE_SELF, RUSAGE_CHILDREN, RUSAGE_LWP */
	printf("---------------------------------------\n");
	printf("PROCESS INFORMATION BELOW...\n");
	printf("---------------------------------------\n");

	printf("Attach time: %lld\nDetach time: %lld\nPsync time: %lld\nOther time: %lld\n \nTotal time: %lld\n",
			attach_time, detach_time, psync_time, othertime,totaltime);

	printf("Userspace time: %lld\nKernelspace time: %lld\n",
			usertime, kerneltime);
	/*
	printf("Maximum resident set sized used %lldKB\nMinor pagefaults: %lld\nMajor pagefaults: %lld\n",
			usage.ru_maxrss, usage.ru_minflt, usage.ru_majflt);

	printf("File system input operations:%lld\nFile system output operations: %lld\n",
			usage.ru_inblock, usage.ru_oublock);

	printf("Context switches by voluntary release of processor (due to blocking): %lld\nContext switch by preemeption: %lld\n",
			usage.ru_nvcsw, usage.ru_nivcsw);
			*/
	printf("---------------------------------------\n");
	printf("PROCESS INFORMATION ENDS......\n");
	printf("---------------------------------------\n");
	printf("Total times attach/psync/detach called...%lld\n", total_cycle_times);
	printf("Total times psync alone was called....%lld\n", times_invoked);

#endif
	return;
}

char* make_pmo_name() {
	char* name = (char*)malloc(10);
	sprintf(name, "PMO%d", getpid());
	return name;
}

static char* make_pool_name(int npools) {
	char* name = (char*)malloc(10);
	sprintf(name, "PMO%d", npools);
	return name;
}

void pmalloc_init() {
    pthread_mutex_init(&master_lock, NULL);
}

void p_init()
{
    printf("init\n");
    char* pmo_name = make_pmo_name();
    /*
    if(!pmo_exists(pmo_name)) {
	    printf("Creating a PMO of size %lld\n", sizeof(Pool) + POOL_SIZE);
	    pmo_create(pmo_name, sizeof(Pool) + POOL_SIZE, PMO_KEY);
    }
    */

#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif
    

    pcache = alloc_memory(sizeof(Pool) + POOL_SIZE, pmo_name);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    attach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif

    pcache->size = POOL_SIZE - sizeof(Pool);
    pcache->start = (void*)((char*)pcache + sizeof(Pool));

    pthread_mutex_init(&pcache->lock, NULL);

    // Initial Block
    pcache->free_blocks = pcache->start;
    pcache->free_blocks->is_free = 1;
    pcache->free_blocks->size = POOL_SIZE - sizeof(Block);
    pcache->free_blocks->next = NULL;

    #if DO_PSYNC
#ifdef TIMING
    gettimeofday(&tick, NULL);
#endif
    // Persists changes to pool
    psync(pcache);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif


    #endif
}

void* p_malloc(size_t size) {
    //printf("%ld", size);
    if(pcache == NULL){
        //printf("pool = nULL\n");
	printf("POOL SIZE: %lld\n", POOL_SIZE);
        p_init(POOL_SIZE);
    }
    pthread_mutex_lock(&pcache->lock);
    // Find a free block that can accommodate the requested size.
    Block *previous = NULL;
    Block *current = pcache->free_blocks;
    while (current && (current->is_free == 0 || current->size < size)) {
        previous = current;
        current = current->next;
    }

    if (!current) {
        // There's no free block that can accommodate the requested size.
        return NULL;
    }

    // If the chosen block is larger than necessary, split it.
    if (current->size > size + sizeof(Block)) {
        Block *new_block = (void *)((char *)current + sizeof(Block) + size);
        new_block->is_free = 1;
        new_block->size = current->size - sizeof(Block) - size;
        new_block->next = current->next;

        current->size = size;
        current->next = new_block;
    }

    current->is_free = 0;

    // Remove the block from the free list.
    if (previous) {
        previous->next = current->next;
    } else {
        pcache->free_blocks = current->next;
    }


    #if DO_PSYNC
    // Persists changes to pool
#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif


    psync_nochecksum(pcache);
  //  printf("Psync pcache\n");

#ifdef TIMING
    gettimeofday(&tock, NULL);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
    #endif

    pthread_mutex_unlock(&pcache->lock);
    // Return the location of the data.
    return (char *)current + sizeof(Block);
}

//#define DETACH 1
void p_persist(){
    pthread_mutex_lock(&pcache->lock);
#ifdef TIMING
    struct timeval tick, tock;
    struct timespec psync_tick, psync_tock;
   gettimeofday(&tick, NULL);
   // clock_gettime(CLOCK_MONOTONIC, &psync_tick);
#endif
    
    psync(pcache);


#ifdef TIMING
    gettimeofday(&tock, NULL);
//	clock_gettime(CLOCK_MONOTONIC, &psync_tock);
    //psync_time +=// ((psync_tock.tv_sec  + psync_tock.tv_sec)*1e9 - (psync_tick.tv_nsec + psync_tick.tv_nsec));
//	    (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
//    printf("persisting stops. Psync time is %lld\n", (psync_time));
#endif


    num_psyncs++;
   /* num_psyncs_counter++;
    #if DETACH
    if(num_psyncs >= DETACH) {
	    //printf("Num psyncs %lld >= %lld\n", num_psyncs, DETACH);
#ifdef TIMING
    gettimeofday(&tick, NULL);
#endif
    
    total_cycle_times++;
    	detach(pcache);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    detach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif


	char* name = make_pmo_name();

#ifdef TIMING
    gettimeofday(&tick, NULL);
#endif
    
    	pcache = attach(name, 'w', PMO_KEY);
#ifdef TIMING
    gettimeofday(&tock, NULL);
    attach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif

	num_psyncs = 0;
    }
    //printf("%s", make_pmo_name());
    #endif
    */

    pthread_mutex_unlock(&pcache->lock);
}

void p_free(void *ptr) {
    pthread_mutex_lock(&pcache->lock);
    Block *block = (Block *)((char *)ptr - sizeof(Block));
    block->is_free = 1;

    #ifdef IMMEDIATE_BLOCK 
    // Find a free block that can accommodate the requested size.
    Block *previous = NULL;
    Block *current = pcache->free_blocks;
    while (current && (char*)current < (char*)block) {
        previous = current;
        current = current->next;
    }

    if(!previous){
        // TODO: handle
        block->next = current;
        pcache->free_blocks = block;
        #if DO_PSYNC
        // Persists changes to pool
#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif

        psync(pcache);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    //psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
        #endif
        pthread_mutex_unlock(&pcache->lock);
        return;
    }

    previous->next = block;
    block->next = current;

    //Merges next block if adjecent to free'd block;
    if((char*)block + sizeof(Block) + block->size == (char*)current){
        block->next = current->next;
        block->size += sizeof(Block) + block->size;
    }

    // Merges previous block if adjecent to free'd block;
    if((char*)previous + sizeof(Block) + previous->size == (char*)block){
        previous->next = current;
        previous->size += sizeof(Block) + block->size;
        block = previous;
    }
    #endif

    
    #ifdef FULL_BLOCK
    // Add the block to the start of the free list.
    block->next = pcache->free_blocks;
    pcache->free_blocks = block;

    // Merge adjacent free blocks to prevent fragmentation.
    // TODO: better defrag
    Block *current = pcache->free_blocks;
    while (current && current->next) {
        // Check if two blocks are adjacent
        if ((char*)current + sizeof(Block) + current->size == (char*)current->next) {
            // Merge blocks
            current->size += sizeof(Block) + current->next->size;
            current->next = current->next->next;
        }
        else {
            current = current->next;
        }
    }
    #endif

    #if DO_PSYNC
    // Persists changes to pool
#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif

    psync(pcache);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    //psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
    #endif

    pthread_mutex_unlock(&pcache->lock);
}

void p_destroy(){
    pthread_mutex_destroy(&pcache->lock);
    #if DO_PSYNC
    // Persists changes to pool
 #ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif
    printf("DETACH\n");
    sync_memory(pcache, POOL_SIZE + sizeof(Pool));

#ifdef TIMING
    gettimeofday(&tock, NULL);
    //psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
    printf("Destroy thread %lld\n", getpid());
    #endif

#ifdef TIMING
    gettimeofday(&tick, NULL);
#endif
    
#ifdef SHOULD_PSYNC_BEFORE_DETACH
    sync_memory(pcache, POOL_SIZE + sizeof(Pool));
#endif
    dealloc_memory(pcache, POOL_SIZE + sizeof(Pool)); //detach(pcache);

#ifdef TIMING
    gettimeofday(&tock, NULL);
    detach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif


}

Pool* p_pool_create(void *addr, size_t size, int zeroed){
    pthread_mutex_lock(&master_lock);

    char* pmo_name = make_pool_name(npools);
    if(!pmo_exists(pmo_name))
	    pmo_create(pmo_name, sizeof(Pool) + POOL_SIZE, PMO_KEY);
    Pool* new_pool = (Pool*)addr;


#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif
    
    new_pool = attach(pmo_name, 'w', PMO_KEY);
#ifdef TIMING
    gettimeofday(&tock, NULL);
    attach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif

    if (!zeroed)
		memset(new_pool, 0, sizeof (Pool));

    new_pool->pool_id = npools;
    new_pool->size = POOL_SIZE;
    new_pool->start = (void*)((char*)pcache + sizeof(Pool));

    free(pmo_name);
    pthread_mutex_init(&new_pool->lock, NULL);

    // Initial Block
    new_pool->free_blocks = pcache->start;
    new_pool->free_blocks->is_free = 1;
    new_pool->free_blocks->size = POOL_SIZE - sizeof(Block);
    new_pool->free_blocks->next = NULL;

    
    pools[npools] = new_pool;
    npools++;

    pthread_mutex_unlock(&master_lock);

    #if DO_PSYNC
    // Persists changes to pool
#ifdef TIMING
    gettimeofday(&tick, NULL);
#endif

    sync_memory(new_pool, POOL_SIZE + sizeof(Pool));

#ifdef TIMING
    gettimeofday(&tock, NULL);
    //psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
    #endif
    return new_pool;
}

int p_pool_delete(Pool *pool){
    pthread_mutex_lock(&master_lock);
    int id;
    for(id = 0; id < npools; id++){
        if ((pools[id])->pool_id == pool->pool_id)
            break;
    }

#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif

#ifdef SHOULD_PSYNC_BEFORE_DETACH
	sync_memory(pool, POOL_SIZE + sizeof(Pool));

#endif

    dealloc_memory(pool, POOL_SIZE + sizeof(Pool));

#ifdef TIMING
    gettimeofday(&tock, NULL);
    detach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif



    pools[id] = NULL;
    npools--;

    pthread_mutex_unlock(&master_lock);
    return 0;
}

void* p_pool_malloc(Pool *pool, size_t size) {
    
    pthread_mutex_lock(&pool->lock);
    // Find a free block that can accommodate the requested size.
    Block *previous = NULL;
    Block *current = pool->free_blocks;
    while (current && (current->is_free == 0 || current->size < size)) {
        previous = current;
        current = current->next;
    }

    if (!current) {
        // There's no free block that can accommodate the requested size.
        return NULL;
    }

    // If the chosen block is larger than necessary, split it.
    if (current->size > size + sizeof(Block)) {
        Block *new_block = (void *)((char *)current + sizeof(Block) + size);
        new_block->is_free = 1;
        new_block->size = current->size - sizeof(Block) - size;
        new_block->next = current->next;

        current->size = size;
        current->next = new_block;
    }

    current->is_free = 0;

    // Remove the block from the free list.
    if (previous) {
        previous->next = current->next;
    } else {
        pool->free_blocks = current->next;
    }


    #if DO_PSYNC
#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif

    // Persists changes to pool
	sync_memory(pool, POOL_SIZE + sizeof(Pool));

#ifdef TIMING
    gettimeofday(&tock, NULL);
    //psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif

    #endif


    pthread_mutex_unlock(&pool->lock);
    // Return the location of the data.
    return (char *)current + sizeof(Block);
}

void p_pool_free(Pool *pool, void *ptr) {
    pthread_mutex_lock(&pool->lock);
    Block *block = (Block *)((char *)ptr - sizeof(Block));
    block->is_free = 1;

    // Add the block to the start of the free list.
    block->next = pool->free_blocks;
    pool->free_blocks = block;

    // Merge adjacent free blocks to prevent fragmentation.
    // TODO: This only works in sorted blocks
    Block *current = pool->free_blocks;
    while (current && current->next) {
        // Check if two blocks are adjacent
        if ((char*)current + sizeof(Block) + current->size == (char*)current->next) {
            // Merge blocks
            current->size += sizeof(Block) + current->next->size;
            current->next = current->next->next;
        }
        else {
            current = current->next;
        }
    }
    

    #if DO_PSYNC
    // Persists changes to pool
#ifdef TIMING
    struct timeval tick, tock;
    gettimeofday(&tick, NULL);
#endif


    sync_memory(pool, POOL_SIZE + sizeof(Pool));

#ifdef TIMING
    gettimeofday(&tock, NULL);
    psync_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
#endif
#endif



    pthread_mutex_unlock(&pool->lock);
}

// Defragment 
void p_defrag(){
    pthread_mutex_lock(&master_lock);
    pcache->free_blocks = pcache->start;
    pcache->free_blocks->is_free = 1;
    pcache->free_blocks->next = NULL;
    pcache->free_blocks->size = POOL_SIZE - sizeof(Block);
    pthread_mutex_unlock(&master_lock);
}

unsigned long p_sizeof(void *ptr){
    //pthread_mutex_lock(&master_pool->lock);
    Block* current = (Block*)((char*)ptr - sizeof(Block));
    //pthread_mutex_unlock(&master_pool->lock);
    return current->size;
}
