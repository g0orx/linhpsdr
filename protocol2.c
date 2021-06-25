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
#include "bpsk.h"
#include "mode.h"
#include "filter.h"
#include "wideband.h"
#include "receiver.h"
#include "transmitter.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "signal.h"
#include "vfo.h"
#include "audio.h"
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

static gboolean running;

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

static long high_priority_sequence = 0;
static long general_sequence = 0;
static long rx_specific_sequence = 0;
static long tx_specific_sequence = 0;

static int micoutputsamples;  // 48000 in, 192000 out

//static double micinputbuffer[MAX_BUFFER_SIZE*2]; // 48000 ---- UNUSED
//static double iqoutputbuffer[MAX_BUFFER_SIZE*4*2]; //192000 --- UNUSED

static long tx_iq_sequence;
static unsigned char iqbuffer[1444];
static int iqindex;
static int micsamples;

//static float phase = 0.0F; //UNUSED

static long response_sequence;
static int response;

static int samples[MAX_RECEIVERS];
#ifdef INCLUDED
static int outputsamples;
#endif

static long audiosequence;
static unsigned char audiobuffer[260]; // was 1444
static int audioindex;

#define NET_BUFFER_SIZE 2048
// Network buffers
static struct sockaddr_in addr;
static socklen_t length;

static unsigned char general_buffer[60];
static unsigned char high_priority_buffer_to_radio[1444];
static unsigned char transmit_specific_buffer[60];
static unsigned char receive_specific_buffer[1444];

static gpointer protocol2_thread(gpointer data);
static gpointer protocol2_timer_thread(gpointer data);
static void  process_iq_data(RECEIVER *rx,unsigned char *buffer);
static void  process_wideband_data(WIDEBAND *w,unsigned char *buffer);
#ifdef PURESIGNAL
static void  process_ps_iq_data(RECEIVER *rx);
#endif
static void  process_command_response(unsigned char *buffer);
static void  process_high_priority(unsigned char *buffer);
static void  process_mic_data(int bytes,unsigned char *buffer);

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
  fprintf(stderr, "iq_thread: channel=%d\n", r->channel);
  protocol2_general();
  protocol2_high_priority();
  protocol2_receive_specific();
}

void protocol2_stop_receiver(RECEIVER *r) {
  protocol2_general();
  protocol2_high_priority();
}

void protocol2_start_wideband(WIDEBAND *w) {
g_print("protocol2_start_wideband\n");
  protocol2_general();
}

void protocol2_stop_wideband() {
  protocol2_general();
}

void protocol2_init(RADIO *r) {
    int i;
    // int rc; // UNUSED

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

    for(i=0;i<r->discovered->supported_receivers;i++) {
      if(r->receiver[i]!=NULL) {
        protocol2_start_receiver(r->receiver[i]);
      }
    }

    data_socket=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(data_socket<0) {
        fprintf(stderr,"protocol2_init: create socket failed for data_socket\n");
        exit(-1);
    }

    int optval = 1;
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // bind to the interface
    if(bind(data_socket,(struct sockaddr*)&r->discovered->info.network.interface_address,r->discovered->info.network.interface_length)<0) {
        fprintf(stderr,"protocol2_init: bind socket failed for data_socket\n");
        exit(-1);
    }

fprintf(stderr,"protocol2_init: date_socket %d bound to interface\n",data_socket);

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

fprintf(stderr,"protocol2_init: high_priority_addr setup for port %d\n",HIGH_PRIORITY_FROM_HOST_PORT);

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
#ifdef USE_VFO_B_MODE_AND_FILTER
          if(radio->transmitter->rx->split) {
              band=band_get_band(radio->transmitter->rx->band_b);
          } else {
#endif
              band=band_get_band(radio->transmitter->rx->band_a);
#ifdef USE_VFO_B_MODE_AND_FILTER
          }
#endif
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
    int r;
    BAND *band;
    int xvtr = 0; // IS THIS UNUSED?
    long long rxFrequency;
    long long txFrequency;
    long phase;
    int mode;
static unsigned char lastOCrx=0;

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
#ifdef USE_VFO_B_MODE_AND_FILTER
        if(radio->transmitter->rx->split) {
#endif
          mode=radio->transmitter->rx->mode_a;
#ifdef USE_VFO_B_MODE_AND_FILTER
        } else {
          mode=radio->transmitter->rx->mode_b;
        }
#endif
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
          //int v=radio->receiver[r]->channel; // UNUSED
          rxFrequency=radio->receiver[r]->frequency_a-radio->receiver[r]->lo_a+radio->receiver[r]->error_a;
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
        RECEIVER *rx=radio->transmitter->rx;
        if(rx!=NULL) {
          if(rx->split) {
            txFrequency=rx->frequency_b-rx->lo_b+rx->error_b;
          } else {
            if(rx->ctun) {
              txFrequency=rx->ctun_frequency-rx->lo_a+rx->error_a;
            } else {
              txFrequency=rx->frequency_a-rx->lo_a+rx->error_a;
            }
          }

          if(radio->transmitter->xit_enabled) {
            txFrequency+=radio->transmitter->xit;
          }
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

    int level=0;
    if(isTransmitting(radio)) {
      BAND *band;
      if(radio->transmitter!=NULL) {
        if(radio->transmitter->rx!=NULL) {
#ifdef USE_VFO_B_MODE_AND_FILTER
          if(radio->transmitter->rx->split) {
            band=band_get_band(radio->transmitter->rx->band_b);
          } else {
#endif
            band=band_get_band(radio->transmitter->rx->band_a);
#ifdef USE_VFO_B_MODE_AND_FILTER
          }
#endif
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
g_print("protocol2_high_priority: band=%d %s level=%d\n",radio->transmitter->rx->band_a, band->title, level);
    }

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
	if(band->OCrx!=0)lastOCrx=band->OCrx;
	high_priority_buffer_to_radio[1401]=lastOCrx<<1;
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
        //abort();
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
#ifdef USE_VFO_B_MODE_AND_FILTER
            if(radio->transmitter->rx->split) {
#endif
              mode=radio->transmitter->rx->mode_a;
#ifdef USE_VFO_B_MODE_AND_FILTER
            } else {
              mode=radio->transmitter->rx->mode_b;
            }
#endif
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
    //_exit(0);
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
    //short sourceport;
    static unsigned char *buffer;
    int bytesread;

fprintf(stderr,"protocol2_thread\n");

    micsamples=0;
    iqindex=4;

    data_socket=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(data_socket<0) {
        fprintf(stderr,"protocol2_thread: create socket failed for data_socket\n");
        exit(-1);
    }

    int optval = 1;
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    // bind to the interface
    if(bind(data_socket,(struct sockaddr*)&radio->discovered->info.network.interface_address,radio->discovered->info.network.interface_length)<0) {
        fprintf(stderr,"protocol2_thread: bind socket failed for data_socket\n");
        exit(-1);
    }

fprintf(stderr,"protocol2_thread: data_socket bound to interface\n");

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

   
    for(i=0;i<radio->discovered->supported_receivers;i++) {
        memcpy(&data_addr[i],&radio->discovered->info.network.address,radio->discovered->info.network.address_length);
        data_addr_length[i]=radio->discovered->info.network.address_length;
        data_addr[i].sin_port=htons(RX_IQ_TO_HOST_PORT_0+i);
        samples[i]=0;
    }

    audioindex=4; // leave space for sequence
    audiosequence=0L;

    running=TRUE;
    protocol2_general();
    protocol2_start();
    protocol2_high_priority();

    while(running) {

        buffer=malloc(NET_BUFFER_SIZE);
        length=sizeof(struct sockaddr_in);
        bytesread=recvfrom(data_socket,buffer,NET_BUFFER_SIZE,0,(struct sockaddr*)&addr,&length);
        if(bytesread<0) {
            fprintf(stderr,"recvfrom socket failed for protocol2_thread");
            exit(-1);
        }

        int sourceport=ntohs(addr.sin_port);

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
              if(ddc>=radio->discovered->supported_receivers)  {
                fprintf(stderr,"unexpected iq data from ddc %d\n",ddc);
              } else {
                if(radio->receiver[ddc]!=NULL) {
                  process_iq_data(radio->receiver[ddc],buffer);
                }
              }
              free(buffer);
              break;
            case WIDE_BAND_TO_HOST_PORT:
              if(radio->wideband!=NULL) {
                process_wideband_data(radio->wideband,buffer);
                free(buffer);
              }
              break;
            case COMMAND_RESPONCE_TO_HOST_PORT:
              process_command_response(buffer);
              free(buffer);
              break;
            case HIGH_PRIORITY_TO_HOST_PORT:
              process_high_priority(buffer);
              free(buffer);
              break;
            case MIC_LINE_TO_HOST_PORT:
              process_mic_data(bytesread,buffer);
              free(buffer);
              break;
            default:
fprintf(stderr,"protocol2_thread: Unknown port %d free %p\n",sourceport,buffer);
              free(buffer);
              break;
        }
    }

    close(data_socket);
    return NULL;
}

static void process_iq_data(RECEIVER *rx,unsigned char *buffer) {
  long sequence;
  /*
  long long timestamp;
  int bitspersample;
  */
  
  int samplesperframe;
  int b;
  int leftsample;
  int rightsample;
  double leftsampledouble;
  double rightsampledouble;
  //unsigned char *buffer;

  //buffer=iq_buffer[rx->channel];

  if(buffer!=NULL) {
  sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);
  

  if(rx->iq_sequence!=sequence) {
    //fprintf(stderr,"rx %d sequence error: expected %ld got %ld\n",rx->channel,rx->iq_sequence,sequence);
    rx->iq_sequence=sequence;
  }
  rx->iq_sequence++;

  /* UNUSED
  timestamp=((long long)(buffer[4]&0xFF)<<56)+((long long)(buffer[5]&0xFF)<<48)+((long long)(buffer[6]&0xFF)<<40)+((long long)(buffer[7]&0xFF)<<32)+
  ((long long)(buffer[8]&0xFF)<<24)+((long long)(buffer[9]&0xFF)<<16)+((long long)(buffer[10]&0xFF)<<8)+(long long)(buffer[11]&0xFF);

  bitspersample=((buffer[12]&0xFF)<<8)+(buffer[13]&0xFF);
  */
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

static void process_wideband_data(WIDEBAND *w,unsigned char *buffer) {
  //long sequence; // UNUSED
  int b;
  int sample;
  double sampledouble;
  //unsigned char *buffer;

  //buffer=wide_buffer;


  //sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF); // UNUSED
  b=4;
  while(b<1028) {
    sample   = (int)((signed char) buffer[b++])<<8;
    sample  |= (int)((unsigned char)buffer[b++]&0xFF);
    sampledouble=(double)sample/32767.0; // for 16 bits
    add_wideband_sample(w, sampledouble);
  }
}

static void process_command_response(unsigned char *buffer) {
    response_sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);
    response=buffer[4]&0xFF;
    fprintf(stderr,"response_sequence=%ld response=%d\n",response_sequence,response);
}

static void process_high_priority(unsigned char *buffer) {


    int previous_ptt;
    /* UNUSED
    long sequence; 
    gint mode;
    int previous_dot;
    int previous_dash;
    */ 



    previous_ptt=radio->ptt;
    /* UNUSED
    int channel=radio->transmitter->rx->channel; 
    sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF);
    previous_dot=radio->dot;
    previous_dash=radio->dash;
    */

    radio->ptt=buffer[4]&0x01;
    radio->dot=(buffer[4]>>1)&0x01;
    radio->dash=(buffer[4]>>2)&0x01;

    radio->pll_locked=(buffer[4]>>3)&0x01;


    radio->adc_overload=buffer[5]&0x01;
    radio->transmitter->exciter_power=((buffer[6]&0xFF)<<8)|(buffer[7]&0xFF);
    radio->transmitter->alex_forward_power=((buffer[14]&0xFF)<<8)|(buffer[15]&0xFF);
    radio->transmitter->alex_reverse_power=((buffer[22]&0xFF)<<8)|(buffer[23]&0xFF);
    radio->supply_volts=((buffer[49]&0xFF)<<8)|(buffer[50]&0xFF);

    /* UNUSED
    if(radio->transmitter->rx->split) {
      mode=radio->transmitter->rx->mode_a;
    } else {
      mode=radio->transmitter->rx->mode_b;
    }
    */
    gint tx_mode=USB;
    RECEIVER *tx_receiver=radio->transmitter->rx;
    if(tx_receiver!=NULL) {
#ifdef USE_VFO_B_MODE_AND_FILTER
      if(radio->transmitter->rx->split) {
        tx_mode=tx_receiver->mode_b;
      } else {
#endif
        tx_mode=tx_receiver->mode_a;
#ifdef USE_VFO_B_MODE_AND_FILTER
      }
#endif
    }

    radio->local_ptt=radio->ptt;
    if(tx_mode==CWL || tx_mode==CWU) {
      radio->local_ptt=radio->ptt|radio->dot|radio->dash;
    }
    if(previous_ptt!=radio->local_ptt) {
      g_idle_add(ext_ptt_changed,(gpointer)radio);
    }

}

static void process_mic_data(int bytes,unsigned char *buffer) {
  // long sequence; //UNUSED
  int b;
  int i;
  short sample;

  //sequence=((buffer[0]&0xFF)<<24)+((buffer[1]&0xFF)<<16)+((buffer[2]&0xFF)<<8)+(buffer[3]&0xFF); // UNUSED
  b=4;
  for(i=0;i<MIC_SAMPLES;i++) {
    sample=(short)(buffer[b++]<<8);
    sample|=(buffer[b++]&0xFF);
    add_mic_sample(radio->transmitter,(float)sample/32768.0);
  }
}

void protocol2_process_local_mic(RADIO *r) {
  int i;
  short sample;

  for(i=0;i<r->local_microphone_buffer_size;i++) {
    add_mic_sample(r->transmitter,r->local_microphone_buffer[i]);
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
  // int specific=0; // UNUSED
fprintf(stderr,"protocol2_timer_thread\n");
  while(running) {
    usleep(100000); // 100ms
    protocol2_transmit_specific();
    protocol2_receive_specific();
  }
  return NULL;
}

gboolean protocol2_is_running() {
  return running;
}
