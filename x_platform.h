#ifndef TOASTER_CROSS_PLATFORM_TYPES_H
#define TOASTER_CROSS_PLATFORM_TYPES_H


#include "stdint.h"

#define IPV4_MULTICAST_ADDR  "224.224.224.224"

#define LOG_FILE_NAME "RLD%04d.TXT"
#define LOG_NFO_FILE_NAME "CLS%04d.TXT"

#define PERFECT   0
#define TOO_EARLY 1
#define TOO_LATE  2
#define BAD_DATA  3

#define PERFECT_WINDNOW_100MS 50

#ifndef TCS3472X_COLOR_SENSOR_H
typedef struct __attribute__((packed))
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t c;
} tcs3472x_color_t;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t c;
} tcs3472x_color8_t;
#endif

//#define LOG_ENTRY_HEADER_INIT() {0x55, 0x00, 0x55, 0x00, 0x55}
//typedef struct __attribute__((packed))
//{
//    uint8_t identifier0;
//    uint8_t ctr0; //valid range 0-127;
//    uint8_t identifier1;
//    uint8_t ctr1;
//    uint8_t idefnitifer2;
//} log_entry_headery;

typedef struct __attribute__((packed))
{
    int16_t pt1000a[10];
    int16_t pt1000b[10];
    int16_t board[10];
    int16_t freq0[10];
    int16_t freq1[10];
    int16_t freq2[10];
    tcs3472x_color_t col;
    int16_t a[4];
    int32_t weight;
    uint16_t mlx_object;
    uint16_t mlx_ambient;
    uint16_t aq_TVOC;
    uint16_t aq_eCO2;
    uint16_t aq_rawH2;
    uint16_t aq_rawEthanol;
    int32_t sht_t;
    int32_t sht_h;
    float smoke_ir;
    float smoke_uv;
} log_entry_100ms_t;

#define CMD_SET_DL_RANGE 0x01
#define CMD_CONTINUE_DL  0x02
#define CMD_REPEAT_DL    0x03
#define CMD_DL_MODE_ALIVE 0x04
#define CMD_DL_FILE_DONE  0x05

typedef struct __attribute__((packed))
{
    uint8_t cmd;
    uint16_t arg1;
    uint16_t arg2;
} toaster_cmd_t;

#define LOG_ENTRY_TRANSPORT_STRUCT_IDENTIFIER 0x55
typedef struct __attribute__((packed))
{
    uint8_t identifier;
    uint16_t file_idx;
    uint16_t idx;
    log_entry_100ms_t payload;;
} log_entry_100ms_transport_t;

typedef struct __attribute__((packed))
{
uint16_t after_ir_on;
uint16_t ir[3];
uint16_t after_all_off;
uint16_t all_off;
uint16_t after_uv_on;
uint16_t uv[3];
} log_entry_pd_100ms_t; //photo diode

typedef struct __attribute__((packed))
{
    uint16_t file_index;
    uint16_t duration_in_100msec;
    uint16_t user_code;
    uint16_t rating;
    uint16_t secs;
    uint16_t toasts_since_startup;


    int32_t nau_raw_val_up; // read value of 24bit adc on load cell, servo up
    int32_t nau_raw_val_down; // read value of 24bit adc on load cell, servo down
    /*
     * WELL_DONE -> SECS: HOW MANY SECS BEFORE WOULD THIS HAVE BEEN ALSO PERFECT ( WILL MARK THE LAST X SECONDS PERFECT, ALL BEFORE AS TOO_EARLY)
     * TOO_EARLY -> SECS - NO MEANING
     * TOO_LATE -> SECS - HOW MANY SECS BEFOR WAS IT PERFECT ( THIS WILL MARK THE 10 SECS BEFORE AND 5 AFTER as WELL DONE
     */
} log_nfo_rating_t;

#define LOG_NFO_RATING_TRANSPORT_STRUCT_IDENTIFIER 0x56
typedef struct __attribute__((packed))
{
    uint8_t identifier;
    uint16_t file_idx;
    log_nfo_rating_t payload;;
} log_nfo_rating_transport_t;


extern float convert_pt1000(int16_t* adc_val);
extern float convert_lmt86lp_raw(int16_t adc_val);
extern float convert_mlx(int16_t raw_val);
extern float convert_tmp23x(int16_t raw);
extern void get_smoke_percent(log_entry_pd_100ms_t *log, float* uv_smoke, float* ir_smoke);
extern uint32_t sensirion_calc_absolute_humidity( const int32_t *temperature, const int32_t *humidity);

extern void printClassData( log_nfo_rating_t* nfo_rating);

#endif //TOASTER_CROSS_PLATFORM_TYPES_H
