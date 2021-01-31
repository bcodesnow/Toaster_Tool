#include "toaster_tool.h"


int parseClassificationData(log_nfo_rating_t *nfo_rating, uint64_t id)
{
    char fname[16];
    struct stat st = {0};

    sprintf(&fname[0], (LOG_NFO_FILE_NAME), id);
    //printf("...%s\n",fname);
    if( access( fname, R_OK ) != -1 )
    {
        stat(fname, &st);
        int size = st.st_size;
        if ( size != sizeof ( log_nfo_rating_t ))
        {
            printf("ERR - Size - parseClassificationData! \n");
            return 1;
        }
        else
        {
            FILE* f = fopen( &fname[0], "r");
            if ( fread( nfo_rating, sizeof (log_nfo_rating_t), 1, f) == 1 )
            {
                if ( nfo_rating->rating == BAD_DATA)
                {
                    printf("ERR - BAD DATA - parseClassificationData \n");
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
    }
    printf("ERR - Access NOT OK! -parseClassificationData \n");
    return 1;
}

void add_header2file(FILE* fil_out, measurement_t type)
{
    switch(type)
    {
    case MEAS_SHT_T:
        fprintf(fil_out,"SHT_T");
        break;
    case MEAS_SHT_H:
        fprintf(fil_out,"SHT_H");
        break;
    case MEAS_MLX_AMBIENT:
        fprintf(fil_out,"MLX_TO");
        break;
    case MEAS_MLX_OBJECT:
        fprintf(fil_out,"MLX_TA");
        break;
    case MEAS_PT1000A:
        fprintf(fil_out,"PT1000A");
        break;
    case MEAS_PT1000B:
        fprintf(fil_out,"PT1000B");
        break;
    case MEAS_FREQ0:
    case MEAS_FREQ1:
    case MEAS_FREQ2:
        /* Todo if the sensors are finally connected */
        fprintf(fil_out,"MEAS_FREQ");
        break;
    case MEAS_TVOC:
        fprintf(fil_out,"MEAS_TVOC");
        break;
    case MEAS_eCO2:
        fprintf(fil_out,"MEAS_eCO2");
        break;
    case MEAS_rawH2:
        fprintf(fil_out,"MEAS_rawH2");
        break;
    case MEAS_rawEthanol:
        fprintf(fil_out,"MEAS_rawEthanol");
        break;
    case MEAS_weight:
        fprintf(fil_out,"MEAS_weight");
        break;
    case MEAS_COL_R:
        fprintf(fil_out,"MEAS_COL_R");
        break;
    case MEAS_COL_G:
        fprintf(fil_out,"MEAS_COL_G");
        break;
    case MEAS_COL_B:
        fprintf(fil_out,"MEAS_COL_B");
        break;
    case MEAS_COL_C:
        fprintf(fil_out,"MEAS_COL_C");
        break;
    case MEAS_A0:
    case MEAS_A1:
    case MEAS_A2:
    case MEAS_A3:
        /* Todo if the sensors are finally connected */
        fprintf(fil_out,"MEAS_A_%d", type- MEAS_A0);
        break;
    case MEAS_BOARD:
        fprintf(fil_out,"MEAS_BOARD");
        break;
    case MEAS_SMOKE_IR:
        fprintf(fil_out,"MEAS_IR");
        break;
    case MEAS_SMOKE_UV:
        fprintf(fil_out,"MEAS_UV");
        break;
    case MEAS_ABS_HUMI:
        fprintf(fil_out,"ABS_HUMI");
        break;
    }
}


int16_t get_i16_avg(int16_t *start_ptr, uint16_t len)
{
    int64_t sum = 0;
    for (int i=0; i<len; i++)
        sum += *(start_ptr++);
    return (sum / len);
}

int16_t get_freq(int16_t *start_ptr, uint16_t len)
{
    int64_t sum = 0;
    for (int i=0; i<len; i++)
        sum += *(start_ptr++);
    return (sum); // each freq value represents the pulses in 10ms.. 10 of them 100ms, 10x100ms=1sec -> ==
}

void check_and_fix_min_max(float *min, float *max, char *string)
{

    bool invalid = false;
    if ( (*min == 0) && (*max == 0) )
    {
        invalid = true;
        printf("WARN... both 0! %s\n", string);
    }
    else if ( *min == *max)
    {
        invalid = true;
        printf("... equal %s\n", string);
    }
    if (invalid)
    {
        printf("WARN... fmin:%.2f,fmax:%.2f fixed! \n", *min ,*max);
        *min = 0.0f;
        *max = 1.0f;
    }
}

void remove_last_x_conv(uint32_t x)
{
    for ( int i=0; i< x; i++)
    {
        log_conv.pt1000a.pop_front();
        log_conv.pt1000b.pop_front();
        log_conv.board.pop_front();
        log_conv.t0.pop_front();
        log_conv.t1.pop_front();
        log_conv.t2.pop_front();
        log_conv.t3.pop_front();
        log_conv.weight.pop_front();
        log_conv.mlx_object.pop_front();
        log_conv.mlx_ambient.pop_front();
        log_conv.sht_t.pop_front();
        log_conv.sht_h.pop_front();
        log_conv.freq0.pop_front();
        log_conv.freq1.pop_front();
        log_conv.freq2.pop_front();
        log_conv.r.pop_front();
        log_conv.g.pop_front();
        log_conv.b.pop_front();
        log_conv.c.pop_front();
        log_conv.aq_TVOC.pop_front();
        log_conv.aq_eCO2.pop_front();
        log_conv.aq_rawH2.pop_front();
        log_conv.aq_rawEthanol.pop_front();
        log_conv.smoke_ir.pop_front();
        log_conv.smoke_uv.pop_front();
        log_conv.sht_abs_humi.pop_front();
    }
}

// Simple 3 Point Running AVG and Outlier Neckoff
//#define SMTH_BUF_SIZE 3
typedef struct
{
    uint64_t newVal;
    uint32_t currVal;
    uint32_t lastVal;
    uint64_t idx;
    uint64_t buff[3];
    float outLierTr;
} smthdAvg_t;

void getIntSmoothedAvg( smthdAvg_t* t)
{
//    printf("newValPre: %d\n", t->newVal);

    t->lastVal = t->currVal;
    if ( t->idx == 0)
    {
        t->buff[0] = t->newVal;
        t->currVal = t->newVal;
    }
    else
    {
        if ( (t->newVal *1.0f) > t->currVal * t->outLierTr )
        {
            if ( t->lastVal < t->currVal )
                t->newVal = ( ( t->currVal + (t->currVal-t->lastVal) ) + ( t->buff[0] + t->buff[1] + t->buff[2] ) ) / 4;
            else
                t->newVal = t->currVal;
        }
        else if ( (t->newVal * 1.0f) < ( t->currVal * ( 2.0f - t->outLierTr) ) )
        {
            if ( t->lastVal > t->currVal )
                t->newVal = ( ( t->currVal - (t->lastVal - t->currVal) ) + ( t->buff[0] + t->buff[1] + t->buff[2] ) ) / 4;
            else
                t->newVal = t->currVal;
        }

        if ( t->idx == 1)
        {
            t->buff[1] = t->newVal;
            t->currVal = ( t->buff[0] + t->buff[1] ) / 2;
        }
        else if ( t->idx == 2)
        {
            t->buff[2] = t->newVal;
            t->currVal = ( t->buff[0] + t->buff[1] + t->buff[2] ) / 3;
        }
        else
        {
            t->buff[t->idx%3] = t->newVal;
            t->currVal = ( t->buff[0] + t->buff[1] + t->buff[2] ) / 3;
        }
    }
//    printf("IDX: %d\n", t->idx);
//    printf("BUF0: %d\n", t->buff[0]);
//    printf("BUF1: %d\n", t->buff[1]);
//    printf("BUF2: %d\n", t->buff[2]);
//    printf("currVal: %d\n", t->currVal);
//    printf("outlier %d\n",  (int)(t->currVal * t->outLierTr));

//    printf("newValPost: %d\n", t->newVal);


}

void add_meas( log_entry_100ms_t* entry, measurement_t type, uint64_t flags, uint64_t idx )
{
    int16_t i16_helper;
    FILE * f;

    static smthdAvg_t f1Smth, f2Smth;

    // Choose Destination
    if ( flags & FJOB_PIPE_TO_PYTHON )
        f = fdWrite;
    else
        f = fil_out;


    switch(type)
    {
    case MEAS_SHT_T:
        if ( flags & FJOB_FORMAT_RAW)
        {
            fprintf(f,"%i", (entry->sht_t) );
        }
        else if ( flags & FJOB_FORMAT_CONVERTED)
        {
            fprintf(f,"%f", (float) (entry->sht_t / 1000.0f) );
        }
        if ( flags & FJOB_PREPARE_PLOT )
        {
            log_conv.sht_t.push_back( (float) (entry->sht_t / 1000.0f) );
        }
        break;
    case MEAS_SHT_H:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", (entry->sht_h) );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", (float) (entry->sht_h / 1000.0f) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.sht_h.push_back( (float) (entry->sht_h / 1000.0f) );
        break;
    case MEAS_MLX_AMBIENT:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->mlx_ambient );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_mlx(entry->mlx_ambient) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.mlx_ambient.push_back( convert_mlx(entry->mlx_ambient)  );
        break;
    case MEAS_MLX_OBJECT:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->mlx_object );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_mlx(entry->mlx_object) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.mlx_object.push_back( convert_mlx(entry->mlx_object)  );
        break;
    case MEAS_PT1000A:
        i16_helper = get_i16_avg( entry->pt1000a, 10);
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", i16_helper );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_pt1000(&i16_helper) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.pt1000a.push_back( convert_pt1000(&i16_helper) );
        break;
    case MEAS_PT1000B:
        i16_helper = get_i16_avg( entry->pt1000b, 10);
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", i16_helper );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_pt1000(&i16_helper) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.pt1000b.push_back( convert_pt1000(&i16_helper) );
        break;
    case MEAS_FREQ0:
        i16_helper = get_freq( entry->freq0, 10);
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
        {
            fprintf(f,"%i", i16_helper );
        }
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.freq0.push_back( i16_helper );
        break;
    case MEAS_FREQ1:
        if (flags & FJOB_SMOOTH_FREQS)
        {
            f1Smth.idx = idx;
            f1Smth.outLierTr = 1.5;
            f1Smth.newVal = get_freq( entry->freq1, 10);
            getIntSmoothedAvg(&f1Smth);
            i16_helper = f1Smth.currVal;
        }
        else
        {
            i16_helper = get_freq( entry->freq1, 10);
        }
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
        {
            fprintf(f,"%i", i16_helper );
        }
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.freq1.push_back( i16_helper );
        break;
    case MEAS_FREQ2:
        if (flags & FJOB_SMOOTH_FREQS)
        {
            f2Smth.idx = idx;
            f2Smth.outLierTr = 1.001;
            f2Smth.newVal = get_freq( entry->freq2, 10);
            getIntSmoothedAvg(&f2Smth);
            i16_helper = f2Smth.currVal;
        }
        else
        {
        i16_helper = get_freq( entry->freq2, 10);
        }
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
        {
            fprintf(f,"%i", i16_helper );
        }
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.freq2.push_back( i16_helper );
        break;
    case MEAS_TVOC:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->aq_TVOC );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.aq_TVOC.push_back( entry->aq_TVOC );
        break;
    case MEAS_eCO2:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->aq_eCO2 );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.aq_eCO2.push_back( entry->aq_eCO2 );
        break;
    case MEAS_rawH2:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->aq_rawH2 );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.aq_rawH2.push_back( entry->aq_rawH2 );
        break;
    case MEAS_rawEthanol:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->aq_rawEthanol );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.aq_rawEthanol.push_back( entry->aq_rawEthanol );
        break;
    case MEAS_weight:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->weight );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.weight.push_back( (float) ( entry->weight / 1000.0f) );
        break;
    case MEAS_COL_R:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->col.r );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.r.push_back( entry->col.r );
        break;
    case MEAS_COL_G:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->col.g );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.g.push_back( entry->col.g );
        break;
    case MEAS_COL_B:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->col.b );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.b.push_back( entry->col.b );
        break;
    case MEAS_COL_C:
        if ( ( flags & FJOB_FORMAT_CONVERTED ) || (flags & FJOB_FORMAT_RAW) )
            fprintf(f,"%i", entry->col.c );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.c.push_back( entry->col.c );
        break;
    case MEAS_A0:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->a[0] );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_lmt86lp_raw(entry->a[0]) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.t0.push_back( convert_lmt86lp_raw(entry->a[0]) );
        break;
    case MEAS_A1:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->a[1] );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_lmt86lp_raw(entry->a[1]) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.t1.push_back( convert_lmt86lp_raw(entry->a[1]) );
        break;
    case MEAS_A2:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->a[2] );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_lmt86lp_raw(entry->a[2]) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.t2.push_back( convert_lmt86lp_raw(entry->a[2]) );
        break;
    case MEAS_A3:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", entry->a[3] );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_lmt86lp_raw(entry->a[3]) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.t3.push_back( convert_lmt86lp_raw(entry->a[3]) );
        break;
    case MEAS_BOARD:
        if ( flags & FJOB_FORMAT_RAW)
            fprintf(f,"%i", get_i16_avg( entry->board, 10) );
        else if ( flags & FJOB_FORMAT_CONVERTED)
            fprintf(f,"%f", convert_tmp23x( get_i16_avg( entry->board, 10) ) );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.board.push_back( convert_tmp23x( get_i16_avg( entry->board, 10)) );
        break;
    case MEAS_SMOKE_IR:
        if ( ( flags & FJOB_FORMAT_RAW ) || ( flags & FJOB_FORMAT_CONVERTED ) )
            fprintf(f,"%f", entry->smoke_ir );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.smoke_ir.push_back( entry->smoke_ir );
        break;
    case MEAS_SMOKE_UV:
        if ( ( flags & FJOB_FORMAT_RAW ) || ( flags & FJOB_FORMAT_CONVERTED ) )
            fprintf(f,"%f", entry->smoke_uv );
        if ( flags & FJOB_PREPARE_PLOT )
            log_conv.smoke_uv.push_back( entry->smoke_uv );
        break;
    case MEAS_ABS_HUMI:
    {
        int32_t val_t = entry->sht_t;
        int32_t val_h = entry->sht_h;
        uint32_t val_abs_h = sensirion_calc_absolute_humidity( &val_t, &val_h );

        if ( ( flags & FJOB_FORMAT_RAW ) || ( flags & FJOB_FORMAT_CONVERTED ) )
            fprintf(f,"%lu",val_abs_h );
        if ( flags & FJOB_PREPARE_PLOT )
        {
            log_conv.sht_abs_humi.push_back( val_abs_h );
        }
    }
        break;
    default:
        printf("Some Unknown Measurement Landed! \n");
        break;
    }
}



int creatNewFifo(const char *fifoName)
{
    struct stat stats;
    if ( stat( fifoName, &stats ) < 0 )
    {
        if ( errno != ENOENT )          // ENOENT is ok, since we intend to delete the file anyways
        {
            perror( "stat failed" );    // any other error is a problem
            return( -1 );
        }
    }
    else                                // stat succeeded, so the file exists
    {
        if ( unlink( fifoName ) < 0 )   // attempt to delete the file
        {
            perror( "unlink failed" );  // the most likely error is EBUSY, indicating that some other process is using the file
            return( -1 );
        }
    }

    if ( mkfifo( fifoName, 0666 ) < 0 ) // attempt to create a brand new FIFO
    {
        perror( "mkfifo failed" );
        return( -1 );
    }
    printf("...Creating Fifo: %s, success! \n", fifoName);
    return( 0 );
}

void writeClassDataToFile(FILE *f, log_nfo_rating_t *nfo_rating)
{
    // Header
    fprintf(f, "RATING, RATING_STR, RATING_SECS, DURATION, T_SINCE_START, NAU_UP, NAU_DOWN, U_TEMP_C, U_HEATER_MODE, U_TEMP_S, U_UCODE\n" );
    uint8_t temp_source = ( nfo_rating->user_code >> 14 ) & 0x3;
    uint8_t heater_mode = ( nfo_rating->user_code >> 10 ) & 0x3;
    uint8_t temp = nfo_rating->user_code & 0xFF;

    // Raw Starts
    fprintf(f,"%d,", nfo_rating->rating);
    switch (nfo_rating->rating)
    {
    case PERFECT:
        fprintf(f, "PERFECT");
        break;
    case TOO_LATE:
        fprintf(f, "TOO_LATE");
        break;
    case TOO_EARLY:
        fprintf(f,"TOO_EARLY");
        break;
    case BAD_DATA:
        fprintf(f,"BAD_DATA");
    default:
        fprintf(f,"UNKNOWN %d", nfo_rating->rating);
        break;
    }
    fprintf(f,",");

    fprintf(f, "%d,", nfo_rating->secs);
    fprintf(f, "%d,", nfo_rating->duration_in_100msec);
    fprintf(f, "%d,", nfo_rating->toasts_since_startup);
    fprintf(f, "%d,", nfo_rating->nau_raw_val_up);
    fprintf(f, "%d,", nfo_rating->nau_raw_val_down);
    fprintf(f, "%d,", temp);
    fprintf(f, "%d,", heater_mode);
    fprintf(f, "%d,", temp_source);
    fprintf(f, "%d\n", nfo_rating->user_code);

}
