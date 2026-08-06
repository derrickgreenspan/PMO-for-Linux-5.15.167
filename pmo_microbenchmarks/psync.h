#include <thread>
#include <sys/time.h>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include "pmo.h"
#include "pmalloc.h"
//#define PSYNC_RATE_USEC 250000
//#define PSYNC_RATE_USEC 1000000
//#define PSYNC_RATE_ITER 2
//#define ATTACHDETACH_RATE
//#define rate_attachdetach 4000000
//#define rate_attachdetach 16000000


extern unsigned long long int total_cycle_times;
extern unsigned long long int times_invoked;
extern unsigned long long int attach_time, detach_time, psync_time;
bool should_psync = false;
unsigned long int PSYNC_RATE_ITER;

#define set_should_psync() should_psync = true

static std::atomic_bool psync_thread_waiting = false; // Indicates to other threads they should stop processessing and wait for psync
static std::atomic_int psync_waiting_ctr = 0;         // Counter used to indicate the number of threads waiting on the psync thread
static std::condition_variable psync_thread_cv;       // Condition variable used by the psync thread to wait for worker threads to complete
static std::condition_variable worker_thread_cv;      // Condition variable used by non-psync threads to wait for psync to complete
static std::mutex psync_mutex;                        // Mutex used by threads
static std::atomic_int psync_active_threads = 0;      // Counter used to indicate how many threads are actually running

static void set_psync_thread_count(int tc)
{
  psync_active_threads.store(tc);
}

static void psync_thread_fn(void) /*long int rate_usec*/
{  
  struct timeval tick, tock;
  while (psync_active_threads.load() != 0)
  {
    const auto now = std::chrono::high_resolution_clock::now();
    if (should_psync)
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
     
      // Perform the psync
        std::lock_guard lk(psync_mutex);
	//printf("Now is %lld\n", now);
        
        p_persist();
	/* Check if time has expired for attach/detach */
//	if (std::chrono::duration_cast<std::chrono::microseconds>(now - last_time_cycle).count() >= rate_attachdetach) {
	if( times_invoked && times_invoked % PSYNC_RATE_ITER == 0) {
		pthread_mutex_lock(&pcache->lock);
		gettimeofday(&tick, NULL);
		/* perform the detach */
		detach(pcache);
		gettimeofday(&tock, NULL);
		detach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
		/* perform the attach */
//	    	printf("Performing reattach..\r");
		gettimeofday(&tick, NULL);
		pcache = (Pool *) attach(make_pmo_name(), 'w', PMO_KEY);
		gettimeofday(&tock, NULL);
		attach_time += (tock.tv_sec - tick.tv_sec)*1e6 + (tock.tv_usec - tick.tv_usec);
		pthread_mutex_unlock(&pcache->lock);
		total_cycle_times++;
	}
        
	times_invoked++;
        psync_waiting_ctr.store(0);
        psync_thread_waiting.store(false);
        should_psync = false;
      
      // Notify all other threads psync has concluded
      worker_thread_cv.notify_all();
    }
  }
//}
}

static std::thread spawn_psync_thread(void)
{
  return std::thread(psync_thread_fn);
}

static void check_for_psync(int tid, size_t progress, size_t end)
{
	float percent;
  // See if the psync thread is waiting for us
  if (psync_thread_waiting.load())
  {
    // Say we are waiting
    std::unique_lock lk(psync_mutex);
    psync_waiting_ctr.fetch_add(1);
    psync_thread_cv.notify_all();
    if(!tid) {
	    percent = ((float) progress/end)*100;
//	    times_invoked++;
	    printf("%f\% (%lld/%lld), Psync Invoked %lld time(s), Attach/Detach Invoked %lld time(s)\r",
			    percent, progress, end, times_invoked, total_cycle_times);
	    fflush(stdout);
    }
    
    // Wait for psync thread to notify us of completion
    worker_thread_cv.wait(lk, [] { return !psync_thread_waiting.load(); });
  }
}

static void psync_thread_finishing()
{
  psync_active_threads.fetch_sub(1);
  psync_thread_cv.notify_all();
}
