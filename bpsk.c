#include <math.h>
#include <gtk/gtk.h>

#include <wdsp.h>

#include "discovered.h"
#include "receiver.h"
#include "bpsk.h"
#include "band.h"
#include "bandstack.h"
#include "mode.h"
#include "filter.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "xvtr_dialog.h"

static int my_pixels=-1;
static float *my_pixel_samples=NULL;

void create_bpsk(RECEIVER *rx) {
  rx->bpsk_counter=0;
  rx->bpsk=TRUE;
}

void destroy_bpsk(RECEIVER *rx) {
  rx->bpsk=FALSE;
}

void reset_bpsk(RECEIVER *rx) {
fprintf(stderr,"reset_bpsk\n");
  rx->bpsk_counter=0;
}

void process_bpsk(RECEIVER *rx) {
  if(rx->pixel_samples!=NULL) {
    if(my_pixels!=rx->pixels) {
      if(my_pixel_samples!=NULL) {
        g_free(my_pixel_samples);
      }
      my_pixels=rx->pixels;
      my_pixel_samples=g_new(float,my_pixels);
    } else {
      if(rx->bpsk_counter==0) {
        for(int i=0;i<my_pixels;i++) {
          my_pixel_samples[i]=rx->pixel_samples[i];
        }
      } else {
        for(int i=0;i<my_pixels;i++) {
          my_pixel_samples[i]+=rx->pixel_samples[i];
        }
      }
    }
    rx->bpsk_counter++;
    if(rx->bpsk_counter==rx->fps) {
      for(int i=0;i<my_pixels;i++) {
        my_pixel_samples[i]=my_pixel_samples[i]/(float)rx->fps;
      }
      float threshold=5.0;
      int gap=(int)(800.0/rx->hz_per_pixel);
      float last=-140;
      int last_index=-1;
      for(int i=my_pixels-3;i>gap;i--) {
        if((my_pixel_samples[i]>=(my_pixel_samples[i-1]+threshold)) &&
           (my_pixel_samples[i-gap]>=(my_pixel_samples[i-1]+threshold))) {
          last_index=i;
          last=my_pixel_samples[last_index];
          break;
        }
      }
      if(last_index!=-1) {
        //last_index=last_index-(gap/2);
        double frequency=(double)rx->frequency_a;
        double half=(double)rx->sample_rate/2.0;
        double min_frequency=frequency-half;
        double max_frequency=frequency+half;
        if(10489800000>min_frequency&&10489800000<max_frequency) {
          double f=min_frequency+((double)last_index*rx->hz_per_pixel);
          long long offset=(long long)(f-10489800000.0);
          if(llabs(offset)>=100 && llabs(offset)<100000) {
g_print("lo_error_update: offset=%lld f=%f half=%f min_frequency=%f max_frequency=%f last_index=%d val=%f hz_per_pixel=%f zoom=%d pixels=%d threshold=%f gap=%d\n",offset,f,half,min_frequency,max_frequency,last_index,rx->pixel_samples[last_index],rx->hz_per_pixel,rx->zoom,rx->pixels,threshold,gap);
            lo_error_update(rx,offset);
          }
        }
      }
      rx->bpsk_counter=0;
    }
  }
}
