# Get git commit version and date
#GIT_VERSION := $(shell git --no-pager describe --tags --always --dirty)
GIT_DATE := $(firstword $(shell git --no-pager show --date=short --format="%ai" --name-only))
GIT_VERSION := $(shell git describe --abbrev=0 --tags)

CC=gcc
LINK=gcc

GTKINCLUDES=`pkg-config --cflags gtk+-3.0`
GTKLIBS=`pkg-config --libs gtk+-3.0`

#OPENGL_OPTIONS=-D OPENGL
#OPENGL_INCLUDES=`pkg-config --cflags epoxy`
#OPENGL_LIBS=`pkg-config --libs epoxy`

AUDIO_LIBS=-lasound -lpulse-simple -lpulse -lpulse-mainloop-glib -lsoundio

# uncomment the line below to include SoapySDR support
#
# Note: SoapySDR support has only been tested with the RTL-SDR and LimeSDR
#       No TX support yet.
#
#       If you want to build with SoapySDR support you will need to install:
#
#       sudo apt-get install libsoapysdr-dev
#	sudo apt-get install soapysdr-module-rtlsdr
#	sudo apt-get install soapysdr-module-lms7
#
#SOAPYSDR_INCLUDE=SOAPYSDR

ifeq ($(SOAPYSDR_INCLUDE),SOAPYSDR)
SOAPYSDR_OPTIONS=-D SOAPYSDR
SOAPYSDR_LIBS=-lSoapySDR
SOAPYSDR_SOURCES= \
soapy_discovery.c \
soapy_protocol.c
SOAPYSDR_HEADERS= \
soapy_discovery.h \
soapy_protocol.h
SOAPYSDR_OBJS= \
soapy_discovery.o \
soapy_protocol.o
endif

# cwdaemon support. Allows linux based logging software to key an Hermes/HermesLite2
# needs :
#			https://github.com/m5evt/unixcw-3.5.1.git

#CWDAEMON_INCLUDE=CWDAEMON

#ifeq ($(CWDAEMON_INCLUDE),CWDAEMON)
#CWDAEMON_OPTIONS=-D CWDAEMON
#CWDAEMON_LIBS=-lcw
#CWDAEMON_SOURCES= \
#cwdaemon.c
#CWDAEMON_HEADERS= \
#cwdaemon.h
#CWDAEMON_OBJS= \
#cwdaemon.o
#endif


OPTIONS=-Wno-deprecated-declarations $(AUDIO_OPTIONS) -D GIT_DATE='"$(GIT_DATE)"' -D GIT_VERSION='"$(GIT_VERSION)"' $(SOAPYSDR_OPTIONS) \
         $(CWDAEMON_OPTIONS)  $(OPENGL_OPTIONS) -O3 -g
#OPTIONS=-g -Wno-deprecated-declarations $(AUDIO_OPTIONS) -D GIT_DATE='"$(GIT_DATE)"' -D GIT_VERSION='"$(GIT_VERSION)"' -O3 -D FT8_MARKER

LIBS=-lrt -lm -lpthread -lwdsp
INCLUDES=$(GTKINCLUDES) $(OPGL_INCLUDES)

COMPILE=$(CC) $(OPTIONS) $(INCLUDES)

PROGRAM=linhpsdr

SOURCES=\
main.c\
audio.c\
version.c\
discovered.c\
discovery.c\
protocol1_discovery.c\
protocol2_discovery.c\
property.c\
mode.c\
filter.c\
band.c\
radio.c\
receiver.c\
transmitter.c\
vfo.c\
meter.c\
rx_panadapter.c\
tx_panadapter.c\
mic_level.c\
mic_gain.c\
drive_level.c\
waterfall.c\
wideband_panadapter.c\
wideband_waterfall.c\
protocol1.c\
protocol2.c\
radio_dialog.c\
receiver_dialog.c\
transmitter_dialog.c\
pa_dialog.c\
eer_dialog.c\
wideband_dialog.c\
about_dialog.c\
button_text.c\
wideband.c\
vox.c\
ext.c\
configure_dialog.c\
bookmark_dialog.c\
puresignal_dialog.c\
oc_dialog.c\
xvtr_dialog.c\
frequency.c\
rigctl.c\
error_handler.c\
radio_info.c\
bpsk.c

HEADERS=\
main.h\
audio.h\
version.h\
discovered.h\
discovery.h\
protocol1_discovery.h\
protocol2_discovery.h\
property.h\
agc.h\
mode.h\
filter.h\
band.h\
radio.h\
receiver.h\
transmitter.h\
vfo.h\
meter.h\
rx_panadapter.h\
tx_panadapter.h\
mic_level.h\
mic_gain.h\
drive_level.h\
wideband_panadapter.h\
wideband_waterfall.h\
waterfall.h\
protocol1.h\
protocol2.h\
radio_dialog.h\
receiver_dialog.h\
transmitter_dialog.h\
pa_dialog.h\
eer_dialog.h\
wideband_dialog.h\
about_dialog.h\
button_text.h\
wideband.h\
vox.h\
ext.h\
configure_dialog.h\
bookmark_dialog.h\
puresignal_dialog.h\
oc_dialog.h\
xvtr_dialog.h\
frequency.h\
rigctl.h\
error_handler.h\
radio_info.h\
bpsk.h

OBJS=\
main.o\
audio.o\
version.o\
discovered.o\
discovery.o\
protocol1_discovery.o\
protocol2_discovery.o\
property.o\
mode.o\
filter.o\
band.o\
radio.o\
receiver.o\
transmitter.o\
vfo.o\
meter.o\
rx_panadapter.o\
tx_panadapter.o\
mic_level.o\
mic_gain.o\
drive_level.o\
wideband_panadapter.o\
wideband_waterfall.o\
waterfall.o\
protocol1.o\
protocol2.o\
radio_dialog.o\
receiver_dialog.o\
transmitter_dialog.o\
pa_dialog.o\
eer_dialog.o\
wideband_dialog.o\
about_dialog.o\
button_text.o\
wideband.o\
vox.o\
ext.o\
configure_dialog.o\
bookmark_dialog.o\
puresignal_dialog.o\
oc_dialog.o\
xvtr_dialog.o\
frequency.o\
rigctl.o\
error_handler.o\
radio_info.o\
bpsk.o

all: prebuild  $(PROGRAM) $(HEADERS) $(SOURCES) $(SOAPYSDR_SOURCES) $(CWDAEMON_SOURCES) 

prebuild:
	rm -f version.o

$(PROGRAM):  $(OBJS) $(SOAPYSDR_OBJS) $(CWDAEMON_OBJS)
	$(LINK) -o $(PROGRAM) $(OBJS) $(SOAPYSDR_OBJS) $(CWDAEMON_OBJS) $(GTKLIBS) $(LIBS) $(AUDIO_LIBS) $(SOAPYSDR_LIBS) $(CWDAEMON_LIBS) $(OPENGL_LIBS)

.c.o:
	$(COMPILE) -c -o $@ $<


clean:
	-rm -f *.o
	-rm -f $(PROGRAM)

install: $(PROGRAM)
	cp $(PROGRAM) /usr/local/bin
	if [ ! -d /usr/share/linhpsdr ]; then mkdir /usr/share/linhpsdr; fi
	cp hpsdr.png /usr/share/linhpsdr
	cp hpsdr_icon.png /usr/share/linhpsdr
	cp hpsdr_small.png /usr/share/linhpsdr
	cp linhpsdr.desktop /usr/share/applications

debian:
	cp $(PROGRAM) pkg/linhpsdr/usr/local/bin
	cp /usr/local/lib/libwdsp.so pkg/linhpsdr/usr/local/lib
	cp hpsdr.png pkg/linhpsdr/usr/share/linhpsdr
	cp hpsdr_icon.png pkg/linhpsdr/usr/share/linhpsdr
	cp hpsdr_small.png pkg/linhpsdr/usr/share/linhpsdr
	cp linhpsdr.desktop pkg/linhpsdr/usr/share/applications
	cd pkg; dpkg-deb --build linhpsdr

