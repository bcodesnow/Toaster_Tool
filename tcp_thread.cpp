#include "tcp_thread.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <sys/time.h>

static int serverSock;

std::deque<fjob_t*>             live_data_jobs;
pthread_mutex_t          job_list_mutex;
pthread_t                tcp_thread_id;
static uint8_t                  thread_state;
static bool*                    start_flag;

#define TCP_STA_LISTENING 0
#define TCP_STA_RECEIVING 1


long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

void* tcp_thread(void* arg)
{
    struct  sockaddr_in serverAddr;
    socklen_t           serverAddrLen = sizeof(serverAddr);

    /* make this thread cancellable using pthread_cancel() */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed");
    }
    bzero(&serverAddr, sizeof(serverAddr));

//    close(serverSock);
    int optval = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVERPORT);

    //inet_aton() converts the Internet host address cp from the IPv4 numbers-and-dots notation into binary form (in network byte order) and stores it in the structure that inp points to.
    // Binding newly created socket to given IP and verification
    if ((bind(serverSock, (sockaddr *) &serverAddr, sizeof(serverAddr))) != 0) {
        printf("socket bind failed...\n");
        pthread_cancel(pthread_self());
    }
    else
        printf("Socket successfully binded..\n");

    thread_state = TCP_STA_LISTENING;
    int clientSock, len;
    struct sockaddr_in clientAddr;
    fjob_t* fjob;


    long long last_msg_ts;
    long long current_msg_ts;
    /* start sending thread */
    while(1)
    {
        printf("A-LIVE!\n");

        if (thread_state == TCP_STA_LISTENING)
        {
            // Now server is ready to listen and verification
            if ((listen(serverSock, 5)) != 0)
            {
                printf("Listen failed...\n");
                pthread_cancel(pthread_self());
            }
            else
                printf("Server listening..\n");

            len = sizeof(clientAddr);

            // Accept the data packet from client and verification
            clientSock = accept(serverSock, (sockaddr*)&clientAddr, (socklen_t *) &len);
            if (clientSock < 0) {
                printf("server acccept failed...\n");
                pthread_cancel(pthread_self());
                current_msg_ts = current_timestamp();
            }
            else
            {
                thread_state++;
                printf("server acccept the client...\n");
            }
        }
        else if ( thread_state == TCP_STA_RECEIVING)
        {

            int32_t rec_len;


            fjob = new fjob_t(); // free in non receive case missing!!
            fjob->ptr_ldat = new log_entry_100ms_t();

            rec_len = recv(serverSock , (uint8_t*) &fjob->ptr_ldat ,  sizeof (log_entry_100ms_t), MSG_DONTWAIT);
            if( rec_len > 0)
            {
                if (rec_len == sizeof (log_entry_100ms_t))
                {
                     printf("Received!");
                    fjob->flags = 1;
                    pthread_mutex_lock(&job_list_mutex);
                    live_data_jobs.push_back(fjob);
                    pthread_mutex_unlock(&job_list_mutex);
                }
                else if (rec_len < sizeof(log_entry_100ms_t))
                {
                    printf("Receive %d bytes! \n", rec_len);
                }
            }
            else
            {
                printf("Nothing this time!\n");
            }


            if ( (current_msg_ts - last_msg_ts) > 15000 )
            {
                printf("the client is inactive, pull the plug!\n");
            }
            if ( fjob != nullptr) // there was a msg!
                last_msg_ts = current_msg_ts;


        }
        /* have we terminated yet? */
        pthread_testcancel();
        /* no, take a rest for a while */
        usleep(1000);
    }
}
