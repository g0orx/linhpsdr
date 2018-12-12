/* Copyright (C)
* 2018 - John Melton, G0ORX/N6LYT
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <wdsp.h>

#include "band.h"
#include "channel.h"
#include "discovered.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "audio.h"
#include "signal.h"
#include "vfo.h"
#ifdef FREEDV
#include "freedv.h"
#endif
#ifdef PSK
#include "psk.h"
#endif
//#include "vox.h"
#include "ext.h"
#include "error_handler.h"

static int last_level=-1;

#define min(x,y) (x<y?x:y)

#define SYNC0 0
#define SYNC1 1
#define SYNC2 2
#define C0 3
#define C1 4
#define C2 5
#define C3 6
#define C4 7

#define DATA_PORT 1024

#define SYNC 0x7F
#define OZY_BUFFER_SIZE 512
//#define OUTPUT_BUFFER_SIZE 1024

// ozy command and control
#define MOX_DISABLED    0x00
#define MOX_ENABLED     0x01

#define MIC_SOURCE_JANUS 0x00
#define MIC_SOURCE_PENELOPE 0x80
#define CONFIG_NONE     0x00
#define CONFIG_PENELOPE 0x20
#define CONFIG_MERCURY  0x40
#define CONFIG_BOTH     0x60
#define PENELOPE_122_88MHZ_SOURCE 0x00
#define MERCURY_122_88MHZ_SOURCE  0x10
#define ATLAS_10MHZ_SOURCE        0x00
#define PENELOPE_10MHZ_SOURCE     0x04
#define MERCURY_10MHZ_SOURCE      0x08
#define SPEED_48K                 0x00
#define SPEED_96K                 0x01
#define SPEED_192K                0x02
#define SPEED_384K                0x03
#define MODE_CLASS_E              0x01
#define MODE_OTHERS               0x00
#define ALEX_ATTENUATION_0DB      0x00
#define ALEX_ATTENUATION_10DB     0x01
#define ALEX_ATTENUATION_20DB     0x02
#define ALEX_ATTENUATION_30DB     0x03
#define LT2208_GAIN_OFF           0x00
#define LT2208_GAIN_ON            0x04
#define LT2208_DITHER_OFF         0x00
#define LT2208_DITHER_ON          0x08
#define LT2208_RANDOM_OFF         0x00
#define LT2208_RANDOM_ON          0x10

// for Hermes-Lite FPGA FW version < 40 only
static gboolean lna_dither[] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
            FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
// for Hermes-Lite FPGA FW version < 40 only
static guchar lna_att[] = {31, 30, 29, 28, 27, 26, 25, 24,
            23, 22, 21, 20, 19, 18, 17, 16,
            15, 14, 13, 12, 11, 10, 9, 8,
            7, 6, 5, 4, 3, 2, 1, 0,
            31, 30, 29, 28, 27, 26, 25, 24,
            23, 22, 21, 20, 19, 18, 17, 16,
            15, 14, 13, 12, 11, 10, 9, 8,
            7, 6, 5, 4, 3, 2, 1, 0};

static int dsp_rate=48000;
static int output_rate=48000;

static int data_socket;
static struct sockaddr_in data_addr;
static int data_addr_length;

static int output_buffer_size;

static unsigned char control_in[5]={0x00,0x00,0x00,0x00,0x00};

static double tuning_phase;
static double phase=0.0;

static int running;
static long ep4_sequence;

static int current_rx=0;

static int samples=0;
static int mic_samples=0;
static int mic_sample_divisor=1;
#ifdef FREEDV
static int freedv_divisor=6;
#endif
#ifdef PSK
static int psk_samples=0;
static int psk_divisor=6;
#endif

static double micinputbuffer[MAX_BUFFER_SIZE*2];

static int left_rx_sample;
static int right_rx_sample;
static int left_tx_sample;
static int right_tx_sample;

static unsigned char output_buffer[OZY_BUFFER_SIZE];
static int output_buffer_index=8;

static int command=1;

enum {
  SYNC_0=0,
  SYNC_1,
  SYNC_2,
  CONTROL_0,
  CONTROL_1,
  CONTROL_2,
  CONTROL_3,
  CONTROL_4,
  LEFT_SAMPLE_HI,
  LEFT_SAMPLE_MID,
  LEFT_SAMPLE_LOW,
  RIGHT_SAMPLE_HI,
  RIGHT_SAMPLE_MID,
  RIGHT_SAMPLE_LOW,
  MIC_SAMPLE_HI,
  MIC_SAMPLE_LOW,
  SKIP
};
static int state=SYNC_0;

static GThread *receive_thread_id;
static void start_protocol1_thread();
static gpointer receive_thread(gpointer arg);
static void process_ozy_input_buffer(char  *buffer);
static void process_wideband_buffer(char  *buffer);
void ozy_send_buffer();

static unsigned char metis_buffer[1032];
static long send_sequence=-1;
static int metis_offset=8;

static int metis_write(unsigned char ep,char* buffer,int length);
static void metis_start_stop(int command);
static void metis_send_buffer(char* buffer,int length);
static void metis_restart();

#define COMMON_MERCURY_FREQUENCY 0x80
#define PENELOPE_MIC 0x80

#ifdef USBOZY
//
// additional defines if we include USB Ozy support
//
#include "ozyio.h"

static GThread *ozy_EP4_rx_thread_id;
static GThread *ozy_EP6_rx_thread_id;
static gpointer ozy_ep4_rx_thread(gpointer arg);
static gpointer ozy_ep6_rx_thread(gpointer arg);
static void start_usb_receive_threads();
static int ozyusb_write(char* buffer,int length);
#define EP6_IN_ID  0x86                         // end point = 6, direction toward PC
#define EP2_OUT_ID  0x02                        // end point = 2, direction from PC
#define EP6_BUFFER_SIZE 2048
static unsigned char usb_output_buffer[EP6_BUFFER_SIZE];
static unsigned char ep6_inbuffer[EP6_BUFFER_SIZE];
static unsigned char usb_buffer_block = 0;
#endif

void protocol1_stop() {
  metis_start_stop(0);
}

void protocol1_run() {
  metis_restart();
}

void protocol1_set_mic_sample_rate(int rate) {
  mic_sample_divisor=rate/48000;
}

void protocol1_init(RADIO *r) {
  int i;

  fprintf(stderr,"create_protocol1\n");

  protocol1_set_mic_sample_rate(r->sample_rate);
  if(radio->local_microphone) {
    if(audio_open_input(r)!=0) {
      radio->local_microphone=FALSE;
    }
  }

#ifdef USBOZY
//
// if we have a USB interfaced Ozy device:
//
  if (radio->discovered->device == DEVICE_OZY) {
    fprintf(stderr,"protocol1_init: initialise ozy on USB\n");
    ozy_initialise();
    start_usb_receive_threads();
  }
  else
#endif

  start_protocol1_thread();

  fprintf(stderr,"protocol1_init: prime radio\n");
  for(i=8;i<OZY_BUFFER_SIZE;i++) {
    output_buffer[i]=0;
  }

  metis_restart();

}

#ifdef USBOZY
//
// starts the threads for USB receive
// EP4 is the wideband endpoint
// EP6 is the "normal" USB frame endpoint
//
static void start_usb_receive_threads()
{
  int rc;

  fprintf(stderr,"protocol1 starting USB receive thread: buffer_size=%d\n",radio->buffer_size);

  ozy_EP6_rx_thread_id = g_thread_new( "OZY EP6 RX", ozy_ep6_rx_thread, NULL);
  if( ! ozy_EP6_rx_thread_id )
  {
    fprintf(stderr,"g_thread_new failed for ozy_ep6_rx_thread\n");
    exit( -1 );
  }
}

//
// receive threat for USB EP4 (wideband) not currently used.
//
static gpointer ozy_ep4_rx_thread(gpointer arg)
{
}

//
// receive threat for USB EP6 (512 byte USB Ozy frames)
// this function loops reading 4 frames at a time through USB
// then processes them one at a time.
//
static gpointer ozy_ep6_rx_thread(gpointer arg) {
  int bytes;
  unsigned char buffer[2048];

  fprintf(stderr, "protocol1: USB EP6 receive_thread\n");
  running=1;
 
  while (running)
  {
    bytes = ozy_read(EP6_IN_ID,ep6_inbuffer,EP6_BUFFER_SIZE); // read a 2K buffer at a time

    if (bytes == 0)
    {
      fprintf(stderr,"protocol1_ep6_read: ozy_read returned 0 bytes... retrying\n");
      continue;
    }
    else if (bytes != EP6_BUFFER_SIZE)
    {
      fprintf(stderr,"protocol1_ep6_read: OzyBulkRead failed %d bytes\n",bytes);
      perror("ozy_read(EP6 read failed");
      //exit(1);
    }
    else
// process the received data normally
    {
      process_ozy_input_buffer(&ep6_inbuffer[0]);
      process_ozy_input_buffer(&ep6_inbuffer[512]);
      process_ozy_input_buffer(&ep6_inbuffer[1024]);
      process_ozy_input_buffer(&ep6_inbuffer[1024+512]);
    }

  }
}
#endif

static void start_protocol1_thread() {
  int i;
  int rc;
  struct hostent *h;

  fprintf(stderr,"protocol1 starting receive thread: buffer_size=%d output_buffer_size=%d\n",radio->buffer_size,output_buffer_size);

  switch(radio->discovered->device) {
#ifdef USBOZY
    case DEVICE_OZY:
      break;
#endif
    default:
      data_socket=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
      if(data_socket<0) {
        perror("protocol1: create socket failed for data_socket\n");
        exit(-1);
      }

      int optval = 1;
      if(setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0) {
        perror("data_socket: SO_REUSEADDR");
      }
      if(setsockopt(data_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0) {
        perror("data_socket: SO_REUSEPORT");
      }

/*
      // set a timeout for receive
      struct timeval tv;
      tv.tv_sec=1;
      tv.tv_usec=0;
      if(setsockopt(data_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))<0) {
        perror("data_socket: SO_RCVTIMEO");
      }
*/
      // bind to the interface
      if(bind(data_socket,(struct sockaddr*)&radio->discovered->info.network.interface_address,radio->discovered->info.network.interface_length)<0) {
        perror("protocol1: bind socket failed for data_socket\n");
        exit(-1);
      }

      memcpy(&data_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
      data_addr_length=radio->discovered->info.network.address_length;
      data_addr.sin_port=htons(DATA_PORT);
      break;
  }

  receive_thread_id = g_thread_new( "protocol1", receive_thread, NULL);
  if( ! receive_thread_id )
  {
    fprintf(stderr,"g_thread_new failed on receive_thread\n");
    exit( -1 );
  }
  fprintf(stderr, "receive_thread: id=%p\n",receive_thread_id);

}

static gpointer receive_thread(gpointer arg) {
  struct sockaddr_in addr;
  int length;
  unsigned char buffer[2048];
  int bytes_read;
  int ep;
  long sequence;

  fprintf(stderr, "protocol1: receive_thread\n");
  running=1;

  length=sizeof(addr);
  while(running) {

    switch(radio->discovered->device) {
#ifdef USBOZY
      case DEVICE_OZY:
        // should not happen
        break;
#endif

      default:
        bytes_read=recvfrom(data_socket,buffer,sizeof(buffer),0,(struct sockaddr*)&addr,&length);
        if(bytes_read<0) {
          if(errno==EAGAIN) {
            error_handler("protocol1: receiver_thread: recvfrom socket failed","Radio not sending data");
          } else {
            error_handler("protocol1: receiver_thread: recvfrom socket failed",strerror(errno));
          }
          running=0;
          continue;
        }

        if(buffer[0]==0xEF && buffer[1]==0xFE) {
          switch(buffer[2]) {
            case 1:
              // get the end point
              ep=buffer[3]&0xFF;

              // get the sequence number
              sequence=((buffer[4]&0xFF)<<24)+((buffer[5]&0xFF)<<16)+((buffer[6]&0xFF)<<8)+(buffer[7]&0xFF);

              switch(ep) {
                case 6: // EP6
                  // process the data
                  process_ozy_input_buffer(&buffer[8]);
                  process_ozy_input_buffer(&buffer[520]);
                  break;
                case 4: // EP4
                  ep4_sequence++;
                  if(sequence!=ep4_sequence) {
                    ep4_sequence=sequence;
                  } else {
                    int seq=(int)(sequence%32L);
                    if((sequence%32L)==0L) {
                      reset_wideband_buffer_index(radio->wideband);
                    }
                    process_wideband_buffer(&buffer[8]);
                    process_wideband_buffer(&buffer[520]);
                  }
                  break;
                default:
                  fprintf(stderr,"unexpected EP %d length=%d\n",ep,bytes_read);
                  break;
              }
              break;
            case 2:  // response to a discovery packet
              fprintf(stderr,"unexepected discovery response when not in discovery mode\n");
              break;
            default:
              fprintf(stderr,"unexpected packet type: 0x%02X\n",buffer[2]);
              break;
          }
        } else {
          fprintf(stderr,"received bad header bytes on data port %02X,%02X\n",buffer[0],buffer[1]);
        }
        break;
    }
  }
}

static void process_control_bytes() {
  gboolean previous_ptt;
  gboolean previous_dot;
  gboolean previous_dash;

  gint tx_mode=USB;

  RECEIVER *tx_receiver=radio->transmitter->rx;
  if(tx_receiver!=NULL) {
    if(radio->transmitter->rx->split) {
      tx_mode=tx_receiver->mode_b;
    } else {
      tx_mode=tx_receiver->mode_a;
    }
  }

  previous_ptt=radio->local_ptt;
  previous_dot=radio->dot;
  previous_dash=radio->dash;
  radio->ptt=(control_in[0]&0x01)==0x01;
  radio->dash=(control_in[0]&0x02)==0x02;
  radio->dot=(control_in[0]&0x04)==0x04;

  radio->local_ptt=radio->ptt;
  if(tx_mode==CWL || tx_mode==CWU) {
    radio->local_ptt=radio->ptt|radio->dot|radio->dash;
  }
  if(previous_ptt!=radio->local_ptt) {
g_print("process_control_bytes: ppt=%d dot=%d dash=%d\n",radio->ptt,radio->dot,radio->dash);
    g_idle_add(ext_ptt_changed,(gpointer)radio);
  }

  switch((control_in[0]>>3)&0x1F) {
    case 0:
      radio->adc_overload=control_in[1]&0x01==0x01;
      radio->IO1=control_in[1]&0x02==0x02;
      radio->IO2=control_in[1]&0x04==0x04;
      radio->IO3=control_in[1]&0x08==0x08;
      if(radio->mercury_software_version!=control_in[2]) {
        radio->mercury_software_version=control_in[2];
        fprintf(stderr,"  Mercury Software version: %d (0x%0X)\n",radio->mercury_software_version,radio->mercury_software_version);
      }
      if(radio->penelope_software_version!=control_in[3]) {
        radio->penelope_software_version=control_in[3];
        fprintf(stderr,"  Penelope Software version: %d (0x%0X)\n",radio->penelope_software_version,radio->penelope_software_version);
      }
      if(radio->ozy_software_version!=control_in[4]) {
        radio->ozy_software_version=control_in[4];
        fprintf(stderr,"FPGA firmware version: %d.%d\n",radio->ozy_software_version/10,radio->ozy_software_version%10);
      }
      break;
    case 1:
      radio->transmitter->exciter_power=((control_in[1]&0xFF)<<8)|(control_in[2]&0xFF); // from Penelope or Hermes
      radio->transmitter->alex_forward_power=((control_in[3]&0xFF)<<8)|(control_in[4]&0xFF); // from Alex or Apollo
      break;
    case 2:
      radio->transmitter->alex_reverse_power=((control_in[1]&0xFF)<<8)|(control_in[2]&0xFF); // from Alex or Apollo
      radio->AIN3=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
      break;
    case 3:
      radio->AIN4=(control_in[1]<<8)+control_in[2]; // from Pennelope or Hermes
      radio->AIN6=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
      break;
  }

}

static int nreceiver;
static int left_sample;
static int right_sample;
static short mic_sample;
static double left_sample_double;
static double right_sample_double;
static double mic_sample_double;
static int nsamples;
static int iq_samples;

static void process_ozy_byte(int b) {
  int i,j;
  switch(state) {
    case SYNC_0:
      if(b==SYNC) {
        state++;
      }
      break;
    case SYNC_1:
      if(b==SYNC) {
        state++;
      }
      break;
    case SYNC_2:
      if(b==SYNC) {
        state++;
      }
      break;
    case CONTROL_0:
      control_in[0]=b;
      state++;
      break;
    case CONTROL_1:
      control_in[1]=b;
      state++;
      break;
    case CONTROL_2:
      control_in[2]=b;
      state++;
      break;
    case CONTROL_3:
      control_in[3]=b;
      state++;
      break;
    case CONTROL_4:
      control_in[4]=b;
      process_control_bytes();
      nreceiver=0;
      iq_samples=(512-8)/((radio->receivers*6)+2);
      nsamples=0;
      state++;
      break;
    case LEFT_SAMPLE_HI:
      left_sample=(int)((signed char)b<<16);
      state++;
      break;
    case LEFT_SAMPLE_MID:
      left_sample|=(int)((((unsigned char)b)<<8)&0xFF00);
      state++;
      break;
    case LEFT_SAMPLE_LOW:
      left_sample|=(int)((unsigned char)b&0xFF);
      left_sample_double=(double)left_sample/8388607.0; // 24 bit sample 2^23-1
      state++;
      break;
    case RIGHT_SAMPLE_HI:
      right_sample=(int)((signed char)b<<16);
      state++;
      break;
    case RIGHT_SAMPLE_MID:
      right_sample|=(int)((((unsigned char)b)<<8)&0xFF00);
      state++;
      break;
    case RIGHT_SAMPLE_LOW:
      right_sample|=(int)((unsigned char)b&0xFF);
      right_sample_double=(double)right_sample/8388607.0; // 24 bit sample 2^23-1
      //find receiver
      i=-1;
      for(j=0;j<radio->discovered->supported_receivers;j++) {
        if(radio->receiver[j]!=NULL) {
          i++;
          if(i==nreceiver) break;
        }
      }
      if(radio->receiver[j]!=NULL) {
        add_iq_samples(radio->receiver[j], left_sample_double,right_sample_double);
      }
      nreceiver++;
      if(nreceiver==radio->receivers) {
        state++;
      } else {
        state=LEFT_SAMPLE_HI;
      }
      break;
    case MIC_SAMPLE_HI:
      mic_sample=(short)(b<<8);
      state++;
      break;
    case MIC_SAMPLE_LOW:
      mic_sample|=(short)(b&0xFF);
      if(!radio->local_microphone) {
        mic_samples++;
        if(mic_samples>=mic_sample_divisor) { // reduce to 48000
#ifdef FREEDV
          if(active_receiver->freedv) {
            add_freedv_mic_sample(radio->transmitter,mic_sample);
          } else {
#endif
            add_mic_sample(radio->transmitter,mic_sample);
#ifdef FREEDV
          }
#endif
          mic_samples=0;
        }
      }
      nsamples++;
      if(nsamples==iq_samples) {
        state=SYNC_0;
      } else {
        nreceiver=0;
        state=LEFT_SAMPLE_HI;
      }
      break;
  }
}

static void process_ozy_input_buffer(char  *buffer) {
  int i;
  if(radio->receivers>0) {
    for(i=0;i<512;i++) {
      process_ozy_byte(buffer[i]&0xFF);
    }
  }
}

#ifdef OLD_PROCESS
static void process_ozy_input_buffer(char  *buffer) {
  int i,j;
  int r;
  int count;
  int b=0;
  unsigned char ozy_samples[8*8];
  int bytes;
  gboolean previous_ptt;
  gboolean previous_dot;
  gboolean previous_dash;
  int left_sample;
  int right_sample;
  short mic_sample;
  double left_sample_double;
  double right_sample_double;
  double mic_sample_double;
  double gain=pow(10.0, radio->transmitter->mic_gain / 20.0);
  int left_sample_1;
  int right_sample_1;
  double left_sample_double_rx;
  double right_sample_double_rx;
  double left_sample_double_tx;
  double right_sample_double_tx;
  int nreceivers;

  gint tx_mode=USB;

  RECEIVER *tx_receiver=radio->transmitter->rx;
  if(tx_receiver!=NULL) {
    if(radio->transmitter->rx->split) {
      tx_mode=tx_receiver->mode_b;
    } else {
      tx_mode=tx_receiver->mode_a;
    }
  }

  if(buffer[b++]==SYNC && buffer[b++]==SYNC && buffer[b++]==SYNC) {
    // extract control bytes
    control_in[0]=buffer[b++];
    control_in[1]=buffer[b++];
    control_in[2]=buffer[b++];
    control_in[3]=buffer[b++];
    control_in[4]=buffer[b++];

    previous_ptt=radio->local_ptt;
    previous_dot=radio->dot;
    previous_dash=radio->dash;
    radio->ptt=(control_in[0]&0x01)==0x01;
    radio->dash=(control_in[0]&0x02)==0x02;
    radio->dot=(control_in[0]&0x04)==0x04;

    radio->local_ptt=radio->ptt;
    if(tx_mode==CWL || tx_mode==CWU) {
      radio->local_ptt=radio->ptt|radio->dot|radio->dash;
    }
    if(previous_ptt!=radio->local_ptt) {
      //g_idle_add(ext_ptt_update,(gpointer)(long)(radio->local_ptt));
    }

    switch((control_in[0]>>3)&0x1F) {
      case 0:
        radio->adc_overload=control_in[1]&0x01==0x01;
        radio->IO1=control_in[1]&0x02==0x02;
        radio->IO2=control_in[1]&0x04==0x04;
        radio->IO3=control_in[1]&0x08==0x08;
        if(radio->mercury_software_version!=control_in[2]) {
          radio->mercury_software_version=control_in[2];
          fprintf(stderr,"  Mercury Software version: %d (0x%0X)\n",radio->mercury_software_version,radio->mercury_software_version);
        }
        if(radio->penelope_software_version!=control_in[3]) {
          radio->penelope_software_version=control_in[3];
          fprintf(stderr,"  Penelope Software version: %d (0x%0X)\n",radio->penelope_software_version,radio->penelope_software_version);
        }
        if(radio->ozy_software_version!=control_in[4]) {
          radio->ozy_software_version=control_in[4];
          fprintf(stderr,"FPGA firmware version: %d.%d\n",radio->ozy_software_version/10,radio->ozy_software_version%10);
        }
        break;
      case 1:
        radio->transmitter->exciter_power=((control_in[1]&0xFF)<<8)|(control_in[2]&0xFF); // from Penelope or Hermes
        radio->transmitter->alex_forward_power=((control_in[3]&0xFF)<<8)|(control_in[4]&0xFF); // from Alex or Apollo
        break;
      case 2:
        radio->transmitter->alex_reverse_power=((control_in[1]&0xFF)<<8)|(control_in[2]&0xFF); // from Alex or Apollo
        radio->AIN3=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
        break;
      case 3:
        radio->AIN4=(control_in[1]<<8)+control_in[2]; // from Pennelope or Hermes
        radio->AIN6=(control_in[3]<<8)+control_in[4]; // from Pennelope or Hermes
        break;
    }

#ifdef PURESIGNAL
    nreceivers=(RECEIVERS*2)+1;
#else
    nreceivers=radio->receivers;
#endif
    int iq_samples=(512-8)/((nreceivers*6)+2);

    for(i=0;i<iq_samples;i++) {
      for(r=0;r<nreceivers;r++) {
        //find receiver
        count=-1;
        for(j=0;j<radio->discovered->supported_receivers;j++) {
          if(radio->receiver[j]!=NULL) {
            count++;
            if(count==r) break;
          }
        }

        left_sample   = (int)((signed char) buffer[b++])<<16;
        left_sample  |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
        left_sample  |= (int)((unsigned char)buffer[b++]&0xFF);
        right_sample  = (int)((signed char)buffer[b++]) << 16;
        right_sample |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
        right_sample |= (int)((unsigned char)buffer[b++]&0xFF);

        left_sample_double=(double)left_sample/8388607.0; // 24 bit sample 2^23-1
        right_sample_double=(double)right_sample/8388607.0; // 24 bit sample 2^23-1

#ifdef PURESIGNAL
        if(!isTransmitting(radio) || (isTransmitting(radio) && !radio->transmitter->puresignal)) {
          switch(r) {
            case 0:
              add_iq_samples(receiver[0], left_sample_double,right_sample_double);
              break;
            case 1:
              break;
            case 2:
              add_iq_samples(receiver[1], left_sample_double,right_sample_double);
              break;
            case 3:
              break;
            case 4:
              break;
          }
        } else {
          switch(r) {
            case 0:
              if(radio->discovered->device==DEVICE_METIS)  {
                left_sample_double_rx=left_sample_double;
                right_sample_double_rx=right_sample_double;
              }
              break;
            case 1:
              if(radio->discovered->device==DEVICE_METIS)  {
                left_sample_double_tx=left_sample_double;
                right_sample_double_tx=right_sample_double;
                add_ps_iq_samples(radio->transmitter, left_sample_double_rx,right_sample_double_rx,left_sample_double_tx,right_sample_double_tx);
              }
              break;
            case 2:
              if(radio->discovered->device==DEVICE_HERMES)  {
                left_sample_double_rx=left_sample_double;
                right_sample_double_rx=right_sample_double;
              }
              break;
            case 3:
              if(radio->discovered->device==DEVICE_METIS)  {
                left_sample_double_tx=left_sample_double;
                right_sample_double_tx=right_sample_double;
                add_ps_iq_samples(radio->transmitter, left_sample_double_tx,right_sample_double_tx,left_sample_double_rx,right_sample_double_rx);
              } else if(radio->discovered->device==DEVICE_ANGELIA || radio->discovered->device==DEVICE_ORION || radio->discovered->device==DEVICE_ORION2) {
                left_sample_double_rx=left_sample_double;
                right_sample_double_rx=right_sample_double;
              }
              break;
            case 4:
              if(radio->discovered->device==DEVICE_ANGELIA || radio->discovered->device==DEVICE_ORION || radio->discovered->device==DEVICE_ORION2) {
                left_sample_double_tx=left_sample_double;
                right_sample_double_tx=right_sample_double;
                add_ps_iq_samples(radio->transmitter, left_sample_double_tx,right_sample_double_tx,left_sample_double_rx,right_sample_double_rx);
              }
              break;
          }
        }
#else
        if(radio->receiver[j]!=NULL) {
          add_iq_samples(radio->receiver[j], left_sample_double,right_sample_double);
        } else {
        }
#endif
      }

      mic_sample  = (short)(buffer[b++]<<8);
      mic_sample |= (short)(buffer[b++]&0xFF);
      if(!radio->local_microphone) {
        mic_samples++;
        if(mic_samples>=mic_sample_divisor) { // reduce to 48000
#ifdef FREEDV
          if(active_receiver->freedv) {
            add_freedv_mic_sample(radio->transmitter,mic_sample);
          } else {
#endif
            add_mic_sample(radio->transmitter,mic_sample);
#ifdef FREEDV
          }
#endif
          mic_samples=0;
        }
      }
    }
  } else {
    time_t t;
    struct tm* gmt;
    time(&t);
    gmt=gmtime(&t);

    g_print("%s: process_ozy_input_buffer: did not find sync: restarting\n",
            asctime(gmt));

    b=0;
    while(b<510) {
      if(buffer[b]==SYNC && buffer[b+1]==SYNC && buffer[b+2]==SYNC) {
        g_print("found sync at %d\n",b);
      }
      b++;
    }
    
    metis_start_stop(0);
    metis_restart();
  }
}
#endif

void protocol1_audio_samples(RECEIVER *rx,short left_audio_sample,short right_audio_sample) {
  if(!isTransmitting(radio)) {
//    if(rx->mixed_audio==0) {
      rx->mixed_left_audio=left_audio_sample;
      rx->mixed_right_audio=right_audio_sample;
//    } else {
//      rx->mixed_left_audio+=left_audio_sample;
//      rx->mixed_right_audio+=right_audio_sample;
//    }
//    rx->mixed_audio++;
//    if(rx->mixed_audio>=radio->receivers) {
      output_buffer[output_buffer_index++]=rx->mixed_left_audio>>8;
      output_buffer[output_buffer_index++]=rx->mixed_left_audio;
      output_buffer[output_buffer_index++]=rx->mixed_right_audio>>8;
      output_buffer[output_buffer_index++]=rx->mixed_right_audio;
      output_buffer[output_buffer_index++]=0;
      output_buffer[output_buffer_index++]=0;
      output_buffer[output_buffer_index++]=0;
      output_buffer[output_buffer_index++]=0;
      if(output_buffer_index>=OZY_BUFFER_SIZE) {
        ozy_send_buffer();
        output_buffer_index=8;
      }
//      rx->mixed_audio=0;
//    }
  }
}

void protocol1_iq_samples(int isample,int qsample) {
  if(isTransmitting(radio)) {
    output_buffer[output_buffer_index++]=0;
    output_buffer[output_buffer_index++]=0;
    output_buffer[output_buffer_index++]=0;
    output_buffer[output_buffer_index++]=0;
    output_buffer[output_buffer_index++]=isample>>8;
    output_buffer[output_buffer_index++]=isample;
    output_buffer[output_buffer_index++]=qsample>>8;
    output_buffer[output_buffer_index++]=qsample;
    if(output_buffer_index>=OZY_BUFFER_SIZE) {
      ozy_send_buffer();
      output_buffer_index=8;
    }
  }
}


void protocol1_process_local_mic(RADIO *r) {
  int b;
  int i;
  short sample;

// always 48000 samples per second
  b=0;
  for(i=0;i<r->local_microphone_buffer_size;i++) {
    sample=(short)(r->local_microphone_buffer[i]*32767.0);
#ifdef FREEDV
    if(active_receiver->freedv) {
      add_freedv_mic_sample(r->transmitter,sample);
    } else {
#endif
      add_mic_sample(r->transmitter,sample);
#ifdef FREEDV
    }
#endif
  }
}

static void process_wideband_buffer(char  *buffer) {
  int i;
  short sample;
  double sampledouble;
  for(i=0;i<512;i+=2) {
    sample = (short) ((buffer[i + 1] << 8) + (buffer[i] & 0xFF));
    sampledouble=(double)sample/32767.0;
    if(radio->wideband!=NULL) {
      add_wideband_sample(radio->wideband, sampledouble);
    }
  }
}

void ozy_send_buffer() {

  int mode;
  int i,j;
  int count;
  BAND *band;
  int nreceivers;
  RECEIVER *tx_receiver;

  output_buffer[SYNC0]=SYNC;
  output_buffer[SYNC1]=SYNC;
  output_buffer[SYNC2]=SYNC;

  if(metis_offset==8) {
    output_buffer[C0]=0x00;
    output_buffer[C1]=0x00;
    switch(radio->sample_rate) {
      case 48000:
        output_buffer[C1]|=SPEED_48K;
        break;
      case 96000:
        output_buffer[C1]|=SPEED_96K;
        break;
      case 192000:
        output_buffer[C1]|=SPEED_192K;
        break;
      case 384000:
        output_buffer[C1]|=SPEED_384K;
        break;
    }

// set more bits for Atlas based device
// CONFIG_BOTH seems to be critical to getting ozy to respond
#ifdef USBOZY
    if ((radio->discovered->device == DEVICE_OZY) || (radio->discovered->device == DEVICE_METIS))
#else
    if (radio->discovered->device == DEVICE_METIS)
#endif
    {
      if (radio->atlas_mic_source)
        output_buffer[C1] |= PENELOPE_MIC;
      output_buffer[C1] |= CONFIG_BOTH;
      if (radio->atlas_clock_source_128mhz)
        output_buffer[C1] |= MERCURY_122_88MHZ_SOURCE;
      output_buffer[C1] |= ((radio->atlas_clock_source_10mhz & 3) << 2);
    }

    output_buffer[C2]=0x00;
    if(radio->classE) {
      output_buffer[C2]|=0x01;
    }
    if(radio->transmitter->rx!=NULL) {
      band=band_get_band(radio->transmitter->rx->band_a);
      if(isTransmitting(radio)) {
        if(radio->transmitter->rx->split) {
          band=band_get_band(radio->transmitter->rx->band_b);
        }
        output_buffer[C2]|=band->OCtx<<1;
        if(radio->tune) {
          if(radio->OCmemory_tune_time!=0) {
            struct timeval te;
            gettimeofday(&te,NULL);
            long long now=te.tv_sec*1000LL+te.tv_usec/1000;
            if(radio->tune_timeout>now) {
              output_buffer[C2]|=radio->oc_tune<<1;
            }
          } else {
            output_buffer[C2]|=radio->oc_tune<<1;
          }
        }
      } else {
        output_buffer[C2]|=band->OCrx<<1;
      }
    }

// TODO - add Alex Attenuation and Alex Antenna
    output_buffer[C3]=0x00;
    if(radio->adc[0].random) {
      output_buffer[C3]|=LT2208_RANDOM_ON;
    }
    if(radio->discovered->device==DEVICE_HERMES_LITE) {
      if(radio->ozy_software_version<40) {
        // old LNA gain control method, see HL2 wiki for details
        output_buffer[C3]|=lna_dither[radio->adc[0].attenuation+12];
      }
    } else {
      if(radio->adc[0].dither) {
        output_buffer[C3]|=LT2208_DITHER_ON;
      }
    }
    if(radio->adc[0].preamp) {
      output_buffer[C3]|=LT2208_GAIN_ON;
    }

    switch(radio->adc[0].antenna) {
      case 0:  // ANT 1
        break;
      case 1:  // ANT 2
        break;
      case 2:  // ANT 3
        break;
      case 3:  // EXT 1
        //output_buffer[C3]|=0xA0;
        output_buffer[C3]|=0xC0;
        break;
      case 4:  // EXT 2
        //output_buffer[C3]|=0xC0;
        output_buffer[C3]|=0xA0;
        break;
      case 5:  // XVTR
        output_buffer[C3]|=0xE0;
        break;
      default:
        break;
    }


// TODO - add Alex TX relay, duplex, receivers Mercury board frequency
    output_buffer[C4]=0x04;  // duplex
#ifdef PURESIGNAL
    nreceivers=(RECEIVERS*2)-1;
    nreceivers+=1; // for PS TX Feedback
#else
    nreceivers=radio->receivers-1;
#endif
    output_buffer[C4]|=nreceivers<<3;
    if(isTransmitting(radio)) {
      switch(radio->alex_tx_antenna) {
        case 0:  // ANT 1
          output_buffer[C4]|=0x00;
          break;
        case 1:  // ANT 2
          output_buffer[C4]|=0x01;
          break;
        case 2:  // ANT 3
          output_buffer[C4]|=0x02;
          break;
        default:
          break;
      }
    } else {
      switch(radio->adc[0].antenna) {
        case 0:  // ANT 1
          output_buffer[C4]|=0x00;
          break;
        case 1:  // ANT 2
          output_buffer[C4]|=0x01;
          break;
        case 2:  // ANT 3
          output_buffer[C4]|=0x02;
          break;
        case 3:  // EXT 1
        case 4:  // EXT 2
        case 5:  // XVTR
          switch(radio->alex_tx_antenna) {
            case 0:  // ANT 1
              output_buffer[C4]|=0x00;
              break;
            case 1:  // ANT 2
              output_buffer[C4]|=0x01;
              break;
            case 2:  // ANT 3
              output_buffer[C4]|=0x02;
              break;
          }
          break;
      }
    }
  } else {
    switch(command) {
      case 1: // tx frequency
        output_buffer[C0]=0x02;
        long long tx_frequency;

        tx_receiver=radio->transmitter->rx;
        if(tx_receiver!=NULL) {
          if(radio->transmitter->rx->split) {
            tx_frequency=tx_receiver->frequency_b;
          } else {
            tx_frequency=tx_receiver->frequency_a;
          }
        }
        output_buffer[C1]=tx_frequency>>24;
        output_buffer[C2]=tx_frequency>>16;
        output_buffer[C3]=tx_frequency>>8;
        output_buffer[C4]=tx_frequency;
        break;
      case 2: // rx frequency
#ifdef PURESIGNAL
        nreceivers=(RECEIVERS*2)+1;
#else
        nreceivers=radio->receivers;
#endif
        if(current_rx<radio->discovered->supported_receivers) {
          output_buffer[C0]=0x04+(current_rx*2);
#ifdef PURESIGNAL
          int v=receiver[current_rx/2]->id;
          if(isTransmitting(radio) && radio->transmitter->puresignal) {
            long long txFrequency;
            if(active_receiver->id==VFO_A) {
              if(split) {
                txFrequency=vfo[VFO_B].frequency-vfo[VFO_B].lo+vfo[VFO_B].offset;
              } else {
                txFrequency=vfo[VFO_A].frequency-vfo[VFO_A].lo+vfo[VFO_A].offset;
              }
            } else {
              if(split) {
                txFrequency=vfo[VFO_A].frequency-vfo[VFO_A].lo+vfo[VFO_A].offset;
              } else {
                txFrequency=vfo[VFO_B].frequency-vfo[VFO_B].lo+vfo[VFO_B].offset;
              }
            }
            output_buffer[C1]=txFrequency>>24;
            output_buffer[C2]=txFrequency>>16;
            output_buffer[C3]=txFrequency>>8;
            output_buffer[C4]=txFrequency;
          } else {
#else
            //RECEIVER *rx=radio->receiver[current_rx];
            //find receiver
            count=-1;
            for(j=0;j<radio->discovered->supported_receivers;j++) {
              if(radio->receiver[j]!=NULL) {
                count++;
                if(count==current_rx) break;
              }
            }
            RECEIVER *rx=radio->receiver[j];
#endif
            long long rx_frequency=0;
            if(rx!=NULL) {
              rx_frequency=rx->frequency_a-rx->lo_a;
              if(rx->rit_enabled) {
                rx_frequency+=rx->rit;
              }
              if(rx->mode_a==CWU) {
                rx_frequency-=(long long)radio->cw_keyer_sidetone_frequency;
              } else if(rx->mode_a==CWL) {
                rx_frequency+=(long long)radio->cw_keyer_sidetone_frequency;
              }
            }

            output_buffer[C1]=rx_frequency>>24;
            output_buffer[C2]=rx_frequency>>16;
            output_buffer[C3]=rx_frequency>>8;
            output_buffer[C4]=rx_frequency;
#ifdef PURESIGNAL
          }
#endif
          current_rx++;
        }
        if(current_rx>=radio->discovered->supported_receivers) {
          current_rx=0;
        }
        break;
      case 3:
        {
        
        int level=0;
        if(isTransmitting(radio)) {
          BAND *band;
          if(radio->transmitter!=NULL) {
            if(radio->transmitter->rx!=NULL) {
              if(radio->transmitter->rx->split) {
                band=band_get_band(radio->transmitter->rx->band_b);
              } else {
                band=band_get_band(radio->transmitter->rx->band_a);
              }
            }
          }
    
          int power=0;
          if(isTransmitting(radio)) {
            if(radio->tune && !radio->transmitter->tune_use_drive) {
              power=(int)(radio->transmitter->drive/100.0*radio->transmitter->tune_percent);
            } else {
              power=(int)radio->transmitter->drive;
            }
          }

          double target_dbm = 10.0 * log10(power * 1000.0);
          double gbb=band->pa_calibration;
          target_dbm-=gbb;
          double target_volts = sqrt(pow(10, target_dbm * 0.1) * 0.05);
          double volts=min((target_volts / 0.8), 1.0);
          double actual_volts=volts*(1.0/0.98);
  
          if(actual_volts<0.0) {
            actual_volts=0.0;
          } else if(actual_volts>1.0) {
            actual_volts=1.0;
          }
  
          level=(int)(actual_volts*255.0);
        }

//        if(level!=last_level) {
//g_print("protocol1: drive level changed from=%d to=%d\n",last_level,level);
//          last_level=level;
//        }
        output_buffer[C0]=0x12;
        output_buffer[C1]=level&0xFF;
        output_buffer[C2]=0x00;
        if(radio->mic_boost) {
          output_buffer[C2]|=0x01;
        }
        if(radio->mic_linein) {
          output_buffer[C2]|=0x02;
        }
        if(radio->filter_board==APOLLO) {
          output_buffer[C2]|=0x2C;
        }
        if((radio->filter_board==APOLLO) && radio->tune) {
          output_buffer[C2]|=0x10;
        }
        output_buffer[C3]=0x00;
        if(radio->transmitter->rx->band_a==band6) {
          output_buffer[C3]=output_buffer[C3]|0x40; // Alex 6M low noise amplifier
        }
        band=band_get_band(radio->transmitter->rx->band_a);
        if(isTransmitting(radio)) {
          if(radio->transmitter->rx->split) {
            band=band_get_band(radio->transmitter->rx->band_b);
          }
        }
        if(band->disablePA) {
          output_buffer[C3]=output_buffer[C3]|0x80; // disable PA
        }
        output_buffer[C4]=0x00;

        switch(radio->adc[0].filters) {
          case AUTOMATIC:
            // nothing to do as the firmware sets the filters
            break;
          case MANUAL:
            output_buffer[C2]|=0x40;
            switch(radio->adc[0].hpf) {
              case BYPASS:
              output_buffer[C2]|=0x40;  // MANUAL 
              output_buffer[C3]|=0x20;  // BYPASS all HPFs
              break;
              case HPF_1_5:
                output_buffer[C3]|=0x10;
                break;
              case HPF_6_5:
                output_buffer[C3]|=0x08;
                break;
              case HPF_9_5:
                output_buffer[C3]|=0x04;
                break;
              case HPF_13:
                output_buffer[C3]|=0x01;
                break;
              case  HPF_20:
                output_buffer[C3]|=0x02;
                break;
            }
            switch(radio->adc[0].lpf) {
              case LPF_160:
                output_buffer[C3]|=0x08;
                break;
              case LPF_80:
                output_buffer[C3]|=0x04;
                break;
              case LPF_60_40:
                output_buffer[C3]|=0x02;
                break;
              case LPF_30_20:
                output_buffer[C3]|=0x01;
                break;
              case  LPF_17_15:
                output_buffer[C3]|=0x40;
                break;
              case  LPF_12_10:
                output_buffer[C3]|=0x20;
                break;
              case  LPF_6:
                output_buffer[C3]|=0x10;
                break;
            }
            break;
          }
        }
        break;
      case 4:
        output_buffer[C0]=0x14;
        output_buffer[C1]=0x00;
        for(i=0;i<2;i++) {
          output_buffer[C1]|=(radio->adc[i].preamp<<i);
        }
        if(radio->mic_ptt_enabled==0) {
          output_buffer[C1]|=0x40;
        }
        if(radio->mic_bias_enabled) {
          output_buffer[C1]|=0x20;
        }
        if(radio->mic_ptt_tip_bias_ring) {
          output_buffer[C1]|=0x10;
        }
        output_buffer[C2]=0x00;
        output_buffer[C2]|=radio->linein_gain;
#ifdef PURESIGNAL
        if(isTransmitting(radio) && radio->transmitter->puresignal) {
          output_buffer[C2]|=0x40;
        }
#endif
        output_buffer[C3]=0x00;
  
        if(radio->discovered->device==DEVICE_HERMES || radio->discovered->device==DEVICE_ANGELIA || radio->discovered->device==DEVICE_ORION || radio->discovered->device==DEVICE_ORION2) {
          output_buffer[C4]=0x20|(int)radio->adc[0].attenuation;
        } else if(radio->discovered->device==DEVICE_HERMES_LITE) {
          if(radio->ozy_software_version>=40) {
            // different LNA gain setting method, see HL2 wiki for details
            output_buffer[C4]=0x40|(radio->adc[0].attenuation+12);
          } else {
            output_buffer[C4]=lna_att[radio->adc[0].attenuation+12];
          }
        } else {
          output_buffer[C4]=0x00;
        }
        break;
      case 5:
        output_buffer[C0]=0x16;
        output_buffer[C1]=0x00;
        if(radio->receivers>=2) {
          if(radio->discovered->device==DEVICE_HERMES || radio->discovered->device==DEVICE_ANGELIA || radio->discovered->device==DEVICE_ORION || radio->discovered->device==DEVICE_ORION2) {
            output_buffer[C1]=0x20/*|(int)radio->receiver[1]->attenuation*/;
          }
        }
        output_buffer[C2]=0x00;
        if(radio->cw_keys_reversed) {
          output_buffer[C2]|=0x40;
        }
        output_buffer[C3]=radio->cw_keyer_speed | (radio->cw_keyer_mode<<6);
        output_buffer[C4]=radio->cw_keyer_weight | (radio->cw_keyer_spacing<<7);
        break;
      case 6:
        // need to add tx attenuation and rx ADC selection
        output_buffer[C0]=0x1C;
        output_buffer[C1]=0x00;
#ifdef PURESIGNAL
        output_buffer[C1]|=radio->receiver[0]->adc;
        output_buffer[C1]|=(radio->receiver[0]->adc<<2);
        output_buffer[C1]|=radio->receiver[1]->adc<<4;
        output_buffer[C1]|=(radio->receiver[1]->adc<<6);
        output_buffer[C2]=0x00;
        if(radio->transmitter->puresignal) {
          output_buffer[C2]|=radio->receiver[2]->adc;
        }
#else
        if(radio->receiver[0]!=NULL) {
          output_buffer[C1]|=radio->receiver[0]->adc;
        }
        if(radio->receiver[1]!=NULL) {
          output_buffer[C1]|=(radio->receiver[1]->adc<<2);
        }
        if(radio->receiver[2]!=NULL) {
          output_buffer[C1]|=(radio->receiver[2]->adc<<4);
        }
        if(radio->receiver[3]!=NULL) {
          output_buffer[C1]|=(radio->receiver[3]->adc<<6);
        }
        output_buffer[C2]=0x00;
        if(radio->receiver[4]!=NULL) {
          output_buffer[C2]|=(radio->receiver[4]->adc);
        }
        if(radio->receiver[5]!=NULL) {
          output_buffer[C2]|=(radio->receiver[5]->adc<<2);
        }
        if(radio->receiver[6]!=NULL) {
          output_buffer[C2]|=(radio->receiver[6]->adc<<4);
        }
#endif
        output_buffer[C3]=0x00;
        output_buffer[C3]|=radio->transmitter->attenuation;
        output_buffer[C4]=0x00;
        break;
      case 7:
        output_buffer[C0]=0x1E;

        gint tx_mode=USB;
        tx_receiver=radio->transmitter->rx;
        if(tx_receiver!=NULL) {
          if(radio->transmitter->rx->split) {
            tx_mode=tx_receiver->mode_b;
          } else {
            tx_mode=tx_receiver->mode_a;
          }
        }

        output_buffer[C1]=0x00;
        if(tx_mode!=CWU && tx_mode!=CWL) {
          // output_buffer[C1]|=0x00;
        } else {
          if(radio->tune || radio->vox || !radio->cw_keyer_internal) {
            output_buffer[C1]|=0x00;
          } else {
            output_buffer[C1]|=0x01;
          }
        }
        output_buffer[C2]=radio->cw_keyer_sidetone_volume;
        output_buffer[C3]=radio->cw_keyer_ptt_delay;
        output_buffer[C4]=0x00;
        break;
      case 8:
        output_buffer[C0]=0x20;
        output_buffer[C1]=(radio->cw_keyer_hang_time>>2) & 0xFF;
        output_buffer[C2]=radio->cw_keyer_hang_time & 0x03;
        output_buffer[C3]=(radio->cw_keyer_sidetone_frequency>>4) & 0xFF;
        output_buffer[C4]=radio->cw_keyer_sidetone_frequency & 0x0F;
        break;
      case 9:
        output_buffer[C0]=0x22;
        output_buffer[C1]=(radio->transmitter->eer_pwm_min>>2) & 0xFF;
        output_buffer[C2]=radio->transmitter->eer_pwm_min & 0x03;
        output_buffer[C3]=(radio->transmitter->eer_pwm_max>>3) & 0xFF;
        output_buffer[C4]=radio->transmitter->eer_pwm_max & 0x03;
        break;
      case 10:
        output_buffer[C0]=0x24;
        output_buffer[C1]=0x00;
        if(isTransmitting(radio)) {
          output_buffer[C1]|=0x80; // ground RX1 on transmit
        }
        output_buffer[C2]=0x00;
        if(radio->alex_rx_antenna==5) { // XVTR
          output_buffer[C2]=0x02;
        }
        output_buffer[C3]=0x00;
        output_buffer[C4]=0x00;
        break;
    }

    if(current_rx==0) {
      command++;
      if(command>10) {
        command=1;
      }
    }
  }

  // set mox
  gint tx_mode=USB;
  tx_receiver=radio->transmitter->rx;
  if(tx_receiver!=NULL) {
    if(radio->transmitter->rx->split) {
      tx_mode=tx_receiver->mode_b;
    } else {
      tx_mode=tx_receiver->mode_a;
    }
  }

  if(tx_mode==CWU || tx_mode==CWL) {
    if(radio->tune) {
      output_buffer[C0]|=0x01;
    }
  } else {
    if(isTransmitting(radio)) {
      output_buffer[C0]|=0x01;
    }
  }

#ifdef USBOZY
//
// if we have a USB interfaced Ozy device:
//
  if (radio->discovered->device == DEVICE_OZY)
        ozyusb_write(output_buffer,OZY_BUFFER_SIZE);
  else
#endif
  metis_write(0x02,output_buffer,OZY_BUFFER_SIZE);

  //fprintf(stderr,"C0=%02X C1=%02X C2=%02X C3=%02X C4=%02X\n",
  //                output_buffer[C0],output_buffer[C1],output_buffer[C2],output_buffer[C3],output_buffer[C4]);
}

#ifdef USBOZY
static int ozyusb_write(char* buffer,int length)
{
  int i;

// batch up 4 USB frames (2048 bytes) then do a USB write
  switch(usb_buffer_block++)
  {
    case 0:
    default:
      memcpy(usb_output_buffer, buffer, length);
      break;

    case 1:
      memcpy(usb_output_buffer + 512, buffer, length);
      break;

    case 2:
      memcpy(usb_output_buffer + 1024, buffer, length);
      break;

    case 3:
      memcpy(usb_output_buffer + 1024 + 512, buffer, length);
      usb_buffer_block = 0;           // reset counter
// and write the 4 usb frames to the usb in one 2k packet
      i = ozy_write(EP2_OUT_ID,usb_output_buffer,EP6_BUFFER_SIZE);
      if(i != EP6_BUFFER_SIZE)
      {
        perror("protocol1: OzyWrite ozy failed");
      }
      break;
  }
}
#endif

static int metis_write(unsigned char ep,char* buffer,int length) {
  int i;

  // copy the buffer over
  for(i=0;i<512;i++) {
    metis_buffer[i+metis_offset]=buffer[i];
  }

  if(metis_offset==8) {
    metis_offset=520;
  } else {
    send_sequence++;
    metis_buffer[0]=0xEF;
    metis_buffer[1]=0xFE;
    metis_buffer[2]=0x01;
    metis_buffer[3]=ep;
    metis_buffer[4]=(send_sequence>>24)&0xFF;
    metis_buffer[5]=(send_sequence>>16)&0xFF;
    metis_buffer[6]=(send_sequence>>8)&0xFF;
    metis_buffer[7]=(send_sequence)&0xFF;


    // send the buffer
    metis_send_buffer(&metis_buffer[0],1032);
    metis_offset=8;

  }

  return length;
}

static void metis_restart() {
  // reset metis frame
  metis_offset==8;

  // reset current rx
  current_rx=0;

  // send commands twice
  command=1;
  do {
    ozy_send_buffer();
  } while (command!=1);

  do {
    ozy_send_buffer();
  } while (command!=1);

  sleep(1);

  // start the data flowing
  metis_start_stop(3);
}

static void metis_start_stop(int command) {
  int i;
  unsigned char buffer[64];
    
  state=SYNC_0;

#ifdef USBOZY
  if(radio->discovered->device!=DEVICE_OZY) {
#endif

  buffer[0]=0xEF;
  buffer[1]=0xFE;
  buffer[2]=0x04;    // start/stop command
  buffer[3]=command;    // send EP6 and EP4 data (0x00=stop)

  for(i=0;i<60;i++) {
    buffer[i+4]=0x00;
  }

  metis_send_buffer(buffer,sizeof(buffer));
#ifdef USBOZY
  }
#endif
}

static void metis_send_buffer(char* buffer,int length) {
  if(sendto(data_socket,buffer,length,0,(struct sockaddr*)&data_addr,data_addr_length)!=length) {
    perror("sendto socket failed for metis_send_data\n");
  }
}


