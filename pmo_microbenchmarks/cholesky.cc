#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <math.h>
#include "pmalloc.h"
#include "psync.h"

#define     PRINT   0
#define     Moh_PRINT   0
#define     Benchmark_mode      3 //0: Base, 1: Recompute, 2: ABFT: Lazy Persistency, 3:Checkpointing
//#define     N              4096
#define	    LIMIT	(256)
#define     NUM_BARRIERS   (2*LIMIT+1) //(4*N/P) 

pthread_mutex_t SyncLock[NUM_BARRIERS];   /* mutex */
pthread_cond_t  SyncCV[NUM_BARRIERS];     /* condition variable */
int             SyncCount[NUM_BARRIERS];  /* number of processors at the barrier so far */
int CP_Limit = 1;
int checkpoints = 0;
//const int n = 512;             //num of columns of matrix a
//const int n = N;             //num of columns of matrix a
unsigned int n;
#if Benchmark_mode == 1      
int lastCompleted_i[P];
int lastCompleted_j[P];
int lastCompletedILog[P];
int lastCompletedJLog[P];
int startedIndicesLogging[P];
#endif
#if Benchmark_mode == 2
alignas(64) unsigned int HashTable[n * P];
#endif
//int a[n][n];  //The two matrices a and b. c is the resultant matrix
int **a;
int PSYNC_RATIO;
//int Checkpoint_a[n][n];  //The two matrices a and b. c is the resultant matrix

void Barrier(int idx) {
    int ret;

    pthread_mutex_lock(&SyncLock[idx]); /* Get the thread lock */
    SyncCount[idx]++;
    if(SyncCount[idx] == P) {
        ret = pthread_cond_broadcast(&SyncCV[idx]);
        assert(ret == 0);
	SyncCount[idx] = 0; 
    } else {
        ret = pthread_cond_wait(&SyncCV[idx], &SyncLock[idx]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock[idx]);
}
void* cholesky(void* tmp) {
    /* each thread has a private version of local variables */
    int threadId = (uintptr_t) tmp; 
    int ret;
    int i, j, k, jj, kk, r, l;                    //iterators
    int sum;                    //sum of multiplication for the active cell
    int firstLoop, lastLoop;
    unsigned int localCheckSum;
    int counter = 0;
    bool synced = false;
    firstLoop = threadId*(n/P);
    lastLoop = firstLoop + (n/P);
    for (j=firstLoop; j<lastLoop; j++) {
            for (i=j; i<n; i++) {
                sum = a[i][j];
                for (k=j-1; k>=0; k--)  {
                    sum -= a[i][k] * a[j][k];
		}

    		
                if (i==j) 
			a[i][j] = sqrt(sum)+1;
                else 
			a[i][j] = sum/(a[j][j]+1);

		if (!synced && !threadId && i % PSYNC_RATIO == 0) {
			should_psync = true;
			synced = true;
		}
	    	check_for_psync(threadId, j, lastLoop);
                
            	//Barrier(1+counter);
		            //	counter++;
            }

	    check_for_psync(threadId, j, lastLoop);
	    synced = false;
    	}
    
    	psync_thread_finishing();
    	Barrier(2*LIMIT);

	return NULL;
}

int main(int argc, char ** argv)
{
	struct timeval end1, begin1, diff1;
	
    struct timeval userspace_start, userspace_end;
    pthread_t*     threads;
    pthread_attr_t attr;
    int            ret, dx;
    int i, j, k;                //iterators
    int sum;                    //sum of multiplication for the active cell
    clock_t tic = clock(), toc;
    n = argc > 1 ? atoi(argv[1]) : 512;
    PSYNC_RATE_ITER = argc > 2 ? atoi(argv[2]) : 8;
    PSYNC_RATIO = argc > 3 ? atoi(argv[3]) : 8;

    gettimeofday(&begin1, NULL);
    POOL_SIZE = sizeof(int*)*n + sizeof(Pool) + sizeof(int) * n*n + 262144; 

    a = (int**) p_malloc(sizeof(int*) * n);
	for (int l = 0; l < n; l++) {
		a[l] = (int*) p_malloc(sizeof(int) * n);
	}
#if Benchmark_mode == 1
    for(i=0; i<P; i++) {
        lastCompleted_i[i] = -1;
        lastCompleted_j[i] = -1;
    }
#endif
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++){
            if(i==j) a[i][j] = 9;
            else a[i][j] = (i+j)%7;
        }
    }

    
    /* Initialize array of thread structures */
    threads = (pthread_t *) malloc(sizeof(pthread_t) * P);
    assert(threads != NULL);

    /* Initialize thread attribute */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); // sys manages contention



    for(i=0; i<NUM_BARRIERS; i++) {
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock[i], NULL);
        assert(ret == 0);

        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV[i], NULL);
        assert(ret == 0);
        SyncCount[i] = 0;
    }


    //start = rdtsc();
    set_psync_thread_count(P);
    auto psync_thread = spawn_psync_thread();

    for(dx=0; dx < P; dx++) {
        ret = pthread_create(&threads[dx], &attr, cholesky, (void*)(uintptr_t)dx);
        assert(ret == 0);
    }

    /* Weit for each ofothe threads to terminate */
    for(dx=0; dx < P; dx++) {
        ret = pthread_join(threads[dx], NULL);
        assert(ret == 0);
    }
    psync_thread.join();
    printf("All threads complete\n");

    //p_persist();

 //   end = rdtsc();
//    printf("%ld ticks, checkpoints %d\n", end - start, checkpoints);

    printf("number of checkpoints = %d\n",checkpoints);

    toc = clock();
#if PRINT
    for(i=0; i<n; i++){
        for(j=0; j<n; j++) 
            printf("%d ", a[i][j]);
        printf("\n");
    }
#endif
        printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

	gettimeofday(&end1, NULL);
	timersub(&end1, &begin1, &diff1);


		p_destroy();
		dump_times(diff1);
    return 0;
}

