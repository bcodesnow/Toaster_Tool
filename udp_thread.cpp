#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <ctime>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>     /* malloc, free, rand */
#include "udp_thread.h"

std::deque<fjob_t*> live_data_jobs;
pthread_mutex_t     job_list_mutex;
pthread_t           udp_thread_id;
uint8_t stopRequested;

//#define UDP_STA_LISTENING 0
//#define TCP_STA_RECEIVING 1
//#define RBUFF_MASK 0xFFF
//#define RBUFF_SIZE (0xFFF+1)
//static int serverSock;
//static uint8_t      thread_state;

//void* udp_thread(void* arg)
//{
//    struct sockaddr_in receive_addr;
//    int trueflag = 1;
//    int fd;

//    printf("Receiver Starts\n");

//    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
//        printf("ERROR: socket!");

//    //    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//    //    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);


//    memset(&receive_addr, 0, sizeof receive_addr);
//    receive_addr.sin_family = AF_INET;
//    receive_addr.sin_port = (in_port_t) htons(SERVERPORT);
//    receive_addr.sin_addr.s_addr = INADDR_ANY;


//    if ( bind(fd, (const struct sockaddr *)&receive_addr, sizeof(receive_addr)) < 0 )
//    {
//        perror("bind failed");
//    }

//    struct timespec nsleep;
//    nsleep.tv_nsec = 750000;

//    fjob_t* fjob;


//    long long last_msg_ts;
//    long long current_msg_ts;
//    struct sockaddr_in cliaddr;


//    printf("UDP STARTED!\n");
//    while(1)
//    {
//        fjob = nullptr;
//        int32_t recSize;
//        fjob = new fjob_t();
//        fjob->ptr_ldat = new log_entry_100ms_t();
//        int len = sizeof(cliaddr);  //len is value/resuslt

//        recSize = recvfrom(fd, (uint8_t*) fjob->ptr_ldat, sizeof(log_entry_100ms_t), MSG_DONTWAIT, (sockaddr*) &cliaddr, (socklen_t*)&len);

//        if (recSize != sizeof(log_entry_100ms_t))
//        {
//            if ( recSize != -1 )
//                printf("Receive %d bytes - not the right size!! \n", recSize);
//            free(fjob->ptr_ldat);
//            free(fjob);
//        }
//        else
//        {
//            printf("Received!");
//            printf("TVOC: %d\n", fjob->ptr_ldat->aq_TVOC);
//            printf("Temp: %d\n", fjob->ptr_ldat->sht_t);

//            fjob->flags = 1;
//            pthread_mutex_lock(&job_list_mutex);
//            live_data_jobs.push_back(fjob);
//            pthread_mutex_unlock(&job_list_mutex);
//        }
//        /* we did it.. */
//        /* have we terminated yet? */
//        pthread_testcancel();
//        /* no, take a rest for a while */
//        nanosleep(0, &nsleep);
//    }
//}


void* udp_multicast_thread(void* arg)
{
    udp_multicast_thread_args_t* args = (udp_multicast_thread_args_t*) arg;
    printf("Arg1->%d, Arg2->%d\n", args->arg1, args->arg2);

    int trueflag = 1;
    if ( args->mode == UDP_MODE_LIVE_DATA)
        printf("Receiver Starts\n");
    else if ( args->mode == UDP_MODE_DOWNLOAD)
        printf("Download Client Starts\n");


    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
    }

    // allow multiple sockets to use the same PORT number
    //
    u_int yes = 1;
    if (  setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes) ) < 0 )
    {
        perror("Reusing ADDR failed");
    }


    //    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    //    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);


    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(SERVERPORT);

    // bind to receive address
    //
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
    }

    // use setsockopt() to request that the kernel join a multicast group
    //
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(IPV4_MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if ( setsockopt( fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq) ) < 0 )
    {
        perror("setsockopt");
    }
    struct timespec nsleep;
    nsleep.tv_nsec = 10000;

    fjob_t* fjob;

    int64_t pkg_cnt;
    printf("UDP STARTED!\n");

    // DOWNLOAD Mode Flags and Variables
#define STATE_PRE_START 1
#define STATE_IN_DL 2
#define STATE_DONE 3

    static int dl_state = STATE_PRE_START;
    static log_entry_100ms_transport_t* log_tran_ptr;
    static log_nfo_rating_transport_t  last_nfo_tran;
    static uint8_t fil_open_header_ready = 0;
    static size_t written = 0;
    // END DOWNLOAD Mode Flags and Variables


    struct timeval tv =
    {
        .tv_sec = 0,
        .tv_usec = 250,
    };
    while(1)
    {
        fjob = nullptr;
        int32_t recSize = 0;
        int addrlen = sizeof(addr);

        fd_set rfds;
        if ( ( args->mode == UDP_MODE_LIVE_DATA ) || ( args->mode == UDP_MODE_LIVE_PREDICT )  )
        {
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            int s = select ( fd +1, &rfds, NULL, NULL, &tv);
            if ( s > 0)
            { // Read only if there is something to read..
                fjob = new fjob_t();
                fjob->ptr_ldat = new log_entry_100ms_t();
                recSize = recvfrom(fd, (uint8_t*) fjob->ptr_ldat, sizeof(log_entry_100ms_t), 0, (sockaddr*) &addr, (socklen_t*)&addrlen);
            }

            if ( UDP_MODE_LIVE_PREDICT )
            {
                if ( stopRequested )
                {
                    unsigned char stop = 0xAA;
                    sendto(fd, (uint8_t*) &stop, sizeof (char), 0, (sockaddr*) &addr, (socklen_t)addrlen);
                    stopRequested = 0;
                }
            }

            if ( recSize > 0)
            {
                if (recSize != sizeof(log_entry_100ms_t))
                {
                    if ( recSize != -1 )
                        printf("Receive %d bytes - not the right size!! \n", recSize);
                    free(fjob->ptr_ldat);
                    free(fjob);
                }
                else
                {
                    pkg_cnt++;
                    if ( pkg_cnt % 100 == 0 )
                        printf("...Received: %d packages.. SHT_T: %.2f\n", pkg_cnt, fjob->ptr_ldat->sht_t / 1000.0f );
                    fjob->flags = 1;
                    pthread_mutex_lock(&job_list_mutex);
                    live_data_jobs.push_back(fjob);
                    pthread_mutex_unlock(&job_list_mutex);
                }
            }
        }
        else if ( args->mode == UDP_MODE_DOWNLOAD)
        {
            char fname[32];

            static uint8_t buf[1024];
            recSize = recvfrom(fd, &buf[0], 1024, 0, (sockaddr*) &addr, (socklen_t*)&addrlen);
            if ( recSize > 0)
            {
                toaster_cmd_t* cmd_received = (toaster_cmd_t*)&buf[0];
                //printf("FIRST BYTE: %d \n", buf[0]);
                static int segment_nr_to_receive = 0;
                static int sequence_err = 0;

                switch ( cmd_received->cmd )
                {
                case CMD_DL_MODE_ALIVE:
                    if ( dl_state == STATE_PRE_START)
                    {
                        toaster_cmd_t cmd_to_send;
                        cmd_to_send.cmd = CMD_SET_DL_RANGE;
                        cmd_to_send.arg1 = args->arg1;
                        cmd_to_send.arg2 = args->arg2;
                        dl_state = STATE_IN_DL;
                        sendto(fd, (uint8_t*) &cmd_to_send, sizeof (toaster_cmd_t), 0, (sockaddr*) &addr, (socklen_t)addrlen);
                    }
                    printf("DL Mode Alive Arrived \n");
                    break;
                case LOG_ENTRY_TRANSPORT_STRUCT_IDENTIFIER:
                    log_tran_ptr = (log_entry_100ms_transport_t*)&buf[0];
                    if ( log_tran_ptr->idx != segment_nr_to_receive)
                    {
                        printf("Udp packages really do not arrive sorted... :(! \n");
                        sequence_err = 1;
                    }

                    segment_nr_to_receive++;
                    if ( fil_open_header_ready)
                    {
                        written += fwrite( (char*) &log_tran_ptr->payload, sizeof(log_entry_100ms_t), sizeof(char), fil_out);
                    }
                    else
                    {
                        printf("ERR: Log Entry arrived, but there was no valid header for it! \n");
                    }

                    break;
                case LOG_NFO_RATING_TRANSPORT_STRUCT_IDENTIFIER:
                    segment_nr_to_receive = 0;
                    sequence_err = 0;
                    memcpy(&last_nfo_tran, &buf[0], sizeof(log_nfo_rating_transport_t ));
                    printf("Header Received %d\n", last_nfo_tran.file_idx);
                    printClassData(&last_nfo_tran.payload);
                    sprintf( fname, "RLD%04d.TXT", last_nfo_tran.file_idx);
                    fil_out = fopen(fname, "w+");
                    fil_open_header_ready = 1;
                    if (fil_out == NULL)
                    {
                        printf("file open failed!");
                        perror("fopen()");
                        exit(EXIT_FAILURE);
                    }
                    break;
                case CMD_DL_FILE_DONE:
                {
                    uint8_t all_ok = 0;
                    printf("File done! \n");
                    fclose(fil_out);
                    toaster_cmd_t cmd_to_send;
                    /*THE LENGHT CHECKING IS NOT CORRECT!!! */
                    if (  ( cmd_received->arg1 == written) && (!sequence_err) )
                    {
                        printf("The right lenght arrived in the right order!\n");
                        all_ok = 1;
                    }
                    else
                    {
                        if ( cmd_received->arg1 != written )
                            printf("ERR - Size %d instead of %d\n", written, cmd_received->arg1 );
                        if ( sequence_err == 1)
                            printf ("ERR - Not ordered!");
                        written = 0;
                        sequence_err = 0;
                        cmd_to_send.cmd = CMD_REPEAT_DL;
                        printf("repeating dl!\n");
                        sendto(fd, (uint8_t*) &cmd_to_send, sizeof (toaster_cmd_t), 0, (sockaddr*) &addr, (socklen_t)addrlen);
                    }
                    if ( all_ok )
                    {
                        // write class data
                        sprintf( fname, "CLS%04d.TXT", last_nfo_tran.file_idx);
                        fil_out = fopen(fname, "w+");
                        if (fil_out == NULL)
                        {
                            perror("fopen()");
                            exit(EXIT_FAILURE);
                        }
                        fwrite( (char*) &last_nfo_tran.payload, sizeof(log_nfo_rating_t), sizeof(char), fil_out);
                        fclose(fil_out);

                        written = 0;
                        fil_open_header_ready = 0;
                        cmd_to_send.cmd = CMD_CONTINUE_DL;
                        sendto(fd, (uint8_t*) &cmd_to_send, sizeof (toaster_cmd_t), 0, (sockaddr*) &addr, (socklen_t)addrlen);
                    }
                }
                    break;
                default:

                    if (recSize == sizeof(log_entry_100ms_t) )
                        printf("Toaster is in Live Data Mode, Please Switch to DL Mode!");
                    else
                        printf("unhandled cmd\n");
                    break;
                }
            }
            //printf("Message with size %d from = \"%s\"\n",  recSize, inet_ntoa(addr.sin_addr));


        }
        /* we did it.. */
        /* have we terminated yet? */
        pthread_testcancel();
        /* no, take a rest for a while */
        nanosleep(0, &nsleep);
    }
}
