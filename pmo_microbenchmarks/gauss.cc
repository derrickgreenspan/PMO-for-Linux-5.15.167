#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include "pmalloc.h"
#include "psync.h"

#define     DEBUG           0
#define     PRINT           0
//#define     P               16 /* 16 threads */
//#define     N               8*1024//32*1024 //8192
#define     NUM_CP          1
#define     LIMIT           (256)//(1*1)
#define     NUM_BARRIERS    (2*LIMIT+1)
//#define	    PSYNC_RATIO 16// How often to call psync

int n;             //num of columns of matrix a
int CP_Limit = 2;
int NumberOfCheckpoints= 0,
    PSYNC_RATIO;
//alignas(64) float   a[n][n];  // the two matrices a and b. c is the resultant matrix
alignas(64) float **a;
uint64_t start, end;

pthread_mutex_t SyncLock[NUM_BARRIERS];   /* mutex */
pthread_cond_t  SyncCV[NUM_BARRIERS];     /* condition variable */
int             SyncCount[NUM_BARRIERS];  /* number of processors at the barrier so far */

inline uint64_t rdtsc()
{
    unsigned long a, d;
    asm volatile ("cpuid; rdtsc" : "=a" (a), "=d" (d) : : "ebx", "ecx");
    return a | ((uint64_t)d << 32);
}

void Barrier(int idx) {
    int ret;

    pthread_mutex_lock(&SyncLock[idx]); /* Get the thread lock */
    SyncCount[idx]++;
    if(SyncCount[idx] == P) {
        ret = pthread_cond_broadcast(&SyncCV[idx]);
        assert(ret == 0);
    } else {
        ret = pthread_cond_wait(&SyncCV[idx], &SyncLock[idx]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock[idx]);
}

void InitA() 
{
    int i, j;

//    srand48(0);
	srand(0);
	float rand_val;
    for (i=0; i< n; i++) {
        for (j=0; j< n; j++)  {
		//drand48()+0.1;
		rand_val = 0.5f; //static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                a[i][j] = rand_val+0.1;
	}
    }
}

void PrintA() 
{
    int i, j;

    printf("The A matrix:\n");
    for (i=0; i< n; i++) {
        for (j=0; j< n; j++) 
            printf("%0.2f\t",a[i][j]); 
        printf("\n");
    }
    printf("\n");
}

void* gauss(void* tmp) 
{
    /* each thread has a private version of local variables */
    int tid = (uintptr_t) tmp; 
    int firstLoop, lastLoop;
    int i, j, k, kk, l, r;                //iterators
    float factor;
    int     initial = 0;
    bool synced = false;

    /********************** The gaussian operation code *********************/
    for (i=0; i<LIMIT; i++) {  //i terates over columns
        /* Make all rows below this one 0 in current column
        The way we do it mutithreaded is that we assign each thread 16 rows.
        Each thread makes 16 rows 0 then it fetches the next 16 rows. 
        k is the iterator of the first row of the group of 16 */
        for (k=((i+1)+(tid*16)); k<n; k+=(P*16)) {
            /* kk iterates over the 16 rows designated for this thread
            starting from the index of the lead row pointed to by k */
            for(kk=k; (kk<(k+16)) && (kk<n);  kk++) {
                factor = a[kk][i]/a[i][i];
                for (j=i; j<n; j++) {
                    a[kk][j] -= factor*a[i][j];
                }
            if(!synced && !tid && i % PSYNC_RATIO /*4/PSYNC_RATIO*/ == 0) {
                    should_psync = true;
                    synced = true;
            }
        	check_for_psync(tid, i, LIMIT);
            } //end of for kk
        } //end of for k
       synced = false;
    } //end of for i 

    psync_thread_finishing();
    Barrier(2*LIMIT);
		return NULL;
}

int main(int argc, char ** argv)
{
	struct timeval begin1, end1, diff1;
	gettimeofday(&begin1, NULL);
    pthread_t*     threads;
    pthread_attr_t attr;
    int            ret, dx;
    int i, j, k, jj, kk;                    //iterators

    if(argc > 1)
        n = atoi(argv[1]);
    else
	    n = 8192;

	if (argc > 2)
		PSYNC_RATE_ITER = atoi(argv[2]);

	else
		PSYNC_RATE_ITER = 2;

    /*
    if (argc > 2)
	    PSYNC_RATIO = atoi(argv[2]);
    else
	    */
	if (argc > 3)
	    	PSYNC_RATIO = atoi(argv[3]);
	else
		PSYNC_RATIO=8;
		
    printf ("PSYNC ratio is %ld\nPSYNC rate iter is %ld\n", PSYNC_RATIO, PSYNC_RATE_ITER);
    POOL_SIZE = sizeof(float *) + sizeof(int)*n*n + sizeof(Pool)+ 2097152;//8388608;//524288; //134217728;
		// Construct 2d Array a using PMOs
		a = (float**) p_malloc(sizeof(float*) * n);

		for (int l = 0; l < n; l++) {
			a[l] = (float*) p_malloc(sizeof(int) * n);
		}

    InitA();
		p_persist();

#if PRINT
    PrintA();
#endif

    /* Initialize array of thread structures */
    set_psync_thread_count(P);
    threads = (pthread_t *) malloc(sizeof(pthread_t) * P);
    assert(threads != NULL);

    /* Initialize thread attribute */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); // sys manages contention

    //    printf("Initializing %d Barriers\n", NUM_BARRIERS);
    for(i=0; i<NUM_BARRIERS; i++) {
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock[i], NULL);
        assert(ret == 0);

        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV[i], NULL);
        assert(ret == 0);
        SyncCount[i] = 0;
    }

    start = rdtsc();

    auto psync_thread = spawn_psync_thread(); //PSYNC_RATE_USEC);
    for(dx=0; dx < P; dx++) {
        ret = pthread_create(&threads[dx], &attr, gauss, (void*)(uintptr_t)dx);
        assert(ret == 0);
    }

    /* Weit for each ofothe threads to terminate */
    for(dx=0; dx < P; dx++) {
        ret = pthread_join(threads[dx], NULL);
        assert(ret == 0);
    }

    psync_thread.join();
    p_persist();
    end = rdtsc();
    printf("%ld ticks\n", end - start);

    printf("number of Checkpoints = %d\n", NumberOfCheckpoints);
#if PRINT
    printf("Final results\n");
    PrintA();
#endif

    gettimeofday(&end1, NULL);
    timersub(&end1, &begin1, &diff1);
		p_destroy();
		dump_times(diff1);
    return 0;
}

