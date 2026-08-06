#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <math.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include "pmalloc.h"
#include <time.h>

#include "psync.h"

#define     DEBUG           0
#define     PRINT           0
#define     NUM_BARRIERS    (2*NUM_CP+1)

int RATIO;
unsigned long long int n, m;
//const   unsigned long long int n = N;             //num of columns of matrix a
//const   unsigned long long int m = M;
const   int factor = m/2;
//alignas(64) int   a[n][n], c[n][n];  // the two matrices a and b. c is the resultant matrix
alignas(64) int **a, **c;
//alignas(64) int   h[m][m];
alignas(64) int **h;
//alignas(64) int   checkpointA[n][n], checkpointC[n][n];
uint64_t start, end;

/*
pthread_mutex_t SyncLock[NUM_BARRIERS];   /* mutex *
pthread_cond_t  SyncCV[NUM_BARRIERS];     /* condition variable *
int             SyncCount[NUM_BARRIERS];  /* number of processors at the barrier so far *
*/

inline uint64_t rdtsc()
{
    unsigned long a, d;
    asm volatile ("cpuid; rdtsc" : "=a" (a), "=d" (d) : : "ebx", "ecx");
    return a | ((uint64_t)d << 32);
}

/*
void Barrier(int idx)
{
    int ret;

    pthread_mutex_lock(&SyncLock[idx]); /* Get the thread lock *
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

*/
void InitH() 
{
    int i, j;

    srand(0);
    for (i=0; i<m; i++) {
        for (j=0; j<m; j++) 
            h[i][j] = rand()%3;
    }
}

void InitA() 
{
    int i, j;

    srand(0);
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) 
            a[i][j] = rand()%20;
    }
}

void PrintC() 
{
    int i, j;

    printf("The A matrix:\n");
    for (i=0; i< n; i++) {
        for (j=0; j< n; j++) 
            printf("%d\t",c[i][j]); 
        printf("\n");
    }
    printf("\n");
}

void PrintH() 
{
    int i, j;

    printf("The H matrix:\n");
    for (i=0; i< m; i++) {
        for (j=0; j< m; j++) 
            printf("%d\t",h[i][j]); 
        printf("\n");
    }
    printf("\n");
}

void PrintA() 
{
    int i, j;

    printf("The A matrix:\n");
    for (i=0; i< n; i++) {
        for (j=0; j< n; j++) 
            printf("%d\t",a[i][j]); 
        printf("\n");
    }
    printf("\n");
}

struct proc_info 
{
	int tid, *sum;
};

void* conv(void* tmp) 
{
    /* each thread has a private version of local variables */
	struct proc_info *proc_inf = (struct proc_info *)tmp;
    int tid = proc_inf->tid; 
    int firstLoop, lastLoop;
    int i, j, k, kk, r, l;                //iterators
    int sum  = *proc_inf->sum;
    int counter = 0;
    int CPcount = 1;
    struct timeval start_time, end_time, diff_time,timestamp, old_timestamp,difftime;

    firstLoop = tid*(n/P);
    lastLoop = firstLoop + (n/P);

    // i is the index of the c matrix row
    // j is the index of the c matrix col
    // r is the index of the kernel row
    // l is the index of the kernel col
    /********************** The gaussian operation code *********************/

    bool synced = false;
    gettimeofday(&start_time, NULL);
    gettimeofday(&old_timestamp, NULL);
    int q;
    for (i=firstLoop; i<lastLoop; i++) {  //i terates over columns
      for (j=0; j<n; j++) {  //i terates over columns
          sum = 0;
          for(r=0; r<m; r++) {
              for(l=0; l<m; l++) {
                  if( ((i-factor+r)>=0) && ((j-factor+l)>=0) && ((i-factor+r)<n) && ((j-factor+l)<n) ) {
                      sum += h[r][l]*a[i-factor+r][j-factor+l];
                  }
              } //end of l
	          } //end of r
          c[i][j] = sum;

	 if(!synced && !tid && i % RATIO == 0) {
			  should_psync = true;
			  synced = true;
  		}
	  check_for_psync(tid, i, lastLoop);

      } //end of j
     	synced = false;

    } //end of for i

  
    gettimeofday(&end_time, NULL);
	  timersub(&end_time, &start_time, &diff_time);
	  printf("%s, %ld.%ld secs\n", "Thread", diff_time.tv_sec, diff_time.tv_usec);
	  fflush(stdout);
  
    psync_thread_finishing();
  
	  return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t*     threads;
    pthread_attr_t attr;
    int            ret, dx;
    int i, j, k, jj, kk;                    //iterators
    struct timeval begin1, end1, diff1;
    gettimeofday(&begin1, NULL);

/*
    for(i=0; i<n; i++) {
        for(j=0; j<n; j++) 
            a[i][j] = (i*n)+j+1;
    }
    h[0][0] = h[0][2] = 1;
    h[0][1] = 2;
    h[1][0] = h[1][1] = h[1][2] = 0;
    h[2][0] = -1;
    h[2][1] = -2;
    h[2][2] = -1;
*/
		// Construct 2d Arrays a, h, and c using PMOs
	if (argc > 1)
		m = atoi(argv[1]);
	else
		m = 256;

	if (argc > 2)
		n = atoi(argv[2]);
	else
		n=1024;

	if (argc > 3)
		RATIO = atoi (argv[3]);
	else
		RATIO=16;

      POOL_SIZE = sizeof(int*)*n*3 + sizeof(Pool) + sizeof(int)*n*n+ 
	      sizeof(int)*m*m + (/*56*//*36*/5*33554432); ////134217728; //268435456; //536870912;  
//	printf("POOL_SIZE should be %lld\n", P_would);
		a = (int**) p_malloc(sizeof(int*) * n);
		h = (int**) p_malloc(sizeof(int*) * m);
		c = (int**) p_malloc(sizeof(int*) * n);

		for (int l = 0; l < n; l++) {
			a[l] = (int*) p_malloc(sizeof(int) * n);
			c[l] = (int*) p_malloc(sizeof(int) * n);
		}

		for (int l = 0; l < m; l++) {
			h[l] = (int*) p_malloc(sizeof(int) * m);
		}

		// Initialize arrays
    InitA();
    InitH();

    //printf("checkpointing interval: %d\n",cpInterval);

#if PRINT
    PrintA();
    PrintH();
#endif

    /* Initialize array of thread structures */
    set_psync_thread_count(P);
    threads = (pthread_t *) malloc(sizeof(pthread_t) * P);
    assert(threads != NULL);

    /* Initialize thread attribute */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM); // sys manages contention

    //    printf("Initializing %d Barriers\n", NUM_BARRIERS);
    /*
    for(i=0; i<NUM_BARRIERS; i++) {
        /* Initialize mutexs *
        ret = pthread_mutex_init(&SyncLock[i], NULL);
        assert(ret == 0);

        /* Init condition variable *
        ret = pthread_cond_init(&SyncCV[i], NULL);
        assert(ret == 0);
        SyncCount[i] = 0;
    }
    */

    start = rdtsc();
    
    /*
    std::cout << "psync rate: " << PSYNC_RATE_USEC << "\n";
    */
    auto psync_thread = spawn_psync_thread();//PSYNC_RATE_USEC);
    
    struct proc_info proc_inf[P];
    for(dx=0; dx < P; dx++) {
	    proc_inf[dx].tid = dx;
	    proc_inf[dx].sum = (int*)p_malloc(sizeof (int));
        ret = pthread_create(&threads[dx], &attr, conv, (void*)&proc_inf[dx]);
        assert(ret == 0);
    }

    /* Weit for each ofothe threads to terminate */
    for(dx=0; dx < P; dx++) {
        ret = pthread_join(threads[dx], NULL);
        assert(ret == 0);
    }
    psync_thread.join();
    p_persist();
    
    gettimeofday(&end1, NULL);
    timersub(&end1, &begin1, &diff1);
    std::cout << "Total Time = " << diff1.tv_sec << "." <<  diff1.tv_usec << " secs\n";

    end = rdtsc();
    printf("%ld ticks\n", end - start);


#if PRINT
    printf("Final results\n");
    PrintC();
#endif

		p_destroy();
	dump_times(diff1);
    return 0;
}


