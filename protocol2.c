/* Copyrigha (C)
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


//#define ECHO_MIC

#include <gtk/gtk.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <semaphore.h>
#include <math.h>

#include <wdsp.h>

#include "alex.h"
#include "band.h"
#include "channel.h"
#include "discovered.h"
#include "mode.h"
#include "filter.h"
#include "wideband.h"
#include "receiver.h"
#include "transmitter.h"
#include "adc.h"
#include "radio.h"
#include "signal.h"
#include "vfo.h"
#include "audio.h"
#ifdef FREEDV
#include "freedv.h"
#endif
#ifdef LOCALCW
#include "iambic.h"
#endif
//#include "vox.h"
#include "ext.h"
#include "main.h"
#include "protocol2.h"

#define min(x,y) (x<y?x:y)

#define PI 3.1415926535897932F

int data_socket=-1;

static int running;

sem_t response_sem;

static struct sockaddr_in base_addr;
static int base_addr_length;

static struct sockaddr_in receiver_addr;
static int receiver_addr_length;

static struct sockaddr_in transmitter_addr;
static int transmitter_addr_length;

static struct sockaddr_in high_priority_addr;
static int high_priority_addr_length;

static struct sockaddr_in audio_addr;
static int audio_addr_length;

static struct sockaddr_in iq_addr;
static int iq_addr_length;

static struct sockaddr_in data_addr[MAX_RECEIVERS];
static int data_addr_length[MAX_RECEIVERS];

static struct sockaddr_in wide_addr;
static int wide_addr_length;

static GThread *protocol2_thread_id;
static GThread *protocol2_timer_thread_id;

static long rx_sequence = 0;

static long high_priority_sequence = 0;
static long general_sequence = 0;
static long rx_specific_sequence = 0;
static long tx_specific_sequence = 0;

//static int buffer_size=BUFFER_SIZE;
//static int fft_size=4096;
static int dspRate=48000;
static int outputRate=48000;

static int micSampleRate=48000;
static int micDspRate=48000;
static int micOutputRate=192000;
static int micoutputsamples;  // 48000 in, 192000 out

static double micinputbuffer[MAX_BUFFER_SIZE*2]; // 48000
static double iqoutputbuffer[MAX_BUFFER_SIZE*4*2]; //192000

static long tx_iq_sequence;
static unsigned char iqbuffer[1444];
static int iqindex;
static int micsamples;

static int SPECTRUM_UPDATES_PER_SECOND=10;

static float phase = 0.0F;

static long response_sequence;
static int response;

//static sem_t send_high_priority_sem;
//static int send_high_priority=0;
//static sem_t send_general_sem;
//static int send_general=0;

static sem_t command_response_sem_ready;
static sem_t command_response_sem_buffer;
static GThread *command_response_thread_id;
static sem_t high_priority_sem_ready;
static sem_t high_priority_sem_buffer;
static GThread *high_priority_thread_id;
static sem_t mic_line_sem_ready;
static sem_t mic_line_sem_buffer;
static GThread *mic_line_thread_id;
static sem_t iq_sem_ready[MAX_RECEIVERS];
static sem_t iq_sem_buffer[MAX_RECEIVERS];
static GThread *iq_thread_id[MAX_RECEIVERS];
static sem_t wideband_sem_ready;
static sem_t wideband_sem_buffer;
static GThread *wideband_thread_id;

static int samples[MAX_RECEIVERS];
#ifdef INCLUDED
static int outputsamples;
#endif

static int leftaudiosample;
static int rightaudiosample;
static long audiosequence;
static unsigned char audiobuffer[260]; // was 1444
static int audioindex;

#ifdef PSK
static int psk_samples=0;
static int psk_resample=6;  // convert from 48000 to 8000
#endif

#define NET_BUFFER_SIZE 2048
// Network buffers
static struct sockaddr_in addr;
static int length;
//static unsigned char buffer[NET_BUFFER_SIZE];
static unsigned char *iq_buffer[MAX_RECEIVERS];
static unsigned char *command_response_buffer;
static unsigned char *high_priority_buffer;
static unsigned char *mic_line_buffer;
static int mic_bytes_read;

static unsigned char *wide_buffer;

static unsigned char general_buffer[60];
static unsigned char high_priority_buffer_to_radio[1444];
static unsigned char transmit_specific_buffer[60];
static unsigned char receive_specific_buffer[1444];

static gpointer protocol2_thread(gpointer data);
static gpointer protocol2_timer_thread(gpointer data);
static gpointer command_response_thread(gpointer data);
static gpointer high_priority_thread(gpointer data);
static gpointer mic_line_thread(gpointer data);
static gpointer iq_thread(gpointer data);
static gpointer wideband_thread(gpointer data);
#ifdef PURESIGNAL
static gpointer ps_iq_thread(gpointer data);
#endif
static void  process_iq_data(RECEIVER *rx);
static void  process_wideband_data(WIDEBAND *w);
#ifdef PURESIGNAL
static void  process_ps_iq_data(RECEIVER *rx);
#endif
static void  process_command_response();
static void  process_high_priority();
static void  process_mic_data(int bytes);

#ifdef INCLUDED
static void protocol2_calc_buffers() {
  switch(sample_rate) {
    case 48000:
      outputsamples=r->buffer_size;
      break;
    case 96000:
      outputsamples=r->buffer_size/2;
      break;
    case 192000:
      outputsamples=r->buffer_size/4;
      break;
    case 384000:
      outputsamples=r->buffer_size/8;
      break;
    case 768000:
      outputsamples=r->buffer_size/16;
      break;
    case 1536000:
      outputsamples=r->buffer_size/32;
      break;
  }
}
#endif

void filter_board_changed() {
    protocol2_general();
}

/*
void pa_changed() {
    protocol2_general();
}

void tuner_changed() {
    protocol2_general();
}
*/

void cw_changed() {
#ifdef LOCALCW
    // update the iambic keyer params
    if (radio->cw_keyer_internal == 0)
        keyer_update();
#endif
}

void protocol2_start_receiver(RECEIVER *r) {
  int rc;
  rc=sem_init(&iq_sem_ready[r->channel], 0, 0);
  rc=sem_init(&iq_sem_buffer[r->channel], 0, 0);
  iq_thread_id[r->channel] = g_thread_new( "iq thread", iq_thread, (gpointer)(long)r->channel);
  if( ! iq_thread_id ) {
    fprintf(stderr,"g_thread_new failed for iq_thread: rx=%d\n",r->channel);
    exit( -1 );
  }
  fprintf(stderr, "iq_thread: channel=%d thread=%p\n", r->channel, iq_thread_id[r->channel]);
  protocol2_general();
  protocol2_high_priority();
  protocol2_receive_specific();
}

void protocol2_stop_receiver(RECEIVER *r) {
  int rc;
  rc=sem_destroy(&iq_sem_ready[r->channel]);
  rc=sem_destroy(&iq_sem_buffer[r->channel]);
  protocol2_general();
  protocol2_high_priority();
}

void protocol2_start_wideband(WIDEBAND *w) {
  int rc;
g_print("protocol2_start_wideband\n");
  rc=sem_init(&wideband_sem_ready, 0, 0);
  rc=sem_init(&wideband_sem_buffer, 0, 0);
  wideband_thread_id=g_thread_new("wideband thread",wideband_thread,w);
  if(!wideband_thread_id) {
    fprintf(stderr,"g_thread_new failed for wideband_thread: channel=%d\n",w->channel);
    exit( -1 );
  }
  protocol2_general();
}

void protocol2_stop_wideband() {
  int rc;
  rc=sem_destroy(&wideband_sem_ready);
  rc=sem_destroy(&wideband_sem_buffer);
  protocol2_general();
}

void protocol2_init(RADIO *r) {
    int i;
    int rc;

    fprintf(stderr,"protocol2_init: MIC_SAMPLES=%d\n",MIC_SAMPLES);

#ifdef INCLUDED
    outputsamples=r->buffer_size;
#endif
    micoutputsamples=r->buffer_size*4;

    if(r->local_microphone) {
      if(audio_open_input(r)!=0) {
        fprintf(stderr,"audio_open_input failed\n");
        r->local_microphone=FALSE;
      }
    }

#ifdef INCLUDED
    protocol2_calc_buffers();
#endif

    rc=sem_init(&response_sem, 0, 0);
    //rc=sem_init(&send_high_priority_sem, 0, 1);
    //rc=sem_init(&send_general_sem, 0, 1);

    rc=sem_init(&command_response_sem_ready, 0, 0);
    rc=sem_init(&command_response_sem_buffer, 0, 0);
    command_response_thread_id = g_thread_new( "command_response thread",command_response_thread, NULL);
    if( ! command_response_thread_id ) {
      fprintf(stderr,"g_thread_new failed on command_response_thread\n");
      exit( -1 );
    }
    fprintf(stderr, "command_response_thread: id=%p\n",command_response_thread_id);
    rc=sem_init(&high_priority_sem_ready, 0, 0);
    rc=sem_init(&high_priority_sem_buffer, 0, 0);
    high_priority_thread_id = g_thread_new( "high_priority thread", high_priority_thread, NULL);
    if( ! high_priority_thread_id ) {
      fprintf(stderr,"g_thread_new failed on high_priority_thread\n");
      exit( -1 );
    }
    fprintf(stderr, "high_priority_thread: id=%p\n",high_priority_thread_id);
    rc=sem_init(&mic_line_sem_ready, 0, 0);
    rc=sem_init(&mic_line_sem_buffer, 0, 0);
    mic_line_thread_id = g_thread_new( "mic_line thread", mic_line_thread, NULL);
    if( ! mic_line_thread_id ) {
      fprintf(stderr,"g_thread_new failed on mic_line_thread\n");
      exit( -1 );
    }
    fprintf(stderr, "mic_line_thread: id=%p\n",mic_line_thread_id);

    for(i=0;i<r->discovered->supported_receivers;i++) {
      if(r->receiver[i]!=NULL) {
        protocol2_start_receiver(r->receiver[i]);
      }
    }

#ifdef PURESIGNAL
    // for PS the two feedback streams are synced on the one DDC
    if(r->discovered->device!=NEW_DEVICE_HERMES) {
      rc=sem_init(&iq_sem_ready[r->receiver[i]->channel], 0, 0);
      rc=sem_init(&iq_sem_buffer[r->receiver[i]->channel], 0, 0);
      iq_thread_id[r->receiver[i]->channel] = g_thread_new( "ps iq thread", ps_iq_thread, (gpointer)(long)PS_TX_FEEDBACK);
      if( ! iq_thread_id ) {
        fprintf(stderr,"g_thread_new failed for ps_iq_thread: rx=%d\n",PS_TX_FEEDBACK);
        exit( -1 );
      }
      fprintf(stderr, "iq_thread: id=%p\n",iq_thread_id);
    }
#endif

data_socket=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(data_socket<0) {
        fprintf(stderr,"metis: create socket failed for data_socket\n");
        exit(-1);
    }

    int optval = 1;
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // bind to the interface
    if(bind(data_socket,(struct sockaddr*)&r->discovered->info.network.interface_address,r->discovered->info.network.interface_length)<0) {
        fprintf(stderr,"metis: bind socket failed for data_socket\n");
        exit(-1);
    }

fprintf(stderr,"protocol2_thread: date_socket %d bound to interface\n",data_socket);

    memcpy(&base_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    base_addr_length=r->discovered->info.network.address_length;
    base_addr.sin_port=htons(GENERAL_REGISTERS_FROM_HOST_PORT);

    memcpy(&receiver_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    receiver_addr_length=r->discovered->info.network.address_length;
    receiver_addr.sin_port=htons(RECEIVER_SPECIFIC_REGISTERS_FROM_HOST_PORT);

    memcpy(&transmitter_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    transmitter_addr_length=r->discovered->info.network.address_length;
    transmitter_addr.sin_port=htons(TRANSMITTER_SPECIFIC_REGISTERS_FROM_HOST_PORT);

    memcpy(&high_priority_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    high_priority_addr_length=r->discovered->info.network.address_length;
    high_priority_addr.sin_port=htons(HIGH_PRIORITY_FROM_HOST_PORT);

fprintf(stderr,"protocol2_thread: high_priority_addr setup for port %d\n",HIGH_PRIORITY_FROM_HOST_PORT);

    memcpy(&audio_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    audio_addr_length=r->discovered->info.network.address_length;
    audio_addr.sin_port=htons(AUDIO_FROM_HOST_PORT);

    memcpy(&iq_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    iq_addr_length=r->discovered->info.network.address_length;
    iq_addr.sin_port=htons(TX_IQ_FROM_HOST_PORT);


    for(i=0;i<r->discovered->supported_receivers;i++) {
        memcpy(&data_addr[i],&r->discovered->info.network.address,r->discovered->info.network.address_length);
        data_addr_length[i]=r->discovered->info.network.address_length;
        data_addr[i].sin_port=htons(RX_IQ_TO_HOST_PORT_0+i);
        samples[i]=0;
    }

    memcpy(&wide_addr,&r->discovered->info.network.address,r->discovered->info.network.address_length);
    wide_addr_length=r->discovered->info.network.address_length;
    wide_addr.sin_port=htons(WIDE_BAND_TO_HOST_PORT);

    protocol2_thread_id = g_thread_new( "protocol2", protocol2_thread, NULL);
    if( ! protocol2_thread_id )
    {
        fprintf(stderr,"g_thread_new failed on protocol2_thread\n");
        exit( -1 );
    }
    fprintf(stderr, "protocol2_thread: id=%p\n",protocol2_thread_id);

}

void protocol2_general() {
    BAND *band;

    band=NULL;
    if(radio!=NULL) {
      if(radio->transmitter!=NULL) {
        if(radio->transmitter->rx!=NULL) {
          if(radio->transmitter->rx->split) {
              band=band_get_band(radio->transmitter->rx->band_a);
          } else {
              band=band_get_band(radio->transmitter->rx->band_b);
          }
        }
      }
    }
    memset(general_buffer, 0, sizeof(general_buffer));

    general_buffer[0]=general_sequence>>24;
    general_buffer[1]=general_sequence>>16;
    general_buffer[2]=general_sequence>>8;
    general_buffer[3]=general_sequence;

    if(radio!=NULL) {
      if(radio->wideband!=NULL) {
        general_buffer[23]=0x01; // enable for ADC-0
      }
    }

    // use defaults apart from
    general_buffer[37]=0x08;  //  phase word (not frequency)
    general_buffer[38]=0x01;  //  enable hardware timer

    if(band==NULL || band->disablePA) {
      general_buffer[58]=0x00;
    } else {
      general_buffer[58]=0x01;  // enable PA
    }

    if(radio!=NULL) {
      if(radio->filter_board==APOLLO) {
        general_buffer[58]|=0x02; // enable APOLLO tuner
      }

      if(radio->filter_board==ALEX) {
        if(radio->discovered->device==NEW_DEVICE_ORION2) {
          general_buffer[59]=0x03;  // enable Alex 0 and 1
        } else {
          general_buffer[59]=0x01;  // enable Alex 0
        }
      }
    }

    if(sendto(data_socket,general_buffer,sizeof(general_buffer),0,(struct sockaddr*)&base_addr,base_addr_length)<0) {
      fprintf(stderr,"sendto socket failed for general: seq=%ld\n",general_sequence);
    }
    general_sequence++;
}

void protocol2_high_priority() {
    int i, r;
    BAND *band;
    int xvtr;
    long long rxFrequency;
    long long txFrequency;
    long phase;
    int mode;

    if(data_socket==-1) {
      return;
    }

    if(radio==NULL) {
      return;
    }

    memset(high_priority_buffer_to_radio, 0, sizeof(high_priority_buffer_to_radio));

    high_priority_buffer_to_radio[0]=high_priority_sequence>>24;
    high_priority_buffer_to_radio[1]=high_priority_sequence>>16;
    high_priority_buffer_to_radio[2]=high_priority_sequence>>8;
    high_priority_buffer_to_radio[3]=high_priority_sequence;

    mode=USB;
      if(radio->transmitter->rx!=NULL) {
        if(radio->transmitter->rx->split) {
          mode=radio->transmitter->rx->mode_a;
        } else {
          mode=radio->transmitter->rx->mode_b;
        }
      }
      high_priority_buffer_to_radio[4]=running;
      if(mode==CWU || mode==CWL) {
        if(radio->tune) {
          high_priority_buffer_to_radio[4]|=0x02;
        }
#ifdef LOCALCW
      if (radio->cw_keyer_internal == 0) {
        // set the ptt if we're not in breakin mode and mox is on
        if(radio->cw_breakin == 0 && getMox()) high_priority_buffer_to_radio[4]|=0x02;
        high_priority_buffer_to_radio[5]|=(keyer_out) ? 0x01 : 0;
        //high_priority_buffer_to_radio[5]|=(*kdot) ? 0x02 : 0;
        //high_priority_buffer_to_radio[5]|=(*kdash) ? 0x04 : 0;
        high_priority_buffer_to_radio[5]|=(key_state==SENDDOT) ? 0x02 : 0;
        high_priority_buffer_to_radio[5]|=(key_state==SENDDASH) ? 0x04 : 0;
      }
#endif
      } else {
        if(isTransmitting(radio)) {
          high_priority_buffer_to_radio[4]|=0x02;
        }
      }

// rx

      for(r=0;r<radio->discovered->supported_receivers;r++) {
        if(radio->receiver[r]!=NULL) {
          int v=radio->receiver[r]->channel;
          rxFrequency=radio->receiver[r]->frequency_a-radio->receiver[r]->lo_a;
          if(radio->receiver[r]->rit_enabled) {
            rxFrequency+=radio->receiver[r]->rit;
          }
  
/*
        switch(radio->receiver[r]->mode_a) {
          case CWU:
            rxFrequency-=radio->cw_keyer_sidetone_frequency;
            break;
          case CWL:
            rxFrequency+=radio->cw_keyer_sidetone_frequency;
            break;
          default:
            break;
        }
*/
          phase=(long)((4294967296.0*(double)rxFrequency)/122880000.0);
          high_priority_buffer_to_radio[9+(radio->receiver[r]->channel*4)]=phase>>24;
          high_priority_buffer_to_radio[10+(radio->receiver[r]->channel*4)]=phase>>16;
          high_priority_buffer_to_radio[11+(radio->receiver[r]->channel*4)]=phase>>8;
          high_priority_buffer_to_radio[12+(radio->receiver[r]->channel*4)]=phase;
        }
      }

      // tx
      if(radio->transmitter->rx!=NULL) {
        txFrequency=radio->transmitter->rx->frequency_a-radio->transmitter->rx->lo_a;
        if(radio->transmitter->rx->split) {
          txFrequency=radio->transmitter->rx->frequency_b-radio->transmitter->rx->lo_b;
        }

        switch(radio->transmitter->rx->mode_a) {
          case CWU:
            txFrequency+=radio->cw_keyer_sidetone_frequency;
            break;
          case CWL:
            txFrequency-=radio->cw_keyer_sidetone_frequency;
            break;
          default:
            break;
        }
  
        phase=(long)((4294967296.0*(double)txFrequency)/122880000.0);
        }

#ifdef PURESIGNAL
      if(isTransmitting(radio) && radio->transmitter->puresignal) {
        // set puresignal rx to transmit frequency
        high_priority_buffer_to_radio[9]=phase>>24;
        high_priority_buffer_to_radio[10]=phase>>16;
        high_priority_buffer_to_radio[11]=phase>>8;
        high_priority_buffer_to_radio[12]=phase;

        high_priority_buffer_to_radio[13]=phase>>24;
        high_priority_buffer_to_radio[14]=phase>>16;
        high_priority_buffer_to_radio[15]=phase>>8;
        high_priority_buffer_to_radio[16]=phase;
      }
#endif

    high_priority_buffer_to_radio[329]=phase>>24;
    high_priority_buffer_to_radio[330]=phase>>16;
    high_priority_buffer_to_radio[331]=phase>>8;
    high_priority_buffer_to_radio[332]=phase;

    int power=0;
    if(isTransmitting(radio)) {
      if(radio->tune && !radio->transmitter->tune_use_drive) {
        power=(int)((double)radio->transmitter->drive/100.0*(double)radio->transmitter->tune_percent);
      } else {
        power=radio->transmitter->drive;
      }
    }

    if(radio->transmitter!=NULL) {
      if(radio->transmitter->rx!=NULL) {
        if(radio->transmitter->rx->split) {
          band=band_get_band(radio->transmitter->rx->band_a);
          xvtr=radio->transmitter->rx->band_a>=BANDS;
        } else {
          band=band_get_band(radio->transmitter->rx->band_b);
          xvtr=radio->transmitter->rx->band_b>=BANDS;
        }
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

    int level=(int)(actual_volts*255.0);

    high_priority_buffer_to_radio[345]=level&0xFF;

    if(radio->transmitter->rx!=NULL) {
      if(isTransmitting(radio)) {
        //high_priority_buffer_to_radio[1401]=band->OCtx<<1;
        if(radio->tune) {
/*
          if(OCmemory_tune_time!=0) {
            struct timeval te;
            gettimeofday(&te,NULL);
            long long now=te.tv_sec*1000LL+te.tv_usec/1000;
            if(tune_timeout>now) {
              high_priority_buffer_to_radio[1401]|=OCtune<<1;
            }
          } else {
            high_priority_buffer_to_radio[1401]|=OCtune<<1;
          }
*/
        }
      } else {
        band=band_get_band(radio->transmitter->rx->band_a);
        high_priority_buffer_to_radio[1401]=band->OCrx<<1;
      }
/* 
      if(radio->discovered->device==NEW_DEVICE_ATLAS) {
        for(r=0;r<radio->discovered->supported_receivers;r++) {
          high_priority_buffer_to_radio[1403]|=radio->receiver[i]->preamp;
        }
      }
*/
    }


    long filters=0x00000000;

    if(isTransmitting(radio)) {
      filters=0x08000000;
#ifdef PURESIGNAL
      if(radio->transmitter->puresignal) {
        filters|=0x00040000;
      }
#endif
    }

    switch(radio->adc[0].filters) {
      case AUTOMATIC:
        break;
      case MANUAL:
        break;
    }

    if(radio->transmitter->rx!=NULL) {
      rxFrequency=radio->transmitter->rx->frequency_a-radio->transmitter->rx->lo_a;
      switch(radio->adc[0].filters) {
        case AUTOMATIC:
          switch(radio->discovered->device) {
            case NEW_DEVICE_ORION2:
              if(rxFrequency<1500000L) {
                filters|=0x1000;
              } else if(rxFrequency<2100000L) {
                filters|=0x40;
              } else if(rxFrequency<5500000L) {
                filters|=0x20;
              } else if(rxFrequency<11000000L) {
                filters|=0x10;
              } else if(rxFrequency<22000000L) {
                filters|=0x02;
              } else if(rxFrequency<35000000L) {
                filters|=0x04;
              } else {
                filters|=0x08;
              }
              break;
            default:
              if(rxFrequency<1500000L) {
                filters|=0x1000;
              } else if(rxFrequency<6500000L) {
                filters|=0x40;
              } else if(rxFrequency<9500000L) {
                filters|=0x20;
              } else if(rxFrequency<13000000L) {
                filters|=0x10;
              } else if(rxFrequency<20000000L) {
                filters|=0x02;
              } else if(rxFrequency<50000000L) {
                filters|=0x04;
              } else {
                filters|=0x80;;
              }
              break;
          }
          break;
        case MANUAL:
          switch(radio->adc[0].hpf) {
            case BYPASS:
              filters|=0x1000;
              break;
            case HPF_1_5:
              filters|=0x40;
              break;
            case HPF_6_5:
              filters|=0x20;
              break;
            case HPF_9_5:
              filters|=0x10;
              break;
            case HPF_13:
              filters|=0x02;
              break;
            case HPF_20:
              filters|=0x04;
              break;
          }
          break;
      }

      switch(radio->adc[0].filters) {
        case AUTOMATIC:
          switch(radio->discovered->device) {
            case NEW_DEVICE_ORION2:
              if(txFrequency>32000000) {
                filters|=0x20000000;
              } else if(txFrequency>22000000) {
                filters|=0x40000000;
              } else if(txFrequency>11000000) {
                filters|=0x20000000;
              } else if(txFrequency>5500000) {
                filters|=0x100000;
              } else if(txFrequency>2100000) {
                filters|=0x200000;
              } else if(txFrequency>1500000) {
                filters|=0x400000;
              } else {
                filters|=0x800000;
              }
              break;
            default:
              if(txFrequency>35600000) {
                filters|=0x08;
              } else if(txFrequency>24000000) {
                filters|=0x04;
              } else if(txFrequency>16500000) {
                filters|=0x02;
              } else if(txFrequency>8000000) {
                filters|=0x10;
              } else if(txFrequency>5000000) {
                filters|=0x20;
              } else if(txFrequency>2500000) {
                filters|=0x40;
              } else {
                filters|=0x40;
              }
              break;
          }
       case MANUAL:
          switch(radio->adc[0].lpf) {
            case LPF_160:
              filters|=0x800000;
              break;
            case LPF_80:
              filters|=0x400000;
              break;
            case LPF_60_40:
              filters|=0x200000;
              break;
            case LPF_30_20:
              filters|=0x100000;
              break;
            case LPF_17_15:
              filters|=0x80000000;
              break;
            case LPF_12_10:
              filters|=0x40000000;
              break;
            case LPF_6:
              filters|=0x20000000;
              break;
          }
          break;
      }

      switch(radio->adc[0].antenna) {
          case 0:  // ANT 1
            break;
          case 1:  // ANT 2
            break;
          case 2:  // ANT 3
            break;
          case 3:  // EXT 1
            filters|=ALEX_RX_ANTENNA_EXT2;
            break;
          case 4:  // EXT 2
            filters|=ALEX_RX_ANTENNA_EXT1;
            break;
          case 5:  // XVTR
            if(!xvtr) {
              filters|=ALEX_RX_ANTENNA_XVTR;
            }
            break;
          default:
            // invalid value - set to 0
            band->alexRxAntenna=0;
            break;
      }

      if(isTransmitting(radio)) {
        if(!xvtr) {
          switch(radio->alex_tx_antenna) {
            case 0:  // ANT 1
              filters|=ALEX_TX_ANTENNA_1;
              break;
            case 1:  // ANT 2
              filters|=ALEX_TX_ANTENNA_2;
              break;
            case 2:  // ANT 3
              filters|=ALEX_TX_ANTENNA_3;
              break;
            default:
              // invalid value - set to 0
              filters|=ALEX_TX_ANTENNA_1;
              band->alexRxAntenna=0;
              break;
          }
        }
      } else {
        switch(radio->adc[0].antenna) {
          case 0:  // ANT 1
            filters|=ALEX_TX_ANTENNA_1;
            break;
          case 1:  // ANT 2
            filters|=ALEX_TX_ANTENNA_2;
            break;
          case 2:  // ANT 3
            filters|=ALEX_TX_ANTENNA_3;
            break;
          case 3:  // EXT 1
          case 4:  // EXT 2
          case 5:  // XVTR
            if(!xvtr) {
              switch(radio->alex_tx_antenna) {
                case 0:  // ANT 1
                  filters|=ALEX_TX_ANTENNA_1;
                  break;
                case 1:  // ANT 2
                  filters|=ALEX_TX_ANTENNA_2;
                  break;
                case 2:  // ANT 3
                  filters|=ALEX_TX_ANTENNA_3;
                  break;
              }
            }
            break;
        }
      }

      high_priority_buffer_to_radio[1432]=(filters>>24)&0xFF;
      high_priority_buffer_to_radio[1433]=(filters>>16)&0xFF;
      high_priority_buffer_to_radio[1434]=(filters>>8)&0xFF;
      high_priority_buffer_to_radio[1435]=filters&0xFF;

//fprintf(stderr,"filters: txrx0: %02X %02X %02X %02X for rx=%lld tx=%lld\n",high_priority_buffer_to_radio[1432],high_priority_buffer_to_radio[1433],high_priority_buffer_to_radio[1434],high_priority_buffer_to_radio[1435],rxFrequency,txFrequency);

      filters=0x00000000;
      rxFrequency=radio->receiver[0]->frequency_a-radio->receiver[0]->lo_a;

      switch(radio->discovered->device) {
        case NEW_DEVICE_ORION2:
          if(rxFrequency<1500000L) {
            filters|=ALEX_BYPASS_HPF;
          } else if(rxFrequency<2100000L) {
            filters|=ALEX_1_5MHZ_HPF;
          } else if(rxFrequency<5500000L) {
            filters|=ALEX_6_5MHZ_HPF;
          } else if(rxFrequency<11000000L) {
            filters|=ALEX_9_5MHZ_HPF;
          } else if(rxFrequency<22000000L) {
              filters|=ALEX_13MHZ_HPF;
          } else if(rxFrequency<35000000L) {
            filters|=ALEX_20MHZ_HPF;
          } else {
            filters|=ALEX_6M_PREAMP;
          }
          break;
        default:
          if(rxFrequency<1800000L) {
            filters|=ALEX_BYPASS_HPF;
          } else if(rxFrequency<6500000L) {
            filters|=ALEX_1_5MHZ_HPF;
          } else if(rxFrequency<9500000L) {
            filters|=ALEX_6_5MHZ_HPF;
          } else if(rxFrequency<13000000L) {
            filters|=ALEX_9_5MHZ_HPF;
          } else if(rxFrequency<20000000L) {
            filters|=ALEX_13MHZ_HPF;
          } else if(rxFrequency<50000000L) {
            filters|=ALEX_20MHZ_HPF;
          } else {
            filters|=ALEX_6M_PREAMP;
          }
          break;
      }

      //high_priority_buffer_to_radio[1428]=(filters>>24)&0xFF;
      //high_priority_buffer_to_radio[1429]=(filters>>16)&0xFF;
      high_priority_buffer_to_radio[1430]=(filters>>8)&0xFF;
      high_priority_buffer_to_radio[1431]=filters&0xFF;

//fprintf(stderr,"filters: rx1: %02X %02X for rx=%lld\n",high_priority_buffer_to_radio[1430],high_priority_buffer_to_radio[1431],rxFrequency);

//fprintf(stderr,"protocol2_high_priority: OC=%02X filters=%04X for frequency=%lld\n", high_priority_buffer_to_radio[1401], filters, rxFrequency);

  
      if(isTransmitting(radio)) {
        high_priority_buffer_to_radio[1443]=radio->transmitter->attenuation;
      } else {
        high_priority_buffer_to_radio[1443]=radio->adc[0].attenuation;
        high_priority_buffer_to_radio[1442]=radio->adc[1].attenuation;
      }
    }

    int rc;
    if((rc=sendto(data_socket,high_priority_buffer_to_radio,sizeof(high_priority_buffer_to_radio),0,(struct sockaddr*)&high_priority_addr,high_priority_addr_length))<0) {
        fprintf(stderr,"sendto socket failed for high priority: rc=%d errno=%d\n",rc,errno);
        abort();
    }

    high_priority_sequence++;
}

static void protocol2_transmit_specific() {
    int mode;

    memset(transmit_specific_buffer, 0, sizeof(transmit_specific_buffer));

    transmit_specific_buffer[0]=tx_specific_sequence>>24;
    transmit_specific_buffer[1]=tx_specific_sequence>>16;
    transmit_specific_buffer[2]=tx_specific_sequence>>8;
    transmit_specific_buffer[3]=tx_specific_sequence;

    mode=USB;
    if(radio!=NULL) {
      if(radio->transmitter!=NULL) {
        if(radio->transmitter->rx!=NULL) {
            if(radio->transmitter->rx->split) {
              mode=radio->transmitter->rx->mode_a;
            } else {
              mode=radio->transmitter->rx->mode_b;
            }
        }
      }
    }

    transmit_specific_buffer[4]=1; // 1 DAC
    transmit_specific_buffer[5]=0; //  default no CW
    // may be using local pihpsdr OR hpsdr CW
    if(radio!=NULL) {
      if(mode==CWU || mode==CWL) {
        transmit_specific_buffer[5]|=0x02;
      }
      if(radio->cw_keys_reversed) {
        transmit_specific_buffer[5]|=0x04;
      }
      if(radio->cw_keyer_mode==KEYER_MODE_A) {
        transmit_specific_buffer[5]|=0x08;
      }
      if(radio->cw_keyer_mode==KEYER_MODE_B) {
        transmit_specific_buffer[5]|=0x28;
      }
      if(radio->cw_keyer_sidetone_volume!=0) {
        transmit_specific_buffer[5]|=0x10;
      }
      if(radio->cw_keyer_spacing) {
        transmit_specific_buffer[5]|=0x40;
      }
      if(radio->cw_breakin) {
        transmit_specific_buffer[5]|=0x80;
      }

      transmit_specific_buffer[6]=radio->cw_keyer_sidetone_volume; // sidetone off
      transmit_specific_buffer[7]=radio->cw_keyer_sidetone_frequency>>8;
      transmit_specific_buffer[8]=radio->cw_keyer_sidetone_frequency; // sidetone frequency
      transmit_specific_buffer[9]=radio->cw_keyer_speed; // cw keyer speed
      transmit_specific_buffer[10]=radio->cw_keyer_weight; // cw weight
      transmit_specific_buffer[11]=radio->cw_keyer_hang_time>>8;
      transmit_specific_buffer[12]=radio->cw_keyer_hang_time; // cw hang delay
      transmit_specific_buffer[13]=0; // rf delay
      transmit_specific_buffer[50]=0;
      if(radio->mic_linein) {
        transmit_specific_buffer[50]|=0x01;
      }
      if(radio->mic_boost) {
        transmit_specific_buffer[50]|=0x02;
      }
      if(radio->mic_ptt_enabled==0) {  // set if disabled
        transmit_specific_buffer[50]|=0x04;
      }
      if(radio->mic_ptt_tip_bias_ring) {
        transmit_specific_buffer[50]|=0x08;
      }
      if(radio->mic_bias_enabled) {
        transmit_specific_buffer[50]|=0x10;
      }
    }

    if(radio!=NULL) {
      transmit_specific_buffer[51]=radio->linein_gain;
    }     

    if(sendto(data_socket,transmit_specific_buffer,sizeof(transmit_specific_buffer),0,(struct sockaddr*)&transmitter_addr,transmitter_addr_length)<0) {
        fprintf(stderr,"sendto socket failed for tx specific: sequence=%ld\n",tx_specific_sequence);
    }

    tx_specific_sequence++;

}

void protocol2_receive_specific() {
  int i;

  memset(receive_specific_buffer, 0, sizeof(receive_specific_buffer));

  receive_specific_buffer[0]=rx_specific_sequence>>24;
  receive_specific_buffer[1]=rx_specific_sequence>>16;
  receive_specific_buffer[2]=rx_specific_sequence>>8;
  receive_specific_buffer[3]=rx_specific_sequence;

  receive_specific_buffer[4]=2; // 2 ADCs

  if(radio!=NULL) {
    for(i=0;i<2;i++) {
      receive_specific_buffer[5]|=radio->adc[i].dither<<i; // dither enable
      receive_specific_buffer[6]|=radio->adc[i].random<<i; // random enable
    }
    for(i=0;i<radio->discovered->supported_receivers;i++) {
      if(radio->receiver[i]!=NULL) {
        receive_specific_buffer[7]|=(1<<radio->receiver[i]->channel); // DDC enable
        receive_specific_buffer[17+(radio->receiver[i]->channel*6)]=radio->receiver[i]->adc;
        receive_specific_buffer[18+(radio->receiver[i]->channel*6)]=((radio->receiver[i]->sample_rate/1000)>>8)&0xFF;
        receive_specific_buffer[19+(radio->receiver[i]->channel*6)]=(radio->receiver[i]->sample_rate/1000)&0xFF;
        receive_specific_buffer[22+(radio->receiver[i]->channel*6)]=24;
      }
    }
  }

#ifdef PURESIGNAL
    if(radio->transmitter->puresignal && isTransmitting(radio)) {
      int ps_rate=192000;

      receive_specific_buffer[5]|=radio->dither<<radio->receiver[i]->channel; // dither enable
      receive_specific_buffer[6]|=radio->random<<radio->receiver[i]->channel; // random enable
      receive_specific_buffer[17+(radio->receiver[i]->channel*6)]=radio->receiver[PS_RX_FEEDBACK]->adc;
      receive_specific_buffer[18+(radio->receiver[i]->channel*6)]=((ps_rate/1000)>>8)&0xFF;
      receive_specific_buffer[19+(radio->receiver[i]->channel*6)]=(ps_rate/1000)&0xFF;
      receive_specific_buffer[22+(radio->receiver[i]->channel*6)]=24;

      radio->receiver[i]->channel=radio->receiver[PS_TX_FEEDBACK]->channel;
      receive_specific_buffer[5]|=radio->dither<<radio->receiver[i]->channel; // dither enable
      receive_specific_buffer[6]|=radio->random<<radio->receiver[i]->channel; // random enable
      receive_specific_buffer[17+(radio->receiver[i]->channel*6)]=radio->receiver[PS_TX_FEEDBACK]->adc;
      receive_specific_buffer[18+(radio->receiver[i]->channel*6)]=((ps_rate/1000)>>8)&0xFF;
      receive_specific_buffer[19+(radio->receiver[i]->channel*6)]=(ps_rate/1000)&0xFF;
      receive_specific_buffer[22+(radio->receiver[i]->channel*6)]=24;
      receive_specific_buffer[1363]=0x02; // sync DDC1 to DDC0

      receive_specific_buffer[7]|=1; // DDC enable
    }
#endif

//fprintf(stderr,"protocol2_receive_specific: enable=%02X\n",receive_specific_buffer[7]);
    if(sendto(data_socket,receive_specific_buffer,sizeof(receive_specific_buffer),0,(struct sockaddr*)&receiver_addr,receiver_addr_length)<0) {
      fprintf(stderr,"sendto socket failed for receive_specific: sequence=%ld\n",rx_specific_sequence);
    }
    rx_specific_sequence++;
}

static void protocol2_start() {
    protocol2_transmit_specific();
    protocol2_receive_specific();
    protocol2_timer_thread_id = g_thread_new( "protocol2 timer", protocol2_timer_thread, NULL);
    if( ! protocol2_timer_thread_id )
    {
        fprintf(stderr,"g_thread_new failed on protocol2_timer_thread\n");
        exit( -1 );
    }
    fprintf(stderr, "protocol2_timer_thread: id=%p\n",protocol2_timer_thread_id);

}

void protocol2_stop() {
    running=0;
    protocol2_high_priority();
    usleep(100000); // 100 ms
}

void protocol2_run() {
    protocol2_high_priority();
}

double calibrate(int v) {
    // Angelia
    double v1;
    v1=(double)v/4095.0*3.3;

    return (v1*v1)/0.095;
}

static gpointer protocol2_thread(gpointer data) {

    int i;
    int ddc;
    short sourceport;
    unsigned char *buffer;
    int bytesread;

fprintf(stderr,"protocol2_thread\n");

    micsamples=0;
    iqindex=4;
/*
    data_socket=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(data_socket<0) {
        fprintf(stderr,"metis: create socket failed for data_socket\n");
        exit(-1);
    }

    int optval = 1;
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // bind to the interface
    if(bind(data_socket,(struct sockaddr*)&radio->discovered->info.network.interface_address,radio->discovered->info.network.interface_length)<0) {
        fprintf(stderr,"metis: bind socket failed for data_socket\n");
        exit(-1);
    }

fprintf(stderr,"protocol2_thread: data_socket %d bound to interface\n");

    memcpy(&base_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    base_addr_length=radio->discovered->info.network.address_length;
    base_addr.sin_port=htons(GENERAL_REGISTERS_FROM_HOST_PORT);

    memcpy(&receiver_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    receiver_addr_length=radio->discovered->info.network.address_length;
    receiver_addr.sin_port=htons(RECEIVER_SPECIFIC_REGISTERS_FROM_HOST_PORT);

    memcpy(&transmitter_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    transmitter_addr_length=radio->discovered->info.network.address_length;
    transmitter_addr.sin_port=htons(TRANSMITTER_SPECIFIC_REGISTERS_FROM_HOST_PORT);

    memcpy(&high_priority_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    high_priority_addr_length=radio->discovered->info.network.address_length;
    high_priority_addr.sin_port=htons(HIGH_PRIORITY_FROM_HOST_PORT);

fprintf(stderr,"protocol2_thread: high_priority_addr setup for port %d\n",HIGH_PRIORITY_FROM_HOST_PORT);

    memcpy(&audio_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    audio_addr_length=radio->discovered->info.network.address_length;
    audio_addr.sin_port=htons(AUDIO_FROM_HOST_PORT);

    memcpy(&iq_addr,&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
    iq_addr_length=radio->discovered->info.network.address_length;
    iq_addr.sin_port=htons(TX_IQ_FROM_HOST_PORT);

   
    for(i=0;i<radio->discovered-supported_receivers;i++) {
        memcpy(&data_addr[i],&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
        data_addr_length[i]=radio->discovered->info.network.address_length;
        data_addr[i].sin_port=htons(RX_IQ_TO_HOST_PORT_0+i);
        samples[i]=0;
    }
*/
    audioindex=4; // leave space for sequence
    audiosequence=0L;

    running=1;
    protocol2_general();
    protocol2_start();
    protocol2_high_priority();

    while(running) {

        buffer=malloc(NET_BUFFER_SIZE);
        bytesread=recvfrom(data_socket,buffer,NET_BUFFER_SIZE,0,(struct sockaddr*)&addr,&length);
        if(bytesread<0) {
            fprintf(stderr,"recvfrom socket failed for protocol2_thread");
            exit(-1);
        }

        short sourceport=ntohs(addr.sin_port);

        switch(sourceport) {
            case RX_IQ_TO_HOST_PORT_0:
            case RX_IQ_TO_HOST_PORT_1:
            case RX_IQ_TO_HOST_PORT_2:
            case RX_IQ_TO_HOST_PORT_3:
            case RX_IQ_TO_HOST_PORT_4:
            case RX_IQ_TO_HOST_PORT_5:
            case RX_IQ_TO_HOST_PORT_6:
            case RX_IQ_TO_HOST_PORT_7:
              ddc=sourceport-RX_IQ_TO_HOST_PORT_0;
//fprintf(stderr,"iq packet from port=%d ddc=%d\n",sourceport,ddc);
              if(ddc>=radio->discovered->supported_receivers)  {
                fprintf(stderr,"unexpected iq data from ddc %d\n",ddc);
              } else {
                if(radio->receiver[ddc]!=NULL) {
                  sem_wait(&iq_sem_ready[ddc]);
                  iq_buffer[ddc]=buffer;
                  sem_post(&iq_sem_buffer[ddc]);
                }
              }
              break;
            case WIDE_BAND_TO_HOST_PORT:
              if(radio->wideband!=NULL) {
                sem_wait(&wideband_sem_ready);
                wide_buffer=buffer;
                sem_post(&wideband_sem_buffer);
              }
              break;
            case COMMAND_RESPONCE_TO_HOST_PORT:
              sem_wait(&command_response_sem_ready);
              command_response_buffer=buffer;
              sem_post(&command_response_sem_buffer);
              //process_command_response();
              break;
            case HIGH_PRIORITY_TO_HOST_PORT:
              sem_wait(&high_priority_sem_ready);
              high_priority_buffer=buffer;
              sem_post(&high_priority_sem_buffer);
              //process_high_priority();
              break;
            case MIC_LINE_TO_HOST_PORT:
              sem_wait(&mic_line_sem_ready);
              mic_line_buffer=buffer;
              mic_bytes_read=bytesread;
              sem_post(&mic_line_sem_buffer);
              break;
            default:
fprintf(stderr,"protocol2_thread: Unknown port %d\n",sourceport);
              free(buffer);
              break;
        }
    }

    close(data_socket);
}

static gpointer command_response_thread(gpointer data) {
  while(1) {
fprintf(stderr,"command_response_thread\n");
    sem_post(&command_response_sem_ready);
    sem_wait(&command_response_sem_buffer);
    process_command_response();
    free(command_response_buffer);
  }
}

static gpointer high_priority_thread(gpointer data) {
fprintf(stderr,"high_priority_thread\n");
  while(1) {
    sem_post(&high_priority_sem_ready);
    sem_wait(&high_priority_sem_buffer);
    process_high_priority();
    free(high_priority_buffer);
  }
}

static gpointer mic_line_thread(gpointer data) {
fprintf(stderr,"mic_line_thread\n");
  while(1) {
    sem_post(&mic_line_sem_ready);
    sem_wait(&mic_line_sem_buffer);
    if(!radio->local_microphone) {
      process_mic_data(mic_bytes_read);
    }
    free(mic_line_buffer);
  }
}

static gpointer iq_thread(gpointer data) {
  int rx=(uintptr_t)data;
  int ddc=radio->receiver[rx]->channel; // was ddc
fprintf(stderr,"iq_thread: rx=%d ddc=%d\n",rx,ddc);
  while(1) {
    sem_post(&iq_sem_ready[ddc]);
    sem_wait(&iq_sem_buffer[ddc]);
    if(radio->receiver[rx]!=NULL) {
      process_iq_data(radio->receiver[rx]);
      free(iq_buffer[ddc]);
    } else {
      break;
    }
  }
  int rc=sem_destroy(&iq_sem_ready[ddc]);
  rc=sem_destroy(&iq_sem_buffer[ddc]);
fprintf(stderr,"iq_thread terminated: rx=%d ddc=%d\n",rx,ddc);
}

#ifdef PURESIGNAL
static gpointer ps_iq_thread(gpointer data) {
  int rx=(uintptr_t)data;
  int ddc=radio->receiver[rx]->channel; // was ddc
fprintf(stderr,"ps_iq_thread: rx=%d ddc=%d\n",rx,ddc);
  while(1) {
    sem_post(&iq_sem_ready[ddc]);
    sem_wait(&iq_sem_buffer[ddc]);
    process_ps_iq_data(radio->receiver[rx]);
    free(iq_buffer[ddc]);
  }
}
#endif

static void process_iq_data(RECEIVER *rx) {
  long sequence;
  long long timestamp;
  int bitspersample;
  int samplesperframe;
  int b;
  int leftsample;
  int rightsample;
  double leftsampledouble;
  double rightsampledouble;
  unsigned char *buffer;

  buffer=iq_buffer[rx->channel];

  sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);

  if(rx->iq_sequence!=sequence) {
    //fprintf(stderr,"rx %d sequence error: expected %ld got %ld\n",rx->channel,rx->iq_sequence,sequence);
    rx->iq_sequence=sequence;
  }
  rx->iq_sequence++;

  timestamp=((long long)(buffer[4]&0xFF)<<56)+((long long)(buffer[5]&0xFF)<<48)+((long long)(buffer[6]&0xFF)<<40)+((long long)(buffer[7]&0xFF)<<32);
  ((long long)(buffer[8]&0xFF)<<24)+((long long)(buffer[9]&0xFF)<<16)+((long long)(buffer[10]&0xFF)<<8)+(long long)(buffer[11]&0xFF);
  bitspersample=((buffer[12]&0xFF)<<8)+(buffer[13]&0xFF);
  samplesperframe=((buffer[14]&0xFF)<<8)+(buffer[15]&0xFF);

//fprintf(stderr,"process_iq_data: rx=%d seq=%ld bitspersample=%d samplesperframe=%d\n",rx->id, sequence,bitspersample,samplesperframe);
  b=16;
  int i;
  for(i=0;i<samplesperframe;i++) {
    leftsample   = (int)((signed char) buffer[b++])<<16;
    leftsample  |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    leftsample  |= (int)((unsigned char)buffer[b++]&0xFF);
    rightsample  = (int)((signed char)buffer[b++]) << 16;
    rightsample |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    rightsample |= (int)((unsigned char)buffer[b++]&0xFF);

    //leftsampledouble=(double)leftsample/8388607.0; // for 24 bits
    //rightsampledouble=(double)rightsample/8388607.0; // for 24 bits
    leftsampledouble=(double)leftsample/16777215.0; // for 24 bits
    rightsampledouble=(double)rightsample/16777215.0; // for 24 bits

    add_iq_samples(rx, leftsampledouble,rightsampledouble);
  }
}

#ifdef PURESIGNAL
static void process_ps_iq_data(RECEIVER *rx) {
  long sequence;
  long long timestamp;
  int bitspersample;
  int samplesperframe;
  int b;
  int leftsample0;
  int rightsample0;
  double leftsampledouble0;
  double rightsampledouble0;
  int leftsample1;
  int rightsample1;
  double leftsampledouble1;
  double rightsampledouble1;
  unsigned char *buffer;

  int min_sample0;
  int max_sample0;
  int min_sample1;
  int max_sample1;

  buffer=iq_buffer[rx->channel];

  sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);

  if(rx->iq_sequence!=sequence) {
    //fprintf(stderr,"rx %d sequence error: expected %ld got %ld\n",rx->id,rx->iq_sequence,sequence);
    rx->iq_sequence=sequence;
  }
  rx->iq_sequence++;

  timestamp=((long long)(buffer[4]&0xFF)<<56)+((long long)(buffer[5]&0xFF)<<48)+((long long)(buffer[6]&0xFF)<<40)+((long long)(buffer[7]&0xFF)<<32);
  ((long long)(buffer[8]&0xFF)<<24)+((long long)(buffer[9]&0xFF)<<16)+((long long)(buffer[10]&0xFF)<<8)+(long long)(buffer[11]&0xFF);
  bitspersample=((buffer[12]&0xFF)<<8)+(buffer[13]&0xFF);
  samplesperframe=((buffer[14]&0xFF)<<8)+(buffer[15]&0xFF);

//fprintf(stderr,"process_ps_iq_data: rx=%d seq=%ld bitspersample=%d samplesperframe=%d\n",rx->channel, sequence,bitspersample,samplesperframe);
  b=16;
  int i;
  for(i=0;i<samplesperframe;i+=2) {
    leftsample0   = (int)((signed char) buffer[b++])<<16;
    leftsample0  |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    leftsample0  |= (int)((unsigned char)buffer[b++]&0xFF);
    rightsample0  = (int)((signed char)buffer[b++]) << 16;
    rightsample0 |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    rightsample0 |= (int)((unsigned char)buffer[b++]&0xFF);

    leftsampledouble0=(double)leftsample0/8388607.0; // for 24 bits
    rightsampledouble0=(double)rightsample0/8388607.0; // for 24 bits

    leftsample1   = (int)((signed char) buffer[b++])<<16;
    leftsample1  |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    leftsample1  |= (int)((unsigned char)buffer[b++]&0xFF);
    rightsample1  = (int)((signed char)buffer[b++]) << 16;
    rightsample1 |= (int)((((unsigned char)buffer[b++])<<8)&0xFF00);
    rightsample1 |= (int)((unsigned char)buffer[b++]&0xFF);

    leftsampledouble1=(double)leftsample1/8388607.0; // for 24 bits
    rightsampledouble1=(double)rightsample1/8388607.0; // for 24 bits

    add_ps_iq_samples(radio->transmitter, leftsampledouble0,rightsampledouble0,leftsampledouble1,rightsampledouble1);

//fprintf(stderr,"%06x,%06x %06x,%06x\n",leftsample0,rightsample0,leftsample1,rightsample1);
/*
    if(i==0) {
      min_sample0=leftsample0;
      max_sample0=leftsample0;
      min_sample1=leftsample1;
      max_sample1=leftsample1;
    } else {
      if(leftsample0<min_sample0) {
        min_sample0=leftsample0;
      }
      if(leftsample0>max_sample0) {
        max_sample0=leftsample0;
      }
      if(leftsample1<min_sample1) {
        min_sample1=leftsample1;
      }
      if(leftsample1>max_sample1) {
        max_sample1=leftsample1;
      }
    }
*/
  }
//fprintf(stderr,"0 - min=%d max=%d  1 - min=%d max=%d\n",min_sample0,max_sample0,min_sample1,max_sample1);

}
#endif

static void process_wideband_data(WIDEBAND *w) {
  long sequence;
  int b;
  int sample;
  double sampledouble;
  unsigned char *buffer;

  buffer=wide_buffer;


  sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);
  b=4;
  while(b<1028) {
    sample   = (int)((signed char) buffer[b++])<<8;
    sample  |= (int)((unsigned char)buffer[b++]&0xFF);
    sampledouble=(double)sample/32767.0; // for 16 bits
    add_wideband_sample(w, sampledouble);
  }
}

static gpointer wideband_thread(gpointer data) {
  WIDEBAND *w=(WIDEBAND *)data;
g_print("wideband_thread\n");
  while(1) {
    sem_post(&wideband_sem_ready);
    sem_wait(&wideband_sem_buffer);
    if(radio->wideband!=NULL) {
      process_wideband_data(w);
      free(wide_buffer);
    } else {
      break;
    }
  }
  int rc=sem_destroy(&wideband_sem_ready);
  rc=sem_destroy(&wideband_sem_buffer);
}


static void process_command_response() {
    response_sequence=((command_response_buffer[0]&0xFF)<<24)+((command_response_buffer[1]&0xFF)<<16)+((command_response_buffer[2]&0xFF)<<8)+(command_response_buffer[3]&0xFF);
    response=command_response_buffer[4]&0xFF;
    fprintf(stderr,"response_sequence=%ld response=%d\n",response_sequence,response);
    sem_post(&response_sem);
}

static void process_high_priority(unsigned char *buffer) {
    long sequence;
    gint mode;
    int previous_ptt;
    int previous_dot;
    int previous_dash;

    int channel=radio->transmitter->rx->channel;

    sequence=((high_priority_buffer[0]&0xFF)<<24)+((high_priority_buffer[1]&0xFF)<<16)+((high_priority_buffer[2]&0xFF)<<8)+(high_priority_buffer[3]&0xFF);

    previous_ptt=radio->ptt;
    previous_dot=radio->dot;
    previous_dash=radio->dash;

    radio->ptt=high_priority_buffer[4]&0x01;
    radio->dot=(high_priority_buffer[4]>>1)&0x01;
    radio->dash=(high_priority_buffer[4]>>2)&0x01;

    radio->pll_locked=(high_priority_buffer[4]>>3)&0x01;


    radio->adc_overload=high_priority_buffer[5]&0x01;
    radio->transmitter->exciter_power=((high_priority_buffer[6]&0xFF)<<8)|(high_priority_buffer[7]&0xFF);
    radio->transmitter->alex_forward_power=((high_priority_buffer[14]&0xFF)<<8)|(high_priority_buffer[15]&0xFF);
    radio->transmitter->alex_reverse_power=((high_priority_buffer[22]&0xFF)<<8)|(high_priority_buffer[23]&0xFF);
    radio->supply_volts=((high_priority_buffer[49]&0xFF)<<8)|(high_priority_buffer[50]&0xFF);

    if(radio->transmitter->rx->split) {
      mode=radio->transmitter->rx->mode_a;
    } else {
      mode=radio->transmitter->rx->mode_b;
    }

    gint tx_mode=USB;
    RECEIVER *tx_receiver=radio->transmitter->rx;
    if(tx_receiver!=NULL) {
      if(radio->transmitter->rx->split) {
        tx_mode=tx_receiver->mode_b;
      } else {
        tx_mode=tx_receiver->mode_a;
      }
    }

    radio->local_ptt=radio->ptt;
    if(tx_mode==CWL || tx_mode==CWU) {
      radio->local_ptt=radio->ptt|radio->dot|radio->dash;
    }
    if(previous_ptt!=radio->local_ptt) {
      g_idle_add(ext_ptt_changed,(gpointer)radio);
    }

}

static void process_mic_data(int bytes) {
  long sequence;
  int b;
  int i;
  short sample;

  sequence=((mic_line_buffer[0]&0xFF)<<24)+((mic_line_buffer[1]&0xFF)<<16)+((mic_line_buffer[2]&0xFF)<<8)+(mic_line_buffer[3]&0xFF);
  b=4;
  for(i=0;i<MIC_SAMPLES;i++) {
    sample=(short)((mic_line_buffer[b++]<<8) | (mic_line_buffer[b++]&0xFF));
#ifdef FREEDV
    if(radio->transmitter->rx->freedv) {
      add_freedv_mic_sample(radio->transmitter,sample);
    } else {
#endif
      add_mic_sample(radio->transmitter,sample);
#ifdef FREEDV
    }
#endif
  }
}

void protocol2_process_local_mic(RADIO *r) {
  int i;
  short sample;

  for(i=0;i<r->local_microphone_buffer_size;i++) {
    sample = (short)(r->local_microphone_buffer[i]*32768.0);
#ifdef FREEDV
    if(radio->transmitter->rx->freedv) {
      add_freedv_mic_sample(r->transmitter,sample);
    } else {
#endif
      add_mic_sample(r->transmitter,sample);
#ifdef FREEDV
    }
#endif
  }
}


void protocol2_audio_samples(RECEIVER *rx,short left_audio_sample,short right_audio_sample) {
  int rc;

  // insert the samples
  audiobuffer[audioindex++]=left_audio_sample>>8;
  audiobuffer[audioindex++]=left_audio_sample;
  audiobuffer[audioindex++]=right_audio_sample>>8;
  audiobuffer[audioindex++]=right_audio_sample;
            
  if(audioindex>=sizeof(audiobuffer)) {

    // insert the sequence
    audiobuffer[0]=audiosequence>>24;
    audiobuffer[1]=audiosequence>>16;
    audiobuffer[2]=audiosequence>>8;
    audiobuffer[3]=audiosequence;

    // send the buffer

    rc=sendto(data_socket,audiobuffer,sizeof(audiobuffer),0,(struct sockaddr*)&audio_addr,audio_addr_length);
    if(rc!=sizeof(audiobuffer)) {
      fprintf(stderr,"sendto socket failed for %ld bytes of audio: %d\n",sizeof(audiobuffer),rc);
    }
    audiosequence++;
    audioindex=4;
  }
}

void protocol2_iq_samples(int isample,int qsample) {
  iqbuffer[iqindex++]=isample>>16;
  iqbuffer[iqindex++]=isample>>8;
  iqbuffer[iqindex++]=isample;
  iqbuffer[iqindex++]=qsample>>16;
  iqbuffer[iqindex++]=qsample>>8;
  iqbuffer[iqindex++]=qsample;

  if(iqindex==sizeof(iqbuffer)) {
    iqbuffer[0]=tx_iq_sequence>>24;
    iqbuffer[1]=tx_iq_sequence>>16;
    iqbuffer[2]=tx_iq_sequence>>8;
    iqbuffer[3]=tx_iq_sequence;

    // send the buffer
    if(sendto(data_socket,iqbuffer,sizeof(iqbuffer),0,(struct sockaddr*)&iq_addr,iq_addr_length)<0) {
      fprintf(stderr,"sendto socket failed for iq: sequence=%ld\n",tx_iq_sequence);
    }
    tx_iq_sequence++;
    iqindex=4;
  }
}

void* protocol2_timer_thread(void* arg) {
  int specific=0;
fprintf(stderr,"protocol2_timer_thread\n");
  while(running) {
    usleep(100000); // 100ms
    protocol2_transmit_specific();
    protocol2_receive_specific();
  }
}
