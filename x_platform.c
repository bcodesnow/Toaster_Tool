#include "x_platform.h"

#define DEBUG_OUT_SMOKE 0

#include "stdio.h"


#define R0 1000.0f
#define V_SUP 3.3f
#define C_A 3.9083E-3f
#define C_B 5775E-7f
float convert_pt1000(int16_t* adc_val)
{
    float gain_val = 2.048f;
    float voltage = gain_val / 0x7fff * (*adc_val);
    float rt; // the resistor of interest
    float t;
    // Wheatstone Bridge
    rt =  ( ( R0 * V_SUP ) + ( 2.0f * R0 * voltage )  ) / ( (R0 * V_SUP) - (2.0f * R0 * voltage) ) * 1000;

    // simplified for the 0-100C Range (its for gui only, we record the raw data)
    t = (rt-R0) / ( R0 * C_A);
    return t;
    //    float T;
    //    R=R/R0;
    //    T=0.0-A;
    //    T+=sqrt((A*A) - 4.0 * B * (1.0 - R));
    //    T/=(2.0 * B);
    //    if(T>0&&T<200) {
    //      return T;
    //    }
    //    else {
    //      //T=  (0.0-A - sqrt((A*A) - 4.0 * B * (1.0 - R))) / 2.0 * B;
    //      T=0.0-A;
    //      T-=sqrt((A*A) - 4.0 * B * (1.0 - R));
    //      T/=(2.0 * B);
    //      return T;
    //    }
}

float convert_mlx(int16_t raw_val)
{
    double temp;
    temp = raw_val * 0.02;
    return (float)(temp - 273.15);
}

float convert_tmp23x(int16_t raw)
{
#define TMP236X_VOFFS_BELOW_2350MV 400.0f
#define TMP236X_VOFFS_ABOVE_2350MV 2350.0f
#define TMP236X_TINFL_BELOW_2350MV 0.0f
#define TMP236X_TINFL_ABOVE_2350MV 100.0f

    float gain_val = 2.048f;
    float voltage = gain_val / 0x7fff * raw;
    //return ( ( ( voltage * 1000 ) - TMP236X_TINFL_ABOVE_2350MV ) / 19.5f ) + TMP236X_TINFL_ABOVE_2350MV;
    return ( ( voltage*1000 ) - TMP236X_VOFFS_BELOW_2350MV ) / 19.5f;

}

#define T0 20
#define T1 150
#define V0 1885
#define V1 420

float convert_lmt86lp(float voltage)
{
    return ( ( voltage - V1) * (T1 - T0)  / (V1 - V0) ) + T1; // double - 4 usec, float 1 usec
    //    return (float) ( ( 10.888 - sqrt ( pow(-10.888, 2.0) + 4.0 * 0.00347 * (1777.3 - voltage) ) ) / 2.0 * (0.0-0.00347) ) + 30.0; //7usec and it does not work

}


float convert_lmt86lp_raw(int16_t val)
{
    float gain_val = 4.096f;
    float voltage = gain_val / 0x7fff * val * 1000;
    return ( ( voltage - V1) * (T1 - T0)  / (V1 - V0) ) + T1; // double - 4 usec, float 1 usec
    //    return (float) ( ( 10.888 - sqrt ( pow(-10.888, 2.0) + 4.0 * 0.00347 * (1777.3 - voltage) ) ) / 2.0 * (0.0-0.00347) ) + 30.0; //7usec and it does not work

}


#define BUFFER_SIZE 0xFF
void get_smoke_avg(log_entry_pd_100ms_t *log, uint16_t* uv, uint16_t* ir, uint16_t* uv_lt, uint16_t* ir_lt)
{
    static uint16_t lt_ir[BUFFER_SIZE];
    static uint16_t lt_uv[BUFFER_SIZE];
    static uint32_t idx = 0, i, k;

    int16_t calc_ir = 0, calc_uv = 0;

    for ( i=0; i<3; i++ )
    {
        calc_uv+= log->uv[i];
        calc_ir+= log->ir[i];
    }

    calc_ir = ( calc_ir * 10 ) / 3;
    calc_uv = ( calc_uv * 10 ) / 3;

    *ir = lt_ir[ (idx & BUFFER_SIZE) ] = calc_ir;
    *uv = lt_uv[ (idx++ & BUFFER_SIZE) ] = calc_uv;

    calc_ir = calc_uv = 0;
    k = ( idx < BUFFER_SIZE ) ? idx : BUFFER_SIZE;
    for ( i=0; i<k; i++ )
    {
        calc_uv+= lt_uv[i];
        calc_ir+= lt_ir[i];
    }
    *ir_lt = ( calc_ir * 10 ) / k;
    *uv_lt = ( calc_uv * 10 ) / k;
}


void get_smoke_percent(log_entry_pd_100ms_t *log, float* uv_smoke, float* ir_smoke)
{

#if DEBUG_OUT_SMOKE
    uint16_t* freq = (uint16_t*) log;
    for (int i=0; i<10; i++)
        printf("I: %d, VAL: %d \n", i, freq[i]);
#endif
    static uint32_t i;
    static uint16_t ir_min = 0xFFF, uv_min = 0xFFF, baseline_min = 0xFFF;

    float ir_avg = 0, uv_avg = 0;

    for ( i=0; i<3; i++ )
    {
        uv_avg+= log->uv[i];
        ir_avg+= log->ir[i];
        if ( log->uv[i] < uv_min )
            uv_min = log->uv[i];
        if ( log->ir[i] < ir_min )
            ir_min = log->ir[i];
    }
    if (log->all_off < baseline_min)
        baseline_min = log->all_off;
#if DEBUG_OUT_SMOKE
    printf("|IR_MIN:%d|UV_MiN:%d|ALLOFF_MIN:%d\n", ir_min, uv_min, log->all_off);
#endif
    ir_avg = ( ir_avg  / 3.0f ) - log->all_off;
    uv_avg = ( uv_avg  / 3.0f ) - log->all_off;

    *uv_smoke =  (( (float) (uv_avg) / (uv_min - log->all_off) ) - 1.0f ) * 100.0f;
    *ir_smoke =  (( (float) (ir_avg) / (ir_min - log->all_off) ) - 1.0f ) * 100.0f;
}


/* SHT - ABS HUMI */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif


#define T_LO (-20000)
#define T_HI 70000

static const uint32_t AH_LUT_100RH[] = {1078,  2364,  4849,  9383,   17243,
                                        30264, 50983, 82785, 130048, 198277};

static const uint32_t T_STEP = (T_HI - T_LO) / (ARRAY_SIZE(AH_LUT_100RH) - 1);


uint32_t sensirion_calc_absolute_humidity(const int32_t *temperature, const int32_t *humidity)
{
    uint32_t t, i, rem, ret;

    if (*humidity <= 0)
        return 0;

    if (*temperature < T_LO)
        t = 0;
    else
        t = (uint32_t)(*temperature - T_LO);

    i = t / T_STEP;
    rem = t % T_STEP;

    if (i >= ARRAY_SIZE(AH_LUT_100RH) - 1) {
        ret = AH_LUT_100RH[ARRAY_SIZE(AH_LUT_100RH) - 1];

    } else if (rem == 0) {
        ret = AH_LUT_100RH[i];

    } else {
        ret = (AH_LUT_100RH[i] +
               ((AH_LUT_100RH[i + 1] - AH_LUT_100RH[i]) * rem / T_STEP));
    }

    // Code is mathematically (but not numerically) equivalent to
    //    return (ret * (*humidity)) / 100000;
    // Maximum ret = 198277 (Or last entry from AH_LUT_100RH)
    // Maximum *humidity = 119000 (theoretical maximum)
    // Multiplication might overflow with a maximum of 3 digits
    // Trick: ((ret >> 3) * (uint32_t)(*humidity)) does never overflow
    // Now we only need to divide by 12500, as the tripple righ shift
    // divides by 8

    return ((ret >> 3) * (uint32_t)(*humidity)) / 12500;
}

void printClassData(log_nfo_rating_t *nfo_rating)
{
    uint8_t temp_source = ( nfo_rating->user_code >> 14 ) & 0x3;
    uint8_t heater_mode = ( nfo_rating->user_code >> 10 ) & 0x3;
    uint8_t temp = nfo_rating->user_code & 0xFF;

    printf("******** NFO LOG ******** \n");
    printf("*** File Index: %d  *** \n", nfo_rating->file_index);
    switch (nfo_rating->rating)
    {
    case PERFECT:
        printf("*** Rating: Well Done *** \n");
        break;
    case TOO_LATE:
        printf("*** Rating: Too Late *** \n");
        break;
    case TOO_EARLY:
        printf("*** Rating: Too Early *** \n");
        break;
    case BAD_DATA:
        printf("*** Rating: Bad Robot *** \n");
    default:
        printf("*** Rating: %d *** \n", nfo_rating->rating);
        break;
    }

    printf("*** Secs ( Rating ): %d  *** \n", nfo_rating->secs);
    printf("*** Secs ( Duration ): %f  *** \n", nfo_rating->duration_in_100msec / 10.0);
    printf("*** Nau Up: %d  *** \n", nfo_rating->nau_raw_val_up);
    printf("*** Nau Down: %d  *** \n", nfo_rating->nau_raw_val_down);
    printf("*** Toasts Since Start Up: %d  *** \n", nfo_rating->toasts_since_startup);
    printf("*** User Code: %d  *** \n", nfo_rating->user_code);
    printf("*** Temp: %d  *** \n", temp);
    printf("*** Heater Mode: %d  *** \n", heater_mode);
    printf("*** Temp Source: %d  *** \n", temp_source);
    printf("************************* \n");
}
