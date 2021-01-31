#ifndef TCP_THREAD_H
#define TCP_THREAD_H

#include "toaster_tool.h"

#include <pthread.h>
#include <deque>

extern std::deque<fjob_t*> live_data_jobs;
extern pthread_mutex_t job_list_mutex;
extern pthread_t tcp_thread_id;

extern void* tcp_thread(void* arg);

#endif // TCP_THREAD_H
