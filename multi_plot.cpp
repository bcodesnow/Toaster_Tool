#include "multi_plot.h"


void multi_plot( fjob_t* job )
{
    uint16_t id = job->id;
    static int call_count = 0;
    int i, j;
    if (!log_conv.status)
    {
        printf("Error - Data uninitialized for plotting!\n");
        exit(0);
    }
    if ( call_count == 0)
    {
        if (gp == nullptr)
            gp = popen(GNUPLOT, "w");	/* 'gp' is the GNUPLOT pipe descriptor */
        if (gp == nullptr)
        {
            printf("Error opening pipe to GNU plot.\n");
            exit(0);
        }

        if ( job->flags & FJOB_USE_PNG_TERMINAL )
        {
            gcmd("set terminal pngcairo size 2460,1920 \n");
        }
        else
        {
        gcmd("set term wxt title \"Sensor Data\" \n");

        gcmd("set terminal wxt size 2460,1920 \n");
        }

    }
    else
    {
        gcmd("clear \n");
    }

    call_count++;
    uint64_t len = log_conv.sht_h.size();

    if ( job->flags & FJOB_USE_PNG_TERMINAL)
        gcmd("set output \"%d.png\" \n", job->id);

    /* Plot #0 - Temp SHT, MLX */
    gcmd("set size 1,1\n");
    gcmd("set multiplot layout 5,3\n"); /*columnsfirst scale 1.1,0.9*/
    gcmd("set font \"Roboto,12\" \n");
    gcmd("set grid\n");

    // TEMP SHT - Filled Curves
    gcmd("set title \"SHT31 Temperature\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Temperature / º C \" \n");

    float f_min = *std::min_element(log_conv.sht_t.begin(), log_conv.sht_t.end());
    float f_max = *std::max_element(log_conv.sht_t.begin(), log_conv.sht_t.end());
    gcmd("set label 3 \"File Id: %d \" at screen 0.01,0.01 font \"Roboto,14\" \n", id);

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set style data filledcurves\n");
    //gcmd("set datafile separator ','; plot '-' using 1:2 with lines, '' using 1:3 with lines \n");
    gcmd("set datafile separator ','; plot '-' using 1:2 lt rgb 'red' \n");
    // column names
    gcmd("%s,%s\n", "Time","SHT31T");
    // filled curves first point
    gcmd("%d,%f\n", 0, 0.0f);
    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%f\n", i, log_conv.sht_t.at(i));
    }
    // filled curves last point
    gcmd("%d,%f\n", len, 0.0f);
    gcmd("e;\n");
    gcmd("unset label 3 \n");

    // HUMIDITY SHT - Filled Curves
    gcmd("set title \"SHT31 Humidity\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Humidity / % \" \n");
    f_min = *std::min_element(log_conv.sht_h.begin(), log_conv.sht_h.end());
    f_max = *std::max_element(log_conv.sht_h.begin(), log_conv.sht_h.end());
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set style data filledcurves\n");
    //gcmd("set datafile separator ','; plot '-' using 1:2 with lines, '' using 1:3 with lines \n");
    gcmd("set datafile separator ','; plot '-' using 1:2 lt rgb 'blue' \n");
    // column names
    gcmd("%s,%s\n", "Time","SHT31H");
    // filled curves first point
    gcmd("%d,%f\n", 0, 0.0f);
    // data
    for(i=0; i<len; i++)
    {
        //printf("%d,%d,%d\n", i, log_conv.aq_TVOC.at(i), log_conv.aq_eCO2.at(i));
        gcmd("%d,%f\n", i, log_conv.sht_h.at(i));
        //gcmd("%d,%d,%d\n", i, log_conv.aq_TVOC.at(i), log_conv.aq_eCO2.at(i));
        //printf("%d %d %d\n", i, log_conv.aq_TVOC.at(i), log_conv.aq_eCO2.at(i));

    }
    // filled curves last point
    gcmd("%d,%f\n", len, 0.0f);

    gcmd("e;\n");

    // AQ raw Eth, raw h2
    uint8_t data_col_count = 2;
    std::vector<int>i_minimas;
    std::vector<int>i_maximas;
    gcmd("set title \"SGP30 Air Quality\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \" raw \n");
    i_minimas.push_back( *std::min_element(log_conv.aq_rawH2.begin(), log_conv.aq_rawH2.end()) );
    i_minimas.push_back( *std::min_element(log_conv.aq_rawEthanol.begin(), log_conv.aq_rawEthanol.end()) );
    i_maximas.push_back( *std::max_element(log_conv.aq_rawH2.begin(), log_conv.aq_rawH2.end()) );
    i_maximas.push_back( *std::max_element(log_conv.aq_rawEthanol.begin(), log_conv.aq_rawEthanol.end()) );
    f_min = *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "AQ RAW");
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 with lines, '' using 1:3 with lines \n");
    for (j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s\n", "Time","rawH2","rawEthanol");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%d,%d\n", i, log_conv.aq_rawH2.at(i), log_conv.aq_rawEthanol.at(i) );
        }
        gcmd("e;\n");
    }

    // AQ TVOC, Eco2
    data_col_count = 2;
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \"SGP30 Air Quality\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Q / ppm \" \n");
    i_minimas.push_back( *std::min_element(log_conv.aq_TVOC.begin(), log_conv.aq_TVOC.end()) );
    i_minimas.push_back( *std::min_element(log_conv.aq_eCO2.begin(), log_conv.aq_eCO2.end()) );
    i_maximas.push_back( *std::max_element(log_conv.aq_TVOC.begin(), log_conv.aq_TVOC.end()) );
    i_maximas.push_back( *std::max_element(log_conv.aq_eCO2.begin(), log_conv.aq_eCO2.end()) );
    f_min = *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "AQ");
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 with lines, '' using 1:3 with lines \n");
    for (j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s\n", "Time","TVOC","eCO2");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%d,%d\n", i, log_conv.aq_TVOC.at(i), log_conv.aq_eCO2.at(i) );
        }
        gcmd("e;\n");
    }

#if ADD_C_TO_COLOR_PLOT == 1
    //COLOR RGBC
    data_col_count = 4;
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \"Color Sensor\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Intensity \" \n");
    i_minimas.push_back( *std::min_element(log_conv.r.begin(), log_conv.r.end()) );
    i_minimas.push_back( *std::min_element(log_conv.g.begin(), log_conv.g.end()) );
    i_minimas.push_back( *std::min_element(log_conv.b.begin(), log_conv.b.end()) );
    i_minimas.push_back( *std::min_element(log_conv.c.begin(), log_conv.c.end()) );
    i_maximas.push_back( *std::max_element(log_conv.r.begin(), log_conv.r.end()) );
    i_maximas.push_back( *std::max_element(log_conv.g.begin(), log_conv.g.end()) );
    i_maximas.push_back( *std::max_element(log_conv.b.begin(), log_conv.b.end()) );
    i_maximas.push_back( *std::max_element(log_conv.c.begin(), log_conv.c.end()) );
    f_min = *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "RGB");

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");

    gcmd("set style line 1 linecolor rgb '#FF0000' linewidth 3\n");
    gcmd("set style line 2 linecolor rgb '#00FF00' linewidth 3\n");
    gcmd("set style line 3 linecolor rgb '#0000FF' linewidth 3\n");
    gcmd("set style line 4 linecolor rgb '#FFFF00' linewidth 3 dashtype 3\n"); // yellow
    gcmd("set style line 5 linecolor rgb '#5032a8' linewidth 3\n"); //lila
    gcmd("set style line 6 linecolor rgb '#182694' linewidth 3\n"); //
    gcmd("set style line 7 linecolor rgb '#FF0000' linewidth 3 dashtype 3\n");
    gcmd("set style line 8 linecolor rgb '#fb8761' linewidth 3\n"); //

    gcmd("set datafile separator ',' \n");
    gcmd("plot '-' using 1:2 with lines linestyle 1, '' using 1:3 with lines linestyle 2, '' using 1:4 with lines linestyle 3, '' using 1:5 with lines linestyle 4\n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s,%s,%s\n", "Time","Red","Green","Blue","C");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%d,%d,%d,%d\n", i, log_conv.r.at(i),log_conv.g.at(i),log_conv.b.at(i),log_conv.c.at(i) );
        }
        gcmd("e;\n");
    }
#else
    //COLOR RGB
    data_col_count = 3;
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \"Color Sensor\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Intensity \" \n");
    i_minimas.push_back( *std::min_element(log_conv.r.begin(), log_conv.r.end()) );
    i_minimas.push_back( *std::min_element(log_conv.g.begin(), log_conv.g.end()) );
    i_minimas.push_back( *std::min_element(log_conv.b.begin(), log_conv.b.end()) );
    //i_minimas.push_back( *std::min_element(log_conv.c.begin(), log_conv.c.end()) );
    i_maximas.push_back( *std::max_element(log_conv.r.begin(), log_conv.r.end()) );
    i_maximas.push_back( *std::max_element(log_conv.g.begin(), log_conv.g.end()) );
    i_maximas.push_back( *std::max_element(log_conv.b.begin(), log_conv.b.end()) );
    //i_maximas.push_back( *std::max_element(log_conv.c.begin(), log_conv.c.end()) );
    f_min = *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "RGB");

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");

    gcmd("set style line 1 linecolor rgb '#FF0000' linewidth 3\n");
    gcmd("set style line 2 linecolor rgb '#00FF00' linewidth 3\n");
    gcmd("set style line 3 linecolor rgb '#0000FF' linewidth 3\n");
    gcmd("set style line 4 linecolor rgb '#FFFF00' linewidth 3 dashtype 3\n"); // yellow
    gcmd("set style line 5 linecolor rgb '#5032a8' linewidth 3\n"); //lila
    gcmd("set style line 6 linecolor rgb '#182694' linewidth 3\n"); //
    gcmd("set style line 7 linecolor rgb '#FF0000' linewidth 3 dashtype 3\n");
    gcmd("set style line 8 linecolor rgb '#fb8761' linewidth 3\n"); //

    gcmd("set datafile separator ',' \n");
    gcmd("plot '-' using 1:2 with lines linestyle 1, '' using 1:3 with lines linestyle 2, '' using 1:4 with lines linestyle 3 \n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s,%s\n", "Time","Red","Green","Blue");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%d,%d,%d\n", i, log_conv.r.at(i),log_conv.g.at(i),log_conv.b.at(i) );
        }
        gcmd("e;\n");
    }

#endif

    // PT1000A, PT1000B
    data_col_count = 2;
    std::vector<float>f_minimas;
    std::vector<float>f_maximas;
    gcmd("set title \"Temperature PT1000\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Temperature / º C \" \n");
    f_minimas.push_back( *std::min_element(log_conv.pt1000a.begin(), log_conv.pt1000a.end()) );
    f_minimas.push_back( *std::min_element(log_conv.pt1000b.begin(), log_conv.pt1000b.end()) );
    f_maximas.push_back( *std::max_element(log_conv.pt1000a.begin(), log_conv.pt1000a.end()) );
    f_maximas.push_back( *std::max_element(log_conv.pt1000b.begin(), log_conv.pt1000b.end()) );
    f_min = *std::min_element(f_minimas.begin(), f_minimas.end());
    f_max = *std::max_element(f_maximas.begin(), f_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "PT");
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");

    gcmd("set datafile separator ','; plot '-' using 1:2" STYLE_LILA ", '' using 1:3 with lines linestyle 2 \n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s\n", "Time","PT1000A","PT1000B");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%f,%f\n", i, log_conv.pt1000a.at(i), log_conv.pt1000b.at(i) );
        }
        gcmd("e;\n");
    }

    // MLX90614 AMB, OBJ
    data_col_count = 2;
    f_minimas.clear();
    f_maximas.clear();
    gcmd("set title \"Temperature MLX90614\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Temperature / º C \" \n");
    f_minimas.push_back( *std::min_element(log_conv.mlx_object.begin(), log_conv.mlx_object.end()) );
    f_minimas.push_back( *std::min_element(log_conv.mlx_ambient.begin(), log_conv.mlx_ambient.end()) );
    f_maximas.push_back( *std::max_element(log_conv.mlx_object.begin(), log_conv.mlx_object.end()) );
    f_maximas.push_back( *std::max_element(log_conv.mlx_ambient.begin(), log_conv.mlx_ambient.end()) );
    f_min = *std::min_element(f_minimas.begin(), f_minimas.end());
    f_max = *std::max_element(f_maximas.begin(), f_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "MLX");

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2" STYLE_RED ", '' using 1:3" STYLE_BLUE " \n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s\n", "Time","Object","Ambient");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%f,%f\n", i, log_conv.mlx_object.at(i), log_conv.mlx_ambient.at(i) );
        }
        gcmd("e;\n");
    }

    // Analog IN 4X Temp
    data_col_count = 4;
    f_minimas.clear();
    f_maximas.clear();
    gcmd("set title \"Temperatures ADS1115 - LMT86\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Temperature / º C \" \n");
    f_minimas.push_back( *std::min_element(log_conv.t0.begin(), log_conv.t0.end()) );
    f_minimas.push_back( *std::min_element(log_conv.t1.begin(), log_conv.t1.end()) );
    f_minimas.push_back( *std::min_element(log_conv.t2.begin(), log_conv.t2.end()) );
    f_minimas.push_back( *std::min_element(log_conv.t3.begin(), log_conv.t3.end()) );
    f_maximas.push_back( *std::max_element(log_conv.t0.begin(), log_conv.t0.end()) );
    f_maximas.push_back( *std::max_element(log_conv.t1.begin(), log_conv.t1.end()) );
    f_maximas.push_back( *std::max_element(log_conv.t2.begin(), log_conv.t2.end()) );
    f_maximas.push_back( *std::max_element(log_conv.t3.begin(), log_conv.t3.end()) );
    f_min = *std::min_element(f_minimas.begin(), f_minimas.end());
    f_max = *std::max_element(f_maximas.begin(), f_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "Analog");

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");

    gcmd("set datafile separator ','; plot '-' using 1:2 " STYLE_RED ", '' using 1:3 " STYLE_GREEN ", '' using 1:4 " STYLE_BLUE ", '' using 1:5 " STYLE_ORANGE " \n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s,%s,%s\n", "Time","T0","T1","T2","T3");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%f,%f,%f,%f\n", i, log_conv.t0.at(i), log_conv.t1.at(i), log_conv.t2.at(i), log_conv.t3.at(i) );
        }
        gcmd("e;\n");
    }

    // Freq in 3X
    data_col_count = 3;
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \"Freqs\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Frequency / 10 Hz \" \n");
    i_minimas.push_back( *std::min_element(log_conv.freq0.begin(), log_conv.freq0.end()) );
    i_minimas.push_back( *std::min_element(log_conv.freq1.begin(), log_conv.freq1.end()) );
    i_minimas.push_back( *std::min_element(log_conv.freq2.begin(), log_conv.freq2.end()) );

    i_maximas.push_back( *std::max_element(log_conv.freq0.begin(), log_conv.freq0.end()) );
    i_maximas.push_back( *std::max_element(log_conv.freq1.begin(), log_conv.freq1.end()) );
    i_maximas.push_back( *std::max_element(log_conv.freq2.begin(), log_conv.freq2.end()) );

    f_min= *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());

    check_and_fix_min_max(&f_min, &f_max, "Freq");

    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);

    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 " STYLE_GREEN ", '' using 1:3 " STYLE_BLUE ", '' using 1:4" STYLE_ORANGE " \n");


    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s,%s\n", "Time","F1","F2","F3");

        // data
        for(i=0; i<len; i++)
        {
//            printf("%d,%d,%d,%d\n", i, log_conv.freq0.at(i), log_conv.freq1.at(i), log_conv.freq2.at(i) );
            gcmd("%d,%d,%d,%d\n", i, log_conv.freq0.at(i), log_conv.freq1.at(i), log_conv.freq2.at(i) );
        }
        gcmd("e;\n");
    }

    // SMOKE
    data_col_count = 2;
    gcmd("set title \"Smoke\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Min / Max - %% \" \n");
    f_minimas.push_back( *std::min_element(log_conv.smoke_ir.begin(), log_conv.smoke_ir.end()) );
    f_minimas.push_back( *std::min_element(log_conv.smoke_uv.begin(), log_conv.smoke_uv.end()) );
    f_maximas.push_back( *std::max_element(log_conv.smoke_ir.begin(), log_conv.smoke_ir.end()) );
    f_maximas.push_back( *std::max_element(log_conv.smoke_uv.begin(), log_conv.smoke_uv.end()) );
    f_min = *std::min_element(f_minimas.begin(), f_minimas.end());
    f_max = *std::max_element(f_maximas.begin(), f_maximas.end());
    check_and_fix_min_max(&f_min, &f_max, "SMOKE");
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");

    gcmd("set datafile separator ','; plot '-' using 1:2" STYLE_LILA ", '' using 1:3 with lines linestyle 2 \n");
    for (int j=0; j<data_col_count; j++)
    {
        // column names
        gcmd("%s,%s,%s\n", "Time","Smoke IR","Smoke UV");
        // data
        for(i=0; i<len; i++)
        {
            gcmd("%d,%f,%f\n", i, log_conv.smoke_ir.at(j), log_conv.smoke_uv.at(j) );
        }
        gcmd("e;\n");
    }


    // Weight
    gcmd("set title \"Weight\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Uncalibrated / NA \" \n");
    f_min = *std::min_element(log_conv.weight.begin(), log_conv.weight.end());
    f_max = *std::max_element(log_conv.weight.begin(), log_conv.weight.end());
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set style data filledcurves\n");
    //gcmd("set datafile separator ','; plot '-' using 1:2 with lines, '' using 1:3 with lines \n");
    gcmd("set datafile separator ','; plot '-' using 1:2 lt rgb 'blue' \n");
    // column names
    gcmd("%s,%s\n", "Time","Weight");
    // filled curves first point
    gcmd("%d,%f\n", 0, 0.0f);
    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%f\n", i, log_conv.weight.at(i));
    }
    // filled curves last point
    gcmd("%d,%f\n", len, 0.0f);
    gcmd("e;\n");

    // Board Temp
    gcmd("set title \"Board Temperature\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Temperature / º C \" \n");
    f_min = *std::min_element(log_conv.board.begin(), log_conv.board.end());
    f_max = *std::max_element(log_conv.board.begin(), log_conv.board.end());
    gcmd("set yrange [%f:%f]\n", f_min*0.8f, f_max*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set style data filledcurves\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 lt rgb 'red' \n");
    // column names
    gcmd("%s,%s\n", "Time","Board Temp");
    // filled curves first point
    gcmd("%d,%f\n", 0, 0.0f);
    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%f\n", i, log_conv.board.at(i));
    }
    // filled curves last point
    gcmd("%d,%f\n", len, 0.0f);
    gcmd("e;\n");


    // Abs Humi
    gcmd("set title \"Absolute Humidty SHT31\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"mg / m³ \" \n");
    uint32_t u32_minima = *std::min_element(log_conv.sht_abs_humi.begin(), log_conv.sht_abs_humi.end());
    uint32_t u32_maxima = *std::max_element(log_conv.sht_abs_humi.begin(), log_conv.sht_abs_humi.end());
    gcmd("set yrange [%f:%f]\n", u32_minima*0.8f, u32_maxima*1.2f);
    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set style data filledcurves\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 lt rgb 'red' \n");
    // column names
    gcmd("%s,%s\n", "Time","Absolute Humidity");
    // filled curves first point
    gcmd("%d,%f\n", 0, 0.0f);
    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%lu\n", i, log_conv.sht_abs_humi.at(i));
    }
    // filled curves last point
    gcmd("%d,%f\n", len, 0.0f);
    gcmd("e;\n");


    // Freq 1
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \" KFS330-MIN - Capacitive Humidity - Freq 1\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Frequency / 10 Hz \" \n");
    i_minimas.push_back( *std::min_element(log_conv.freq1.begin(), log_conv.freq1.end()) );
    i_maximas.push_back( *std::max_element(log_conv.freq1.begin(), log_conv.freq1.end()) );

    f_min= *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());

    check_and_fix_min_max(&f_min, &f_max, "Freq");

    gcmd("set yrange [%f:%f]\n", f_min*0.999f, f_max*1.001f);

    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 " STYLE_GREEN " \n");

    // column names
    gcmd("%s,%s\n", "Time","F1");

    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%d\n", i, log_conv.freq1.at(i) );
    }
    gcmd("e;\n");


    // Freq 2
    i_minimas.clear();
    i_maximas.clear();
    gcmd("set title \" KFS330-MIN - Capacitive Humidity - Freq 2\" \n");
    gcmd("set xlabel \"Time / 10ms\" \n");
    gcmd("set ylabel \"Frequency / 10 Hz \" \n");
    i_minimas.push_back( *std::min_element(log_conv.freq2.begin(), log_conv.freq2.end()) );
    i_maximas.push_back( *std::max_element(log_conv.freq2.begin(), log_conv.freq2.end()) );

    f_min= *std::min_element(i_minimas.begin(), i_minimas.end());
    f_max = *std::max_element(i_maximas.begin(), i_maximas.end());

    check_and_fix_min_max(&f_min, &f_max, "Freq");

    gcmd("set yrange [%f:%f]\n", f_min*0.999f, f_max*1.001f);

    gcmd("set xrange [0:%d]\n", len);
    // we will pipe our way in
    gcmd("set key autotitle columnhead\n");
    gcmd("set datafile separator ','; plot '-' using 1:2 " STYLE_BLUE " \n");

    // column names
    gcmd("%s,%s\n", "Time","F2");

    // data
    for(i=0; i<len; i++)
    {
        gcmd("%d,%d\n", i, log_conv.freq2.at(i) );
    }
    gcmd("e;\n");

    if ( job->flags & FJOB_USE_PNG_TERMINAL )
    {
        gcmd("unset multiplot\n");
        //gcmd("unset output\n");
    }

    fflush(gp);
}
