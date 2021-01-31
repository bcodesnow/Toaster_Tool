#ifndef TOASTER_TOOL_H
#define TOASTER_TOOL_H

extern "C" {
//#include "../esp32_toaster/main/x_platform.h"
#include "x_platform.h"
}

#include <cstdlib>
#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include "unistd.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>

#define SMOOTH_FREQS 1
#define SERVERPORT 8888
//#define SERVERIP   "192.168.2.119"

#define FJOB_FORMAT_RAW         (1u<<1u)
#define FJOB_FORMAT_CONVERTED   (1u<<2u)
#define FJOB_PREPARE_PLOT       (1u<<3u)
#define FJOB_INCLUDE_CLASS_DATA (1u<<4u)
#define FJOB_PIPE_TO_PYTHON     (1u<<5u)
#define FJOB_USE_PNG_TERMINAL   (1u<<6u)
#define FJOB_SMOOTH_FREQS       (1u<<7u)

#define MODE_PLOT 'p'
#define MODE_WRITE_PNG 'z'
#define MODE_RECEIVE_LIVE_DATA 'l'
#define MODE_PLOT_N_WRITE_RAW 'x'
#define MODE_PLOT_N_WRITE_CONVERTED 'y'
#define MODE_DOWNLOAD 'd'
#define MODE_WRITE_RAW 'g'
#define MODE_LIVE_PREDICT 'a'
#define MODE_WRITE_CLASS_DATA_TO_FILE 'c'

#define GNUPLOT "gnuplot -persist"

#define DEBUG_GNUPLOT 0

#if !DEBUG_GNUPLOT
#define gcmd(...) fprintf(gp, __VA_ARGS__)
#else
#define gcmd(...) printf(__VA_ARGS__)
#endif

#define FIFO_TO_PREDICTOR "/tmp/cToPython"
#define FIFO_FROM_PREDICTOR "/tmp/pythonToC"

typedef struct
{
    uint64_t id;
    uint64_t flags;
    uint64_t size;
    uint64_t element_cnt; // bit redundant information
    log_entry_100ms_t* ptr_ldat; // log data
    log_nfo_rating_t class_data;
} fjob_t;

typedef struct
{
    uint64_t id;
    std::string str;
} ferr_t;


typedef struct
{
    int status;
    std::deque<float> pt1000a;
    std::deque<float> pt1000b;
    std::deque<float> board;
    std::deque<float> t0; //a0-3
    std::deque<float> t1;
    std::deque<float> t2;
    std::deque<float> t3;
    std::deque<float> weight;
    std::deque<float> mlx_object;
    std::deque<float> mlx_ambient;
    std::deque<float> sht_t;
    std::deque<float> sht_h;
    std::deque<float> smoke_uv;
    std::deque<float> smoke_ir;
    std::deque<int16_t> freq0;
    std::deque<int16_t> freq1;
    std::deque<int16_t> freq2;
    std::deque<int16_t> r;
    std::deque<int16_t> g;
    std::deque<int16_t> b;
    std::deque<int16_t> c;
    std::deque<uint16_t> aq_TVOC;
    std::deque<uint16_t> aq_eCO2;
    std::deque<uint16_t> aq_rawH2;
    std::deque<uint16_t> aq_rawEthanol;
    std::deque<uint32_t> sht_abs_humi;
} converted_log_t;

typedef enum
{
    MEAS_SHT_T = 0,
    MEAS_SHT_H,
    MEAS_MLX_OBJECT,
    MEAS_MLX_AMBIENT,
    MEAS_PT1000A,
    MEAS_PT1000B,
    MEAS_FREQ0,
    MEAS_FREQ1,
    MEAS_FREQ2,
    MEAS_COL_R,
    MEAS_COL_G,
    MEAS_COL_B,
    MEAS_COL_C,
    MEAS_TVOC,
    MEAS_eCO2,
    MEAS_rawH2,
    MEAS_rawEthanol,
    MEAS_weight,
    MEAS_A0,
    MEAS_A1,
    MEAS_A2,
    MEAS_A3,
    MEAS_BOARD,
    MEAS_SMOKE_UV,
    MEAS_SMOKE_IR,
    MEAS_ABS_HUMI,
    MEAS_MAX
} measurement_t;

#define UDP_MODE_LIVE_DATA 1
#define UDP_MODE_DOWNLOAD 2
#define UDP_MODE_LIVE_PREDICT 3

typedef struct
{
    uint8_t mode;
    uint16_t arg1;
    uint16_t arg2;
} udp_multicast_thread_args_t;

extern udp_multicast_thread_args_t targs;
extern converted_log_t log_conv;
extern FILE * gp; // gnu plot interface
extern FILE * fil_out;
extern FILE * fdRead; // Pipes
extern FILE * fdWrite;

int16_t get_i16_avg(int16_t* start_ptr, uint16_t len);
int16_t get_freq(int16_t* start_ptr, uint16_t len);
int parseClassificationData(log_nfo_rating_t *nfo_rating, uint64_t id);
void add_header2file(FILE* fil_out, measurement_t type);
void remove_last_x_conv(uint32_t x);
void add_meas( log_entry_100ms_t* entry, measurement_t type, uint64_t flags, uint64_t idx );
void check_and_fix_min_max(float* min, float* max, char* string);

int creatNewFifo(const char* fifoName);


void writeClassDataToFile(FILE* f, log_nfo_rating_t *nfo_rating);

#endif // TOASTER_TOOL_H
