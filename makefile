CROSS_COMPILE=$(TOOLCHAIN_CROSS_COMPILE)

CC_PREFIX=$(CROSS_COMPILE)-
CC=$(CC_PREFIX)gcc
CXX=$(CC_PREFIX)g++
LD=$(CC_PREFIX)ld

SYSROOT=$(SDK_ROOTFS)
GALOIS_INCLUDE=$(SDK_GALOIS)

INCS =	-I./../tdp_api
INCS += -I./include/ 							\
		-I$(SYSROOT)/usr/include/         		\
		-I$(SYSROOT)/usr/include/directfb 		\
		-I$(GALOIS_INCLUDE)/Common/include/     \
		-I$(GALOIS_INCLUDE)/OSAL/include/		\
		-I$(GALOIS_INCLUDE)/OSAL/include/CPU1/	\
		-I$(GALOIS_INCLUDE)/PE/Common/include/

LIBS_PATH = -L./../tdp_api
LIBS_PATH += -L$(SYSROOT)/home/galois/lib/
LIBS_PATH += -L$(SYSROOT)/home/galois/lib/directfb-1.4-6-libs

LIBS := $(LIBS_PATH) -ltdp
LIBS += $(LIBS_PATH) -lOSAL	-lshm -lPEAgent
LIBS += $(LIBS_PATH) -ldirectfb -ldirect -lfusion -lrt

CFLAGS += -D__LINUX__ -O0 -Wno-psabi --sysroot=$(SYSROOT)

CXXFLAGS = $(CFLAGS)

all: parser_playback_sample

SRCS =  ./vezba_5.c
SRCS += ./tables_parser.c ./remote_controller.c ./stream_controller.c
SRCS += ./iscrtavanjeGrafike.c ./parsiranjeUlaznogFajla.c

parser_playback_sample:
	$(CC) -o vezba_5 $(INCS) $(SRCS) $(CFLAGS) $(LIBS)
    
clean:
	rm -f vezba_5