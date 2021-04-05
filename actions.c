#include <gtk/gtk.h>

#include "agc.h"
#include "adc.h"
#include "dac.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "discovered.h"
#include "radio.h"
#include "vfo.h"
#include "main.h"
#include "band.h"
#include "protocol2.h"
#include "actions.h"

char *encoder_string[ENCODER_ACTIONS] = {
  "NO ACTION",
  "AF GAIN",
  "AF GAIN RX1",
  "AF GAIN RX2",
  "AGC GAIN",
  "AGC GAIN RX1",
  "AGC GAIN RX2",
  "ATTENUATION/RX GAIN",
  "COMP",
  "CW FREQUENCY",
  "CW SPEED",
  "DIVERSITY GAIN",
  "DIVERSITY GAIN (coarse)",
  "DIVERSITY GAIN (fine)",
  "DIVERSITY PHASE",
  "DIVERSITY PHASE (coarse)",
  "DIVERSITY PHASE (fine)",
  "DRIVE",
  "IF SHIFT",
  "IF SHIFT RX1",
  "IF SHIFT RX2",
  "IF WIDTH",
  "IF WIDTH RX1",
  "IF WIDTH RX2",
  "MIC GAIN",
  "PAN",
  "PANADAPTER HIGH",
  "PANADAPTER LOW",
  "PANADAPTER STEP",
  "RF GAIN",
  "RF GAIN RX1",
  "RF GAIN RX2",
  "RIT",
  "RIT RX1",
  "RIT RX2",
  "SQUELCH",
  "SQUELCH RX1",
  "SQUELCH RX2",
  "TUNE DRIVE",
  "VFO",
  "WATERFALL HIGH",
  "WATERFALL LOW",
  "XIT",
  "ZOOM",
};

char *sw_string[SWITCH_ACTIONS] = {
  "NO ACTION",
  "A TO B",
  "A SWAP B",
  "AGC",
  "ANF",
  "B TO A",
  "BAND -",
  "BAND +",
  "BSTACK -",
  "BSTACK +",
  "CTUN",
  "DIV",
  "FILTER -",
  "FILTER +",
  "FUNCTION",
  "LOCK",
  "MENU BAND",
  "MENU BSTACK",
  "MENU DIV",
  "MENU FILTER",
  "MENU FREQUENCY",
  "MENU MEMORY",
  "MENU MODE",
  "MENU PS",
  "MODE -",
  "MODE +",
  "MOX",
  "MUTE",
  "NB",
  "NR",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "0",
  "CL",
  "EN",
  "PAN -",
  "PAN +",
  "PS",
  "RIT",
  "RIT CL",
  "SAT",
  "SNB",
  "SPLIT",
  "TUNE",
  "TWO TONE",
  "XIT",
  "XIT CL",
  "ZOOM -",
  "ZOOM +",
};

int rotary_action(void *data) {
  ACTION *arg=(ACTION *)data;
  RECEIVER *rx=radio->active_receiver;
  TRANSMITTER *tx=radio->transmitter;
  double d_temp;
  gint i_temp;
  g_print("%s: action=%s val=%d\n",__FUNCTION__,encoder_string[arg->action],arg->val);
  switch(arg->action) {
    case ENCODER_NO_ACTION:
      break;
    case ENCODER_AF_GAIN:
      d_temp=rx->volume;
      d_temp+=((double)arg->val/100.0);
      if(d_temp<0.0) d_temp=0.0;
      if(d_temp>1.0) d_temp=1.0;
      rx->volume=d_temp;
      update_vfo(rx);
      break;
    case ENCODER_AF_GAIN_RX1:
      break;
    case ENCODER_AF_GAIN_RX2:
      break;
    case ENCODER_AGC_GAIN:
      d_temp=rx->agc_gain;
      d_temp+=(double)arg->val;
      if(d_temp<-20.0) d_temp=-20.0;
      if(d_temp>120.0) d_temp=120.0;
      rx->agc_gain=d_temp;
      receiver_set_agc_gain(rx);
      update_vfo(rx);
      break;
    case ENCODER_AGC_GAIN_RX1:
      break;
    case ENCODER_AGC_GAIN_RX2:
      break;
    case ENCODER_ATTENUATION:
      break;
    case ENCODER_COMP:
      break;
    case ENCODER_CW_FREQUENCY:
      break;
    case ENCODER_CW_SPEED:
      break;
    case ENCODER_DIVERSITY_GAIN:
      break;
    case ENCODER_DIVERSITY_GAIN_COARSE:
      break;
    case ENCODER_DIVERSITY_GAIN_FINE:
      break;
    case ENCODER_DIVERSITY_PHASE:
      break;
    case ENCODER_DIVERSITY_PHASE_COARSE:
      break;
    case ENCODER_DIVERSITY_PHASE_FINE:
      break;
    case ENCODER_DRIVE:
      if(tx) {
        tx->drive+=(double)arg->val;
        if(tx->drive<0.0) tx->drive=0.0;
        if(tx->drive>100.0) tx->drive=100.0;
        if(radio->discovered->protocol==PROTOCOL_2) {
          protocol2_high_priority();
        }
      }
      break;
    case ENCODER_IF_SHIFT:
      break;
    case ENCODER_IF_SHIFT_RX1:
      break;
    case ENCODER_IF_SHIFT_RX2:
      break;
    case ENCODER_IF_WIDTH:
      break;
    case ENCODER_IF_WIDTH_RX1:
      break;
    case ENCODER_IF_WIDTH_RX2:
      break;
    case ENCODER_MIC_GAIN:
      if(tx) {
        d_temp = radio->transmitter->mic_gain + (double)arg->val;
        if (d_temp < -10.0) d_temp=-10.0; if (d_temp > 50.0) d_temp=50.0;
      }
      radio->transmitter->mic_gain=d_temp;
      update_radio(radio);
      break;
    case ENCODER_PAN:
      i_temp=rx->pan+(rx->zoom*arg->val);
      if(i_temp<0) {
        i_temp=0;
      } else if(i_temp>(rx->pixels-rx->panadapter_width)) {
        i_temp=rx->pixels-rx->panadapter_width;
      }
      rx->pan=d_temp;
      break;
    case ENCODER_PANADAPTER_HIGH:
      break;
    case ENCODER_PANADAPTER_LOW:
      break;
    case ENCODER_PANADAPTER_STEP:
      break;
    case ENCODER_RF_GAIN:
      break;
    case ENCODER_RF_GAIN_RX1:
      break;
    case ENCODER_RF_GAIN_RX2:
      break;
    case ENCODER_RIT:
      break;
    case ENCODER_RIT_RX1:
      break;
    case ENCODER_RIT_RX2:
      break;
    case ENCODER_SQUELCH:
      break;
    case ENCODER_SQUELCH_RX1:
      break;
    case ENCODER_SQUELCH_RX2:
      break;
    case ENCODER_TUNE_DRIVE:
      break;
    case ENCODER_VFO:
      if(!rx->locked) {
        receiver_move(rx,(long long)(rx->step*arg->val),TRUE);
      }
      break;
    case ENCODER_WATERFALL_HIGH:
      break;
    case ENCODER_WATERFALL_LOW:
      break;
    case ENCODER_XIT:
      break;
    case ENCODER_ZOOM:
      break;
    case ENCODER_ACTIONS:
      break;
  }
  g_free(data);
  return 0;
}


int switch_action(void * data) {
  ACTION *arg=(ACTION *)data;
  RECEIVER *rx=radio->active_receiver;
  gint i_temp;
  g_print("%s: action=%s\n",__FUNCTION__,sw_string[arg->action]);
  switch(arg->action) {
    case NO_ACTION:
      break;
    case A_TO_B:
      vfo_a2b(rx);
      break;
    case A_SWAP_B:
      vfo_aswapb(rx);
      break;
    case AGC:
      if(rx->agc==AGC_FAST) {
        rx->agc=0;
      } else {
	 rx->agc++;
      }
      set_agc(rx);
      update_vfo(rx);
      break;
    case ANF:
      rx->anf=!rx->anf;
      update_noise(rx);
      break;
    case B_TO_A:
      vfo_b2a(rx);
      break;
    case BAND_MINUS:
      set_band(rx,previous_band(rx->band_a),-1);
      update_vfo(rx);
      break;
    case BAND_PLUS:
      set_band(rx,next_band(rx->band_a),-1);
      update_vfo(rx);
      break;
    case BANDSTACK_MINUS:
      break;
    case BANDSTACK_PLUS:
      break;
    case CTUN:
      rx->ctun=!rx->ctun;
      receiver_set_ctun(rx);
      break;
    case DIVERSITY:
      break;
    case FILTER_MINUS:
      i_temp=rx->filter_a-1;
      if(i_temp<0) i_temp=FILTERS-1;
      receiver_filter_changed(rx,i_temp);
      break;
    case FILTER_PLUS:
      i_temp=rx->filter_a+1;
      if(i_temp>-FILTERS) i_temp=0;
      receiver_filter_changed(rx,i_temp);
      break;
    case FUNCTION:
      break;
    case LOCK:
      rx->locked=!rx->locked;
      update_vfo(rx);
      break;
    case MENU_BAND:
      break;
    case MENU_BANDSTACK:
      break;
    case MENU_DIVERSITY:
      break;
    case MENU_FILTER:
      break;
    case MENU_FREQUENCY:
      break;
    case MENU_MEMORY:
      break;
    case MENU_MODE:
      break;
    case MENU_PS:
      break;
    case MODE_MINUS:
      break;
    case MODE_PLUS:
      break;
    case MOX:
      if(radio->can_transmit) set_mox(radio,!radio->mox);
      break;
    case MUTE:
      break;
    case NB:
      if(rx->nb) {
        rx->nb = FALSE;
        rx->nb2= TRUE;
      } else if(rx->nb2) {
        rx->nb = FALSE;
        rx->nb2= FALSE;
      } else {
        rx->nb = TRUE;
        rx->nb2= FALSE;
      }
      update_noise(rx);
      break;
    case NR:
      if(rx->nr) {
        rx->nr = FALSE;
        rx->nr2= TRUE;
      } else if(rx->nr2) {
        rx->nr = FALSE;
        rx->nr2= FALSE;
      } else {
        rx->nr = TRUE;
        rx->nr2= FALSE;
      }
      update_noise(rx);
      break;
    case NUM_1:
      if(arg->function) {
      } else {
        // Band 160
	set_band(rx,band160,-1);
      }
      break;
    case NUM_2:
      if(arg->function) {
      } else {
        // Band 80
	set_band(rx,band80,-1);
      }
      break;
    case NUM_3:
      if(arg->function) {
      } else {
        // Band 60
	set_band(rx,band60,-1);
      }
      break;
    case NUM_4:
      if(arg->function) {
      } else {
        // Band 40
	set_band(rx,band40,-1);
      }
      break;
    case NUM_5:
      if(arg->function) {
      } else {
        // Band 30
	set_band(rx,band30,-1);
      }
      break;
    case NUM_6:
      if(arg->function) {
      } else {
        // Band 20
	set_band(rx,band20,-1);
      }
      break;
    case NUM_7:
      if(arg->function) {
      } else {
        // Band 17
	set_band(rx,band17,-1);
      }
      break;
    case NUM_8:
      if(arg->function) {
      } else {
        // Band 15
	set_band(rx,band15,-1);
      }
      break;
    case NUM_9:
      if(arg->function) {
      } else {
        // Band 12
	set_band(rx,band12,-1);
      }
      break;
    case NUM_0:
      if(arg->function) {
      } else {
        // Band 6
	set_band(rx,band6,-1);
      }
      break;
    case NUM_CLEAR:
      if(arg->function) {
      } else {
        // Band 10
	set_band(rx,band10,-1);
      }
      break;
    case NUM_ENTER:
      if(arg->function) {
      } else {
        // Band Gen
	set_band(rx,bandGen,-1);
      }
      break;
    case PAN_MINUS:
      i_temp=rx->pan-rx->zoom;
      if(i_temp<0) {
        i_temp=0;
      }
      rx->pan=i_temp;
      break;
    case PAN_PLUS:
      i_temp=rx->pan+rx->zoom;
      if(i_temp>(rx->pixels-rx->panadapter_width)) {
        i_temp=rx->pixels-rx->panadapter_width;
      }
      rx->pan=i_temp;
      break;
    case PS:
      break;
    case RIT:
      rx->rit_enabled=!rx->rit_enabled;
      update_vfo(rx);
      break;
    case RIT_CLEAR:
      rx->rit=0;
      update_vfo(rx);
      break;
    case SAT:
      switch (rx->split) {
        case SPLIT_OFF:
          rx->split=SPLIT_SAT;
          break;
        case SPLIT_SAT:
          rx->split=SPLIT_RSAT;
          break;
        case SPLIT_RSAT:
        default:
          rx->split=SPLIT_OFF;
          break;
      }
      update_vfo(rx);
      break;
    case SNB:
      rx->snb=!rx->snb;
      update_noise(rx);
      break;
    case SPLIT:
      if(rx->split==SPLIT_OFF) {
        rx->split=SPLIT_ON;
        if(radio->transmitter) transmitter_set_mode(radio->transmitter,rx->mode_b);
      } else {
        rx->split=SPLIT_OFF;
        if(radio->transmitter) transmitter_set_mode(radio->transmitter,rx->mode_a);
      }
      update_vfo(rx);
      break;
    case TUNE:
      if(radio->can_transmit) set_tune(radio,!radio->tune);
      break;
    case TWO_TONE:
      break;
    case XIT:
      radio->transmitter->xit_enabled = !radio->transmitter->xit_enabled;
      update_vfo(rx);
      break;
    case XIT_CLEAR:
      if(radio->can_transmit && !isTransmitting(radio)) {
        radio->transmitter->xit = 0;
        radio->transmitter->xit_enabled = 0;
        update_vfo(rx);
      }
      break;
    case ZOOM_MINUS:
      i_temp = rx->zoom-1;
      if(i_temp>=1 && i_temp<=8) {
        receiver_change_zoom(rx,i_temp);
        update_vfo(rx);
      }
      break;
    case ZOOM_PLUS:
      i_temp = rx->zoom+1;
      if(i_temp>=1 && i_temp<=8) {
        receiver_change_zoom(rx,i_temp);
        update_vfo(rx);
      }
      break;
    case SWITCH_ACTIONS:
      break;
  }
  g_free(data);
  return 0;
}
