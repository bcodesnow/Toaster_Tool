TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        #tcp_thread.cpp \
        multi_plot.cpp \
        toaster_tool.cpp \
        udp_thread.cpp\
        x_platform.c

HEADERS += \
    #tcp_thread.h \
    multi_plot.h \
    toaster_tool.h \
    udp_thread.h \
    x_platform.h

LIBS += -lpthread
