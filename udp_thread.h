#ifndef UDP_THREAD_H
#define UDP_THREAD_H


#include "toaster_tool.h"

#include <pthread.h>
#include <deque>


extern std::deque<fjob_t*> live_data_jobs;
extern uint8_t stopRequested;
extern pthread_mutex_t job_list_mutex;
extern pthread_t udp_thread_id;

//extern void* udp_thread(void* arg);
extern void* udp_multicast_thread(void* arg);

extern int parseClassificationData( log_nfo_rating_t* nfo_rating, uint64_t id );


#endif // UDP_THREAD_H
