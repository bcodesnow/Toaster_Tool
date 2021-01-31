#include "toaster_tool.h"
#include "multi_plot.h"
#include "udp_thread.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>           /* Definition of AT_* constants */
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <csignal>

FILE *gp = nullptr; // gnu plot interface
FILE* fil_out; // csv output

converted_log_t log_conv;
udp_multicast_thread_args_t targs;

char mode; // global program mode

FILE * fdRead;
FILE * fdWrite;

void print_usage()
{
    printf("*************** Toaster Tool ***************\n");
    printf("* arg 1: mode arg2: first idx, arg3: second idx \n");
    printf("* \n");
    printf("* modes:\n");
    printf("*  l - receive live data \n");
    printf("*  p - plot data \n");
    printf("*  x - plot and write raw data to csv \n");
    printf("*  y - plot and write converted data to csv \n");
    printf("*  z - save plots to png \n");
    printf("*  g - write raw data to csv \n");
    printf("*  d - download data \n");
    printf("*  a - live predict \n");
    printf("*  c - write classificatin information to file \n");
    printf("********************************************\n");
}

void signalHandler( int signum ) {
    printf("\nBeeing Interrupted: %d\nGood Bye! \n", signum);
    if (mode == MODE_LIVE_PREDICT)
    {
        fclose(fdRead);
        fclose(fdWrite);
        // kill python predictor script
        system("kill -9 `cat /tmp/livePredictor.pid`");
        // destroy the pipes
        remove(FIFO_TO_PREDICTOR);
        remove(FIFO_FROM_PREDICTOR);
    }

    // cleanup and close up stuff here
    // terminate program

    exit(signum);
}

void write_csv(fjob_t* fjob)
{
    if (fjob->flags & FJOB_PREPARE_PLOT)
        log_conv.status = 1;
    int i;
    char fname[32];
    uint8_t writeCsv = 0;
    if ( fjob->flags & FJOB_FORMAT_RAW)
    {
        writeCsv = 1;
        sprintf( fname, "RAW_DATASET%04d.CSV", fjob->id);
    }
    else if ( fjob->flags & FJOB_FORMAT_CONVERTED )
    {
        writeCsv = 1;
        sprintf( fname, "CONV_DATASET%04d.CSV", fjob->id);
    }
    if ( writeCsv)
    {
        printf("...Writing Output File: %s\n", fname);
        fil_out = fopen(fname, "w+");
        if (fil_out == NULL)
        {
            perror("fopen()");
            exit(EXIT_FAILURE);
        }

        // add time header
        fprintf(fil_out,"time,");
        // add measurement headers
        for ( i= 0; i<MEAS_MAX; i++ )
        {
            // Fill CSV Header with Column Names
            add_header2file(fil_out, (measurement_t)i);
            if (i!=(MEAS_MAX-1))
                fprintf(fil_out,",");
            else
            {
                // Close Line and Add Class Header)
                fprintf(fil_out,",Classification");
                fprintf(fil_out,"\n");
            }
        }
    }
    printClassData (&fjob->class_data);

    // add data block
    for ( i = 0; i < fjob->element_cnt; i++)
    {
        // TimeStamp
        if ( writeCsv )
            fprintf(fil_out,"%d,", i*100);
        for (int j= 0; j<MEAS_MAX; j++ )
        {
            // Fill in Each Measurement
            add_meas( &fjob->ptr_ldat[i], (measurement_t)j, fjob->flags, i);
            if ( writeCsv )
            {
                if (j!=(MEAS_MAX-1))
                    fprintf(fil_out,",");
                else
                {
                    // Add Classification Data as Last Column and Close Line
                    if ( fjob->class_data.rating == TOO_EARLY )
                        fprintf(fil_out,",%d",fjob->class_data.rating);
                    else if ( TOO_LATE )
                    {
                        if ( i < ( fjob->element_cnt - PERFECT_WINDNOW_100MS -  ( fjob->class_data.secs * 10 ) ) )
                            fprintf(fil_out,",%d",TOO_EARLY);
                        else if ( ( i >= ( fjob->element_cnt - PERFECT_WINDNOW_100MS -  ( fjob->class_data.secs * 10 ) ) )
                                  &&
                                  ( i <= ( fjob->element_cnt -  ( fjob->class_data.secs * 10 ) ) ) )
                            fprintf(fil_out,",%d",PERFECT);
                        else
                            fprintf(fil_out,",%d",TOO_LATE);
                    }
                    else
                    {
                        // Perfect
                        if ( i < ( fjob->element_cnt - PERFECT_WINDNOW_100MS ) )
                            fprintf(fil_out,",%d",TOO_EARLY);
                        else
                            fprintf(fil_out,",%d",PERFECT);
                    }
                    fprintf(fil_out,"\n");
                }
            }
        }
    }
}

int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler);
    int from_id;
    int to_id;
    char fname[16];
    uint64_t i;
    std::vector<fjob_t*> fjobs;
    std::vector<ferr_t> ferrs;

    // Check args
    if ( (argc != 4) && (argc != 2))
    {
        print_usage();
        return 0;
    }
    else
    {
        mode = *argv[1];

        if ( argc == 4)
        {
            from_id = atoi(argv[2]);
            to_id = atoi(argv[3]);
            if ( from_id > to_id )
            {
                printf("from was greater than to, please reconsider! \n");
                return 0;
            }
        }
    }
    printf("Mode -");
    switch (mode)
    {
    case MODE_PLOT:
        printf(" p - plot data \n");
        break;
    case MODE_RECEIVE_LIVE_DATA:
        printf(" l - receive live data \n");
        break;
    case MODE_PLOT_N_WRITE_RAW:
        printf(" x - plot and write raw \n");
        break;
    case MODE_PLOT_N_WRITE_CONVERTED:
        printf(" y - plot and write converted \n");
        break;
    case MODE_DOWNLOAD:
        printf(" d - download \n");
        break;
    case MODE_WRITE_RAW:
        printf(" g - write raw data to csv \n");
        break;
    case MODE_LIVE_PREDICT:
        printf(" a - live predict \n");
        break;
    case MODE_WRITE_PNG:
        printf(" z - save to png \n");
    case MODE_WRITE_CLASS_DATA_TO_FILE:
        printf("c - write classificatin information to file \n");
        break;
    }
    // Let the lions in
    //
    /**************************************************************/
    //
    if ( (mode != MODE_RECEIVE_LIVE_DATA) && ( mode != MODE_DOWNLOAD) && (mode != MODE_LIVE_PREDICT) )
    {
        // Check | Create Directory
        struct stat st = {0};
        if (stat("/processed", &st) == -1) {
            mkdir("/processed", 0700);
        }

        printf("Toaster Tool, processing %d files \n", to_id-from_id+1);
        printf("...User input: from=%d, to=%d!\n", from_id, to_id);

        for ( i = from_id; i<= to_id; i++)
        {
            sprintf(&fname[0], (LOG_FILE_NAME), i);
            printf("...%s\n",fname);
            if( access( fname, R_OK ) != -1 )
            {
                stat(fname, &st);
                int size = st.st_size;
                if ( (size % sizeof(log_entry_100ms_t)) || (size == 0) )
                {
                    ferrs.push_back(ferr_t());
                    ferrs.back().id = i;
                    ferrs.back().str = "wrong size";
                }
                else
                {
                    log_nfo_rating_t rating;
                    if ( parseClassificationData( &rating, i ) == 0 )
                    {
                        // Id has valid data!
                        fjobs.push_back(new fjob_t());
                        fjobs.back()->id = i;
                        fjobs.back()->size = size;
                        memcpy(&fjobs.back()->class_data, &rating, sizeof(log_nfo_rating_t));
                    }
                    else
                    {
                        ferrs.push_back(ferr_t());
                        ferrs.back().id = i;
                        ferrs.back().str = "Class Data Not Found or Invalid";
                    }
                }
            }
            else
            {
                ferrs.push_back(ferr_t());
                ferrs.back().id = i;
                ferrs.back().str = "not found";
            }
        }

        if (ferrs.size())
            for ( i = 0; i<ferrs.size(); i++ )
                printf("ERR... File: " LOG_FILE_NAME " - %s \n", ferrs.at(i).id, ferrs.at(i).str.data() );

        if ( !fjobs.size())
        {
            printf("No valid files to process.. \n");
            return 0;
        }

        for ( i = 0; i < fjobs.size(); i++ )
        {
            sprintf(&fname[0], (LOG_FILE_NAME), fjobs.at(i)->id);
            printf("...%d Bytes to read from: %s \n", fjobs.at(i)->size, fname );
            FILE* f = fopen( &fname[0], "r");

            fjobs.at(i)->element_cnt = fjobs.at(i)->size / sizeof (log_entry_100ms_t);
            log_entry_100ms_t log[fjobs.at(i)->element_cnt];
            if ( fread( (char*)&log[0], sizeof (log_entry_100ms_t), fjobs.at(i)->element_cnt, f) != fjobs.at(i)->element_cnt )
            {
                ferrs.push_back(ferr_t());
                ferrs.back().id = i;
                ferrs.back().str = "error read";
            }
            else
                printf("...File Contains %d elements.\n", fjobs.at(i)->element_cnt);

            fjobs.at(i)->ptr_ldat = log; //using no dynamic alloc right now for the data

            fjobs.at(i)->flags  = 0;
            switch (mode)
            {
            case MODE_PLOT:
                fjobs.at(i)->flags |= FJOB_PREPARE_PLOT;
                break;
            case MODE_PLOT_N_WRITE_RAW:
                fjobs.at(i)->flags |= FJOB_PREPARE_PLOT | FJOB_FORMAT_RAW;
                break;
            case MODE_PLOT_N_WRITE_CONVERTED:
                fjobs.at(i)->flags |= FJOB_PREPARE_PLOT | FJOB_FORMAT_CONVERTED;
                break;
            case MODE_WRITE_RAW:
                fjobs.at(i)->flags |= FJOB_FORMAT_RAW;
                break;
            case MODE_WRITE_PNG:
                fjobs.at(i)->flags |= FJOB_PREPARE_PLOT | FJOB_USE_PNG_TERMINAL;
                break;
            default:
                printf("Landed in Default FJOB MODE!\n");
                break;
            }
            if ( SMOOTH_FREQS == 1)
                fjobs.at(i)->flags |= FJOB_SMOOTH_FREQS;

            log_nfo_rating_t rating;
            if ( mode == MODE_WRITE_CLASS_DATA_TO_FILE )
            {
                char fname[32];
                sprintf( fname, "CLS_NFO%04d.CSV", fjobs.at(i)->id);
                FILE* fcls = fopen( &fname[0], "w");
                writeClassDataToFile(fcls, &rating );
                fclose(fcls);
            }
            else
            {
                write_csv( fjobs.at(i) );
            }
            parseClassificationData( &rating, fjobs.at(i)->id );
            if ( mode == MODE_WRITE_PNG)
                printf("...Writing PNG! %d\n", i+1);
            else
                printf("...Writing CSV! %d\n", i+1);

            if ( (mode != MODE_WRITE_RAW) && (mode != MODE_WRITE_CLASS_DATA_TO_FILE) )
            {
                multi_plot(fjobs.at(i));
                remove_last_x_conv(fjobs.at(i)->element_cnt);
                if ( mode != MODE_WRITE_PNG )
                    sleep(3);
            }
        }
        fjobs.clear();
    }
    //
    /**************************************************************/
    //
    if ( mode == MODE_RECEIVE_LIVE_DATA )
    {
        /* Live Plotting */
        job_list_mutex = PTHREAD_MUTEX_INITIALIZER;
        targs.mode = UDP_MODE_LIVE_DATA;
        log_conv.status  = 1;
        pthread_create(&udp_thread_id, nullptr, udp_multicast_thread, static_cast<void*>( &targs )  );

        uint8_t started = 0;
        int received_log_entry_cnt = 0;
        struct timespec tp1,tp2;
        uint64_t tdiff;

        while (1)
        {
            fflush(stdout);
            pthread_mutex_lock(&job_list_mutex);
            fjob_t* t_job;
            if ( live_data_jobs.size() )
            {
                t_job = live_data_jobs.front();
                live_data_jobs.pop_front();
                pthread_mutex_unlock(&job_list_mutex);

                printf("...live_data_jobs.size(): %ld \n", live_data_jobs.size());
//                if (log_conv.b.size() > 2500)
//                    remove_last_x_conv(1);

                if ( started == 0 )
                {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &tp1) ;
                    started = 1;
                }
                else
                {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &tp2) ;
                    tdiff = ((tp2.tv_sec-tp1.tv_sec)*(1000*1000*1000) + (tp2.tv_nsec-tp1.tv_nsec)) / 1000 ;
                    tp1 = tp2;
                    if ( tdiff > 3E6)
                    {
                        // New Toast Begins
                        received_log_entry_cnt = 0;
                        if ( log_conv.b.size() > 0)
                            remove_last_x_conv(log_conv.b.size());
                    }
                }


                for (int j= 0; j<MEAS_MAX; j++ )
                {
                    add_meas( t_job->ptr_ldat, (measurement_t)j, FJOB_PREPARE_PLOT, 0);
                }

                free (t_job->ptr_ldat);
                free (t_job);

                fjob_t dummyJob;
                dummyJob.id = 69;
                if ( ( received_log_entry_cnt % 50 == 0 ) && received_log_entry_cnt > 49)
                    multi_plot(&dummyJob);
                received_log_entry_cnt++;

            }
            else
            {
                pthread_mutex_unlock(&job_list_mutex);
            }
            /* have we terminated yet? */
            pthread_testcancel();
            //
            /* no, take a rest for a while */
            usleep(100);
        }
    }
    //
    /**************************************************************/
    //
    if ( mode == MODE_LIVE_PREDICT )
    {
#define PIF_STA_STARTING 0
#define PIF_STA_RUNNING 1

#define PIF_SUB_STA_MKFIFO_START_PYTHON 0
#define PIF_SUB_STA_WAIT_WHILE_PYTHON_LOADS 1

        static int predictIFState = PIF_STA_STARTING;
        static int predictIfSubState = 0;

        job_list_mutex = PTHREAD_MUTEX_INITIALIZER;
        targs.mode = UDP_MODE_LIVE_PREDICT;
        pthread_create(&udp_thread_id, nullptr, udp_multicast_thread, static_cast<void*>( &targs )  );

        char buf[512];
        uint8_t started = 0;
        int received_log_entry_cnt = 0;
        struct timespec tp1,tp2;
        uint64_t tdiff;
        while (1)
        {
            int fifosOk = 0;
            switch ( predictIFState )
            {
            case PIF_STA_STARTING:
                switch ( predictIfSubState)
                {
                case PIF_SUB_STA_MKFIFO_START_PYTHON:
                    fifosOk |= creatNewFifo(FIFO_TO_PREDICTOR);
                    fifosOk |= creatNewFifo(FIFO_FROM_PREDICTOR);
                    if ( fifosOk == 0)
                    {
                        printf("opening!\n");
                        fdRead= fopen(FIFO_FROM_PREDICTOR, "r+");

                        fcntl( fileno(fdRead), F_SETFL, fcntl(fileno(fdRead), F_GETFL) | O_NONBLOCK);

                        fdWrite= fopen(FIFO_TO_PREDICTOR, "w+");
                        fcntl( fileno(fdWrite), F_SETFL, fcntl(fileno(fdWrite), F_GETFL) | O_NONBLOCK);

                        // start the python script to make guesses
                        system("gnome-terminal --geometry 80x24+100+300 -e /home/b/ws_python/livePredictor.py ");
                        predictIfSubState = PIF_SUB_STA_WAIT_WHILE_PYTHON_LOADS;
                    }
                    else
                    {
                        perror("creatNewFifo()");
                        exit(EXIT_FAILURE);
                    }
                    break;
                case PIF_SUB_STA_WAIT_WHILE_PYTHON_LOADS:
                    if ( fgets(&buf[0], 128, fdRead) !=  NULL)
                    {
                        if ( buf[0] == 'L' )
                        {
                            printf("...Python Script Loaded...\n");
                            predictIFState = PIF_STA_RUNNING;
                            predictIfSubState = 1;
                            if ( fputs("R\n", fdWrite) < 0)
                            {
                                printf("ERR: Sending Reset to Python\n");
                                perror("sendingThroughPipe");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    break;
                }
                    //FILE* testFil = fopen("RAW_DATASET0088.CSV","r");
                    //printf("%s", fgets(&arr[0], 512, testFil) );
                    //while (1)
                    //{
                break;
            case PIF_STA_RUNNING:
            {
                fjob_t* t_job;
                pthread_mutex_lock(&job_list_mutex);
                int liveDataJobsSize = live_data_jobs.size();
                if ( liveDataJobsSize )
                {
                    // Received new chunk
                    t_job = live_data_jobs.front();
                    live_data_jobs.pop_front();
                    pthread_mutex_unlock(&job_list_mutex);
                    if ( liveDataJobsSize > 3)
                        printf("...live_data_jobs.size(): %d \n", liveDataJobsSize);

                    if ( started == 0 )
                    {
                        clock_gettime(CLOCK_MONOTONIC_RAW, &tp1) ;
                        started = 1;
                    }
                    else
                    {
                        clock_gettime(CLOCK_MONOTONIC_RAW, &tp2) ;
                        tdiff = ((tp2.tv_sec-tp1.tv_sec)*(1000*1000*1000) + (tp2.tv_nsec-tp1.tv_nsec)) / 1000 ;
                        tp1 = tp2;
                        if ( tdiff > 3E6)
                        {
                            // New Toast Begins
                            received_log_entry_cnt = 0;
                            // Reset IF
                            printf("Sending Reset!\n");
                            stopRequested = 0;
                            while( fgets(&buf[0], 128, fdRead) != NULL ) {}; // CLEAN THE PIPE
                            if ( fputs("R\n", fdWrite) < 0)
                            {
                                printf("ERR: Sending Reset to Python\n");
                                perror("sendingThroughPipe");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }

                    // Message in the Queue
                    //printf("Chunk Received! \n");
                    fprintf(fdWrite,"%d,", i*100); // Time Column - (Unused) Value
                    t_job->flags = 0;
                    t_job->flags |= ( FJOB_PIPE_TO_PYTHON | FJOB_FORMAT_RAW ); // Change Pipe to Named Pipe and use Raw
                    for (int j= 0; j<MEAS_MAX; j++ )
                    {
                        // Fill in Each Measurement
                        add_meas( &t_job->ptr_ldat[i], (measurement_t)j, t_job->flags, received_log_entry_cnt );
                        if (j!=(MEAS_MAX-1))
                            fprintf(fdWrite,",");
                        else
                        {
                            fprintf(fdWrite,",1\n");
                        }
                    }
                    received_log_entry_cnt++;
                    fflush(fdWrite);

                    if ( fgets(&buf[0], 128, fdRead) !=  NULL)
                    {
                        if ( buf[0] == '1'  )
                        {
                            stopRequested = 1;
                            printf("STOP received -> dispatching to the Toaster!\n");
                        }
                    }
                    usleep(50000);

                    free (t_job->ptr_ldat);
                    free (t_job);
                }
                else
                    pthread_mutex_unlock(&job_list_mutex);
            }
                break;
            }
            fflush(stdout);
            /* have we terminated yet? */
            pthread_testcancel();
            //
            /* no, take a rest for a while */
            usleep(100);
        }
    }
    //
    /**************************************************************/
    //
    if ( mode == MODE_DOWNLOAD )
    {
        /* Live Plotting */
        targs.mode = UDP_MODE_DOWNLOAD;
        targs.arg1 = from_id;
        targs.arg2 = to_id;

        pthread_create(&udp_thread_id, nullptr, udp_multicast_thread, static_cast<void*>( &targs ) );

        int received_log_entry_cnt = 1;
        while (1)
        {
            /* This thread is useless in this mode, but the other modes take advantage of the separated udp thread */
            fflush(stdout);

            /* have we terminated yet? */
            pthread_testcancel();
            //
            /* no, take a rest for a while */
            usleep(10000);
        }
    }
    return 0;
}

