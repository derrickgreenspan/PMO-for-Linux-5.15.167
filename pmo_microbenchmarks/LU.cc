#include <iostream>
#include <cstdio>
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


#define     GEM5        0
#define     MOH_PRINT   0
#define     PRINT       0 
//#define     P           8 /* 16 threads */
//#define     checkpointLimit   512 
#define     N          7168//6072//5632 //(5120) 
//#define RATIO		32
//#define     N		4096

int checkpointLimit = 1; // 512;
int n, RATIO;
const int p = P;
//alignas(64) long  a[n][n], cp[n][n];
long **a, **cp;
alignas(64) int lastjj[P*16], lastkk, lastkkLog, lastjjLog[P];
bool updatingIndex;
uint64_t start, end;
int numberOfCheckpoints = 0;

pthread_mutex_t SyncLock[N/16];   /* mutex */
pthread_cond_t  SyncCV[N/16];     /* condition variable */
int             SyncCount[N/16];  /* number of processors at the barrier so far */

pthread_mutex_t   SyncLock2[N/16]; /* mutex */
pthread_cond_t    SyncCV2[N/16]; /* condition variable */
int               SyncCount2[N/16]; /* number of processors at the barrier so far */

pthread_mutex_t SyncLock3[N/16];   /* mutex */
pthread_cond_t  SyncCV3[N/16];     /* condition variable */
int             SyncCount3[N/16];  /* number of processors at the barrier so far */

pthread_mutex_t SyncLock4[N/16];   /* mutex */
pthread_cond_t  SyncCV4[N/16];     /* condition variable */
int             SyncCount4[N/16];  /* number of processors at the barrier so far */


inline uint64_t rdtsc()
{
    unsigned long a, d;
    asm volatile ("cpuid; rdtsc" : "=a" (a), "=d" (d) : : "ebx", "ecx");
    return a | ((uint64_t)d << 32);
}



void Barrier(int i) {
    int ret;

    pthread_mutex_lock(&SyncLock[i]); /* Get the thread lock */
    SyncCount[i]++;
    if(SyncCount[i] == p) {
        ret = pthread_cond_broadcast(&SyncCV[i]);
        assert(ret == 0);
    } else {
        ret = pthread_cond_wait(&SyncCV[i], &SyncLock[i]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock[i]);
}

void Barrier2(int i){
    int ret;

    pthread_mutex_lock(&SyncLock2[i]); /* Get the thread lock */
    SyncCount2[i]++;
    if(SyncCount2[i] == P) {
        ret = pthread_cond_broadcast(&SyncCV2[i]);
        assert(ret == 0);
    } else {
        ret = pthread_cond_wait(&SyncCV2[i], &SyncLock2[i]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock2[i]);
}

void Barrier3(int i){
    int ret;

    pthread_mutex_lock(&SyncLock3[i]); /* Get the thread lock */
    SyncCount3[i]++;
    if(SyncCount3[i] == P) {
        ret = pthread_cond_broadcast(&SyncCV3[i]);
        assert(ret == 0);
    } else {
        ret = pthread_cond_wait(&SyncCV3[i], &SyncLock3[i]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock3[i]);
}

void Barrier4(int i) {
    int ret;

    pthread_mutex_lock(&SyncLock4[i]); /* Get the thread lock */
    SyncCount4[i]++;
    if(SyncCount4[i] == p) {
        ret = pthread_cond_broadcast(&SyncCV4[i]);
        assert(ret == 0);
    } else {
        ret = pthread_cond_wait(&SyncCV4[i], &SyncLock4[i]); 
        assert(ret == 0);
    }
    pthread_mutex_unlock(&SyncLock4[i]);
}

void printA(){
    int i, j;
    std::cout << "A matrix:" << std::endl;
    for(i=0; i<n; i++){
        for(j=0; j<n; j++){ 
            if(roundf(a[i][j]) == a[i][j] ) printf("%d\t", (int)a[i][j]);
            else printf("%0.3f\t", a[i][j]);
        }
        printf("\n");
    }
}

void* lu(void* tmp) {
    int tid = (uintptr_t) tmp; 
    int i, j, k, kk, jj, ii, temp;
    long sum;
    bool synced = false;
    int counter = 0;


    for(kk=0; kk<1024; kk+=16) {
        // update the perimeter columns and rows
        if(tid == 0) {
            for(k=kk; k<(kk+16); k++) { 
                for(i=k+1; i<n; i++) { //iterates over the rows under the digonal element
                    // 1) scale column k below diagonal by 1/a(k,k)
                    a[i][k] /= a[k][k];
                    if(k < (kk+16-1) ) {
                        if(i<(kk+16) ) {
                            for(j=k+1; j<n; j++) //iterate over the column elements in the current i row
                                a[i][j] -= a[i][k]*a[k][j];
                        } else {
                            for(j=k+1; j<(kk+16); j++) //iterate over the column elements in the current i row
                                a[i][j] -= a[i][k]*a[k][j];
                        }
                    }
                } //end of for i
            } // end of for k
    	//	check_for_psync(tid, kk, 1024);
	  //  p_persist();
        }
        //Barrier(kk/16);

        //updating the trailing blocks in the trailing matrix
        for(jj=(kk+16); jj<n; jj+=16) {
            for(ii=(kk+(tid+1)*16); ii<n; ii+=(P*16)) {
                for(j=jj; j<(jj+16); j++) {
                    for(i=ii; i<(ii+16); i++) {
                        sum = 0;
                        for(k=kk; k<(kk+16); k++)
                            sum += a[j][k]*a[k][i];

                        a[j][i] = a[j][i]-sum;
                    } //end of j
			if(!synced && !tid && counter % RATIO == 0) {
				should_psync = true;
				synced = true;
			}
		    	check_for_psync(tid, kk, 1024);
			counter++;
	        	} //end of i

	            } //end of ii
        } //end of jj       
	synced = false;

    } // end of for kk


#if MOH_PRINT
    printf("before barrier3, tid=%d, kk=%d\n", tid, kk);
#endif
    //Barrier3(kk/16);
#if MOH_PRINT
    printf("exiting kk loop, tid=%d, kk=%d\n", tid, kk);
#endif

    psync_thread_finishing();
		return NULL;
}

int main(int argc, char **argv) {
	struct timeval tick, tock, tick_tock_diff;
    int i, j;
    gettimeofday(&tick, NULL);
    pthread_t*     threads;
    pthread_attr_t attr;
    int ret, dx;
    if(argc > 1)
       n  = atoi(argv[1]);

    if (argc > 2)
	    RATIO = atoi(argv[2]);
    else
	    RATIO = 32;

    printf ("RATIO is %ld\n", RATIO);
    srand (0); //time(NULL));
    POOL_SIZE = sizeof(long *)*n*3 + sizeof(long)*n*n*4 + sizeof(Pool) + 2*2097152;// 64*524288; //1048576; //2097152; //4194304; //8388608;//33554432; //67108864; // 134217728;

		// Construct 2d Arrays a and cp using PMOs
		a = (long**) p_malloc(sizeof(long*) * 2*n);
		cp = (long**) p_malloc(sizeof(long*) * 2*n);

		for (int l = 0; l < n; l++) {
			a[l] = (long*) p_malloc(sizeof(long) * 2*n);
			cp[l] = (long*) p_malloc(sizeof(long) * 2*n);
		}

    /* Matrices initialization */
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            a[i][j] = rand()%99;
            cp[i][j] = 0;
        }
    }
		p_persist();

    /* reset indices variables */
    for(i=0; i<P; i++) lastjj[i] = -1;

    // pivot the matrix
    //    pivot();
    for(i=0; i<(N/16); i++) {
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock[i], NULL);
        assert(ret == 0);
        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV[i], NULL);
        assert(ret == 0);
        SyncCount[i] = 0;
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock2[i], NULL);
        assert(ret == 0);
        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV2[i], NULL);
        assert(ret == 0);
        SyncCount2[i] = 0;
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock3[i], NULL);
        assert(ret == 0);
        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV3[i], NULL);
        assert(ret == 0);
        SyncCount3[i] = 0;
        /* Initialize mutexs */
        ret = pthread_mutex_init(&SyncLock4[i], NULL);
        assert(ret == 0);
        /* Init condition variable */
        ret = pthread_cond_init(&SyncCV4[i], NULL);
        assert(ret == 0);
        SyncCount4[i] = 0;
    }

    /* Initialize array of thread structures */
    threads = (pthread_t *) malloc(sizeof(pthread_t) * p);
    assert(threads != NULL);
    /* Initialize thread attribute */
    set_psync_thread_count(P);
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); // sys manages contention

    /* Main operation udner Stats */
#if GEM5
    m5_dumpreset_stats(0,0);
#else
    start = rdtsc();
#endif

    auto psync_thread = spawn_psync_thread();
    for(dx=0; dx < p; dx++) {
        ret = pthread_create(&threads[dx], &attr, lu, (void*)(uintptr_t)dx);
        assert(ret == 0);
    }
    /* Wait for each of the threads to terminate */
    for(dx=0; dx < p; dx++) {
        ret = pthread_join(threads[dx], NULL);
        assert(ret == 0);
    }
	psync_thread.join();
	p_persist();

#if GEM5
    m5_dumpreset_stats(0,0);
    m5_exit(0);
#else
    end = rdtsc();
    printf("%ld ticks\n", end - start);
#endif

#if PRINT
    printA();
#endif

//	p_destroy();
		gettimeofday(&tock, NULL);
	timersub(&tock, &tick, &tick_tock_diff);
	dump_times(tick_tock_diff);
    return 0;
}


