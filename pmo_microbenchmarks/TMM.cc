#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <omp.h>
#include <libpmem.h>
#include <pmo.h>
#include "pmalloc.h"
#include "psync.h"
#include <time.h>     
#include <sys/time.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <sstream>

#define NUM_ITER	1

/* This is the control panel of the benchmark */
#define     GEM5            0                   // Running on Gem5 or not
#define     DEBUG           0                   // Turn the debug code on or off
#define     PRINT           0                   // Print results or not
#define     MOHPRINT        0                   // Print results or not
//#define     PSYNC_RATIO	    32 			// How often to call psync
//#define     P               1                   // Threads
//#define     CHKP_LIMIT    3072  10000                   // Number of kk loops to do before taking the checkpoint
//#define     N               3072//1024 //4096                // Square matrix dimension N*N
//#define     TILE            16         // Tile size
#define decomp(i, j,q) ((q)*(i)+(j))

alignas(64) int *lastJJ;         // the last computed iteration of kk loop
alignas(64) int *lastII;           // the last computed iteration of i loop
alignas(64) int *lastKK;         // the last computed iteration of kk loop
alignas(64) int *lastJJLog;        // log for the last computed iteration of i loop
alignas(64) int *lastIILog;        // log for the last computed iteration of i loop
alignas(64) int *lastKKLog;       // log for the last computed iteration of kk loop
alignas(64) int *insideTxII;               // Flag for entering logging region for updating II index
alignas(64) int *insideTxKK;               // Flag for entering logging region for updating KK index

//const int n     = N;                            // matrix dimension
int N, n;
int p; //           = P;
int tile;//  = TILE;		                    // tile size
int firstTime;                    
int PSYNC_RATIO;
struct timespec *t1, *t2, *d;
alignas(64) float *a, *b, *c;    // the two matrices a and b. c is the resultant matrix
#ifdef CHKPT
char *chkptLog;
float *chkptA, *chkptB, *chkptC;
#endif
uint64_t start, end;                            // Used for printing the ticks on real system
std::vector<struct timeval> end_times;
std::mutex lock;

// PSYNC STUFF
/*
std::atomic_bool psync_thread_waiting = false; // Indicates to other threads they should stop processessing and wait for psync
std::atomic_int psync_waiting_ctr = 0;         // Counter used to indicate the number of threads waiting on the psync thread
std::condition_variable psync_thread_cv;       // Condition variable used by the psync thread to wait for worker threads to complete
std::condition_variable worker_thread_cv;      // Condition variable used by non-psync threads to wait for psync to complete
std::mutex psync_mutex;                        // Mutex used by threads
std::atomic_int psync_active_threads = 0;      // Counter used to indicate how many threads are actually running
*/

inline uint64_t rdtsc()
{
    unsigned long a, d;
    asm volatile ("cpuid; rdtsc" : "=a" (a), "=d" (d) : : "ebx", "ecx");
    return a | ((uint64_t)d << 32);
}

void  diff(struct timespec * difference, struct timespec start, struct timespec end)
{
    if ((end.tv_nsec-start.tv_nsec)<0) {
        difference->tv_sec = end.tv_sec-start.tv_sec-1;
        difference->tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        difference->tv_sec = end.tv_sec-start.tv_sec;
        difference->tv_nsec = end.tv_nsec-start.tv_nsec;
    }
}


void Initialize() 
{
    int i, j;

    #ifdef USE_PMO
	pmo_create((char *)"a", sizeof(float)*N*N, (char *) "DEFAULT\0");
	pmo_create((char *)"b", sizeof(float)*N*N, (char *) "DEFAULT\0");
	pmo_create((char *)"c", sizeof(float)*N*N, (char *) "DEFAULT\0");

	a = (float *) attach((char *)"a", 'W', (char *) "DEFAULT\0");
	b = (float *) attach((char *)"b", 'W', (char *) "DEFAULT\0");
	c = (float *) attach((char *)"c", 'W', (char *) "DEFAULT\0");
    #elif defined(USE_PMALLOC)
    printf("Creating a PMO of size %lld\n", N*N*3*sizeof(float));
	printf("N is %lld\n", N*N*sizeof(float));
    a = (float *) p_malloc(sizeof(float)*N*N);
	b = (float *) p_malloc(sizeof(float)*N*N);
	c = (float *) p_malloc(sizeof(float)*N*N);
    #else
    a = (float *) malloc(sizeof(float)*N*N);
	b = (float *) malloc(sizeof(float)*N*N);
	c = (float *) malloc(sizeof(float)*N*N);
    #endif

    srand (0); //time(NULL));    
    for(i=0; i<p; i++) {
        lastJJ[i*16] = (i*(n/p))-tile;
        lastII[i*16] = -tile;
        lastKK[i*16] = -tile;
        lastJJLog[i*16] = (i*(n/p))-tile;
        lastIILog[i*16] = -tile;
        lastKKLog[i*16] = -tile;
        insideTxII[i*16] = 0;
        insideTxKK[i*16] = 0;
    }
    for (i=0; i<n; i++) {
        for (j=0; j<n; j++) {
            a[decomp(i,j,n)] = drand48();
            b[decomp(i,j,n)] = drand48();
            //a[i][j] = 2.0;
            //b[i][j] = 3.0;
            c[decomp(i,j,n)] = 0.0;
            //            a[i][j] = rand() % 20;
            //            b[i][j] = rand() % 20;
            //            c[i][j] = 0;
        }
    }
}

void PrintC() 
{
    int i, j;

    printf("The C matrix:\n");
    for (i=0; i< n; i++) {
        for (j=0; j< n; j++) 
            printf("%0.2f\t",c[decomp(i,j,n)]); 
        printf("\n");
    }
}

/*
bool check_for_psync(struct timeval* old_timestamp)
{
  struct timeval timestamp, difftime;
  
  gettimeofday(&timestamp, NULL);
  timersub(&timestamp, old_timestamp, &difftime);
  
  #ifdef ONESECOND
	if(difftime.tv_sec >= 1)
  #elif HALFSECOND
	if(difftime.tv_usec >= 500000 || difftime.tv_sec >= 1)
  #elif QUARTERSECOND
	if(difftime.tv_usec >= 250000 || difftime.tv_sec >= 1)
  #elif EIGHTHSECOND
	if(difftime.tv_usec >= 125000 || difftime.tv_sec >= 1)
  #elif TENTHSECOND
	if(difftime.tv_usec >= 100000 || difftime.tv_sec >= 1)
  #elif INF
	if(1)
  #else
	exit(-1);
  #endif
	{
    // Notify other threads that a psync is about to occur
    psync_thread_waiting.store(true);
 
 
    // Wait until all threads have finished computing
    {
      std::unique_lock lk(psync_mutex);
      psync_thread_cv.wait(lk, [] {
        return psync_waiting_ctr.load() >= psync_active_threads.load(); 
      });
    }
   
    {
      std::lock_guard lk(psync_mutex);
      
      // Perform the psync
      #ifdef USE_PMO
  		psync(c);
      #elif defined(USE_PMALLOC)
      p_persist();
      #endif
      
      psync_waiting_ctr.store(0);
      psync_thread_waiting.store(false);
    }
    
    // Notify all other threads psync as concluded
    worker_thread_cv.notify_all();
    
    return true;
  }
  
  return false;
}

void psync_thread_func()
{
    struct timeval old_timestamp;
    gettimeofday(&old_timestamp, NULL);
    
    while (psync_active_threads.load() != 0)
    {
      if(check_for_psync(&old_timestamp))
      {
        gettimeofday(&old_timestamp,NULL);
      }
    }
}
*/

void multiply(std::string name, int dx, int wait_count) {
    /* each thread has a private version of local variables */
    int     iter, i, j, k, ii, jj, kk, r, l;  //iterators
    float     sum;                      //sum of multiplication
    int     firstLoop, lastLoop;

    firstLoop = dx*(n/p);
    lastLoop = firstLoop + (n/p);

    struct timeval start_time, end_time, diff_time,timestamp, old_timestamp,difftime;
    bool synced = false;
    

    /*****************  The multiplication code  **********************/

    for(iter = 0; iter < NUM_ITER; iter++)
    {
    gettimeofday(&start_time, NULL);
    gettimeofday(&timestamp, NULL);
    gettimeofday(&old_timestamp, NULL);
    for (kk=0; kk<n; kk+=tile) { 
        for (ii=0; ii<n; ii+=tile) {
            for (jj=firstLoop; jj<lastLoop; jj+=tile) {
                for (i=ii; i<(ii+tile); i++) {
                    for(j=jj; j<(jj+tile); j++) {
                        sum = c[decomp(i,j,N)];             // initialize the value of the current element
                        for(k=kk; k<(kk+tile); k++){
                            sum += a[decomp(i,k,N)]*b[decomp(k,j,N)];// calculate the sum for this element
                        }
                        c[decomp(i,j,N)] += sum;             // store the newly computed element value
			if (!synced && !dx && kk % PSYNC_RATIO == 0) {
				should_psync = true;
				synced = true;
			}
		    	check_for_psync(dx, kk, n);
                    } //end of for j
                } //end of for i
            } //end of for jj
        } //end of for ii
		 synced = false;
    } //end of for kk
	  gettimeofday(&end_time, NULL);
	  timersub(&end_time, &start_time, &diff_time);
    //lock.lock();
    end_times[dx] = diff_time;
    //lock.unlock();
    std::ostringstream ss;
    ss << name << ", " << diff_time.tv_sec << "." << diff_time.tv_usec << "\n";
    std::cout << ss.str();
    }


psync_thread_finishing();
    /*
  psync_active_threads.fetch_sub(1);
  psync_thread_cv.notify_all();
  */

//    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2[tid]);
    return;
}

void dump_proc_info(char *procname)
{
	int num_threads;
	char loc[50];
	sprintf(loc, "mkdir -p RESULTS/%s/%d", procname, num_threads);
	system(loc);	
	sprintf(loc, "cp -r /proc/%d RESULTS/%s/%d/", getpid(), procname, num_threads);
	system(loc);	
}

int main(int argc, char **argv)
{
struct timeval begin1, end1, diff1;
    int execTime;
    gettimeofday(&begin1, NULL);


    if(argc > 1)
        p = atoi(argv[1]);
    else
	    p = 1;

    if(argc > 2)  {
	N = atoi(argv[2]);
//	N = N/(N*3*sizeof(float));
//	printf("N is %lld\n", N);
    }
    else
	    N = 4096;//(4*(1024));//(2*4096);

    if(argc > 3)  
	    tile = atoi(argv[3]);
    else
	    tile = 16;

    if (argc > 4)
	    PSYNC_RATIO = atoi(argv[4]);

    else
	    PSYNC_RATIO = 32;

    printf ("PSYNC ratio is %ld\n", PSYNC_RATIO);

    n = N;
	POOL_SIZE = N*N*3*sizeof(float)+sizeof(Pool); //1024*1024*1024*1024; //253*sizeof(float)*N*N;
	printf("POOL Size is %lld\n", POOL_SIZE + sizeof(Pool));

    std::vector<std::thread> threads;
	lastJJ = (int *) malloc(sizeof(int) * p*16);         // the last computed iteration of kk loop
	lastII = (int *) malloc(sizeof(int) * p*16);           // the last computed iteration of i loop
	lastKK = (int *) malloc(sizeof(int) * p*16);        // the last computed iteration of kk loop
	lastJJLog = (int *) malloc(sizeof(int) * p*16);        // log for the last computed iteration of i loop
	lastIILog = (int *) malloc(sizeof(int) * p*16);        // log for the last computed iteration of i loop
	lastKKLog = (int *) malloc(sizeof(int) * p*16);       // log for the last computed iteration of kk loop
	insideTxII = (int *) malloc(sizeof(int) * p*16);               // Flag for entering logging region for updating II index
	insideTxKK = (int *) malloc(sizeof(int) * p*16);               // Flag for entering logging region for updating KK index
	// t1 = (timespec *) malloc(sizeof(timespec) * p);
	// t2 = (timespec *) malloc(sizeof(timespec) * p);
	// d = (timespec *) malloc(sizeof(timespec) * p);

    int            ret, dx;
    int i;
    
    /* Initialize the matrices
    #ifdef USE_PMALLOC
    p_init();
    #endif
    */
    set_psync_thread_count(p);
    Initialize();

 //   psync_active_threads.store(p);
//    std::thread psync_thread(psync_thread_func);
    auto psync_thread = spawn_psync_thread();
    for(dx = 0; dx < p; dx++){
        std::string name = "Thread" + std::to_string(dx);
        threads.push_back(std::thread(multiply, name, dx, p - 1));
        end_times.push_back(begin1);
    }

    for(auto& t : threads) {
        t.join();
    }
    psync_thread.join();
    //p_persist();

    gettimeofday(&end1, NULL);
  float avg_time = 0;
    for(i=0; i<p; i++) {
        float curr = (float)end_times[i].tv_sec + (float)(end_times[i].tv_usec)/1000000.0;
        avg_time = avg_time + curr;
    }
    avg_time = avg_time/p;
    timersub(&end1, &begin1, &diff1);
    std::cout << "Total Time = " << diff1.tv_sec << "." <<  diff1.tv_usec << " secs\n";

    std::cout << "Avg Time per thread = " << avg_time << " secs\n";

    
    dump_times(diff1);
    return 0;
}

