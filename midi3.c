/*
 * Layer-3 of MIDI support
 * 
 * (C) Christoph van Wullen, DL1YCF
 *
 *
 * In most cases, a certain action only makes sense for a specific
 * type. For example, changing the VFO frequency will only be implemeted
 * for MIDI_WHEEL, and TUNE off/on only with MIDI_KNOB.
 *
 * However, changing the volume makes sense both with MIDI_KNOB and MIDI_WHEEL.
 */
#include <gtk/gtk.h>

#include "band.h"
#include "channel.h"
#include "agc.h"
#include "discovered.h"
#include "bpsk.h"
#include "mode.h"
#include "filter.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "adc.h"
#include "dac.h"
#include "radio.h"
#include "main.h"
#include "protocol1.h"
#include "audio.h"
#include "signal.h"
#include "vfo.h"
#include "transmitter.h"
#include "ext.h"

#include "midi.h"
#ifdef CWDAEMON
#include "cwdaemon.h"
#include <libcw.h>
#endif

int midi_rx;
int midi_debug=FALSE;

typedef struct _action {
  enum MIDIaction action;
  enum MIDItype type;
  int val;
} ACTION;

static int midi_action(void *data) {
    ACTION *a=(ACTION*)data;
    enum MIDIaction action=a->action;
    enum MIDItype type=a->type;
    int val=a->val;

    int new;
    double dnew;
    double *dp;
    int    *ip;
    RECEIVER *rx=radio->receiver[midi_rx];

    if(midi_debug) g_print("%s: action=%d type=%d val=%d\n",__FUNCTION__,action,type,val);
    //
    // Handle cases in alphabetical order of the key words in midi.props
    //
    switch (action) {
	/////////////////////////////////////////////////////////// "A2B"
	case VFO_A2B: // only key supported
	    if (type == MIDI_KEY) {
              vfo_a2b(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "AFGAIN"
	case MIDI_AF_GAIN: // knob or wheel supported
            switch (type) {
	      case MIDI_KNOB:
		rx->volume = 0.01*val;
		break;
	      case MIDI_WHEEL:	
		dnew=rx->volume += 0.01*val;
		if (dnew < 0.0) dnew=0.0; if (dnew > 1.0) dnew=1.0;
		rx->volume = dnew;
		break;
	      default:
		// do not change volume
		// we should not come here anyway
		break;
	    }
	    receiver_set_volume(rx);
	    update_vfo(rx);
	    break;
	/////////////////////////////////////////////////////////// "AGCATTACK"
	case AGCATTACK: // only key supported
	    // cycle through fast/med/slow AGC attack
	    if (type == MIDI_KEY) {
	      new=rx->agc + 1;
	      if (new > AGC_FAST) new=0;
	      rx->agc=new;
	      set_agc(rx);
	      update_vfo(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "AGCVAL"
	case MIDI_AGC: // knob or wheel supported
	    switch (type) {
	      case MIDI_KNOB:
		dnew = -20.0 + 1.4*val;
		break;
	      case MIDI_WHEEL:
		dnew=rx->agc_gain + val;
		if (dnew < -20.0) dnew=-20.0; if (dnew > 120.0) dnew=120.0;
		break;
	      default:
		// do not change value
		// we should not come here anyway
		dnew=rx->agc_gain;
		break;
	    }
	    rx->agc_gain=dnew;
            receiver_set_agc_gain(rx);
	    update_vfo(rx);
	    break;
	/////////////////////////////////////////////////////////// "ANF"
	case ANF:	// only key supported
	    if (type == MIDI_KEY) {
                rx->anf=!rx->anf;
                update_noise(rx);
            }
	    break;
/*
	/////////////////////////////////////////////////////////// "ATT"
	case ATT:	// Key for ALEX attenuator, wheel or knob for slider
	    switch(type) {
	      case MIDI_KEY:
		if (filter_board == ALEX && active_receiver->adc == 0) {
		  new=active_receiver->alex_attenuation + 1;
		  if (new > 3) new=0;
		  g_idle_add(ext_set_alex_attenuation, GINT_TO_POINTER(new));
		  g_idle_add(ext_update_att_preamp, NULL);
		}
		break;
	      case MIDI_WHEEL:
		new=adc_attenuation[active_receiver->adc] + val;
		dp=malloc(sizeof(double));
		*dp=(double) new;
                if(have_rx_gain) {
                  if(*dp<-12.0) {
                    *dp=-12.0;
                  } else if(*dp>48.0) {
                    *dp=48.0;
                  }
                } else {
                  if(*dp<0.0) {
                    *dp=0.0;
                  } else if (*dp>31.0) {
                    *dp=31.0;
                  }
                }
		g_idle_add(ext_set_attenuation_value,(gpointer) dp);
		break;
	      case MIDI_KNOB:
		dp=malloc(sizeof(double));
                if (have_rx_gain) {
		  *dp=-12.0 + 0.6*(double) val;
                } else {
                  *dp = 0.31 * (double) val;
                }
		g_idle_add(ext_set_attenuation_value,(gpointer) dp);
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
	    break;
*/
	/////////////////////////////////////////////////////////// "B2A"
	case VFO_B2A: // only key supported
	    if (type == MIDI_KEY) {
              vfo_b2a(rx);
            }
	    break;
	/////////////////////////////////////////////////////////// "NUMBER PAD"
	case NUMPAD_0:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(0));
	    break;
	case NUMPAD_1:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(1));
	    break;
	case NUMPAD_2:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(2));
	    break;
	case NUMPAD_3:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(3));
	    break;
	case NUMPAD_4:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(4));
	    break;
	case NUMPAD_5:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(5));
	    break;
	case NUMPAD_6:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(6));
	    break;
	case NUMPAD_7:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(7));
	    break;
	case NUMPAD_8:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(8));
	    break;
	case NUMPAD_9:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(9));
	    break;
	case NUMPAD_CL:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(-1));
	    break;
	case NUMPAD_ENTER:
	    g_idle_add(ext_num_pad,GINT_TO_POINTER(-2));
	    break;

	/////////////////////////////////////////////////////////// "MIDIBAND"
        /////////////////////////////////////////////////////////// "BANDUP"
        case MIDI_BAND_10:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band10));
            }
            break;
        case MIDI_BAND_12:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band12));
            }
            break;
#ifdef SOAPYSDR
        case MIDI_BAND_1240:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band1240));
            }
            break;
        case MIDI_BAND_144:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band144));
            }
            break;
#endif
        case MIDI_BAND_15:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band15));
            }
            break;
        case MIDI_BAND_160:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band160));
            }
            break;
        case MIDI_BAND_17:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band17));
            }
            break;
        case MIDI_BAND_20:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band20));
            }
            break;
#ifdef SOAPYSDR
        case MIDI_BAND_220:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band220));
            }
            break;
        case MIDI_BAND_2300:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band2300));
            }
            break;
#endif
        case MIDI_BAND_30:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band30));
            }
            break;
#ifdef SOAPYSDR
        case MIDI_BAND_3400:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band3400));
            }
            break;
        case MIDI_BAND_70:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band70));
            }
            break;
#endif
        case MIDI_BAND_40:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band40));
            }
            break;
#ifdef SOAPYSDR
        case MIDI_BAND_430:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band430));
            }
            break;
#endif
        case MIDI_BAND_6:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band6));
            }
            break;
        case MIDI_BAND_60:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band60));
            }
            break;
        case MIDI_BAND_80:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band80));
            }
            break;
#ifdef SOAPYSDR
        case MIDI_BAND_902:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(band902));
            }
            break;
        case MIDI_BAND_AIR:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(bandAIR));
            }
            break;
#endif
        case MIDI_BAND_GEN:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(bandGen));
            }
            break;
        case MIDI_BAND_WWV:
            if (type == MIDI_KEY) {
              g_idle_add(ext_band_select, GINT_TO_POINTER(bandWWV));
            }
            break;

	/////////////////////////////////////////////////////////// "BANDDOWN"
	/////////////////////////////////////////////////////////// "BANDUP"
        case BAND_DOWN:
	case BAND_UP:
	    switch (type) {
	      case MIDI_KEY:
		new=(action == BAND_UP) ? 1 : -1;
		break;
	      case MIDI_WHEEL:
		new=val > 0 ? 1 : -1;
		break;
	      case MIDI_KNOB:
		// cycle through the bands
		new = ((BANDS-1) * val) / 100 - rx->band_a;
		break;
	      default:
		// do not change
		// we should not come here anyway
		new=0;
		break;
	    }
	    //
	    // If the band has not changed, do nothing. Otherwise
	    // vfo.c will loop through the band stacks
	    //
	    switch(new) {
	      case -1:
	        set_band(rx,previous_band(rx->band_a),-1);
		update_vfo(rx);
		break;
	      case 0:
	        break;
	      case 1:
	        set_band(rx,next_band(rx->band_a),-1);
		update_vfo(rx);
		break;
	    }
	    break;
/*
	/////////////////////////////////////////////////////////// "COMPRESS"
	case COMPRESS: // wheel or knob
	    switch (type) {
	      case MIDI_WHEEL:
		dnew=transmitter->compressor_level + val;
		if (dnew > 20.0) dnew=20.0;
		if (dnew < 0 ) dnew=0;
		break;
	      case MIDI_KNOB:
		dnew=(20.0*val)/100.0;
		break;
	      default:
		// do not change
		// we should not come here anyway
		dnew=transmitter->compressor_level;
		break;
	    }
	    transmitter->compressor_level=dnew;
	    // automatically engange compressor if level > 0.5
	    if (dnew < 0.5) transmitter->compressor=0;
	    if (dnew > 0.5) transmitter->compressor=1;
	    g_idle_add(ext_set_compression, NULL);
	    break;
*/
	/////////////////////////////////////////////////////////// "CTUN"
	case MIDI_CTUN: // only key supported
	    // toggle CTUN
	    if (type == MIDI_KEY) {
	         rx->ctun=!rx->ctun;
                 receiver_set_ctun(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "CURRVFO"
	case VFO: // only wheel supported
	    if (type == MIDI_WHEEL && !rx->locked) {
              receiver_move(rx,(long long)(rx->step*val),TRUE);
            }
	    break;
	/////////////////////////////////////////////////////////// "CWL"
	/////////////////////////////////////////////////////////// "CWR"
	case CWLEFT: // only key
	case CWRIGHT: // only key
//#ifdef LOCALCW
	    if (type == MIDI_KEY) {
		new=(action == CWL);
		//keyer_event(new,val);
	    }
//#endif
	    break;
	/////////////////////////////////////////////////////////// "CWSPEED"
	case CWSPEED: // knob or wheel
            switch (type) {
              case MIDI_KNOB:
		// speed between 5 and 35 wpm
                new= (int) (5.0 + (double) val * 0.3);
                break;
              case MIDI_WHEEL:
		// here we allow from 1 to 60 wpm
                new = radio->cw_keyer_speed + val;
		if (new <  1) new=1;
		if (new > 60) new=60;
                break;
              default:
                // do not change
                // we should not come here anyway
                new = radio->cw_keyer_speed;
                break;
            }
	    radio->cw_keyer_speed=new;
//#ifdef LOCALCW
	    //keyer_update();
//#endif
            g_idle_add(ext_vfo_update, NULL);
	    break;
/*
	/////////////////////////////////////////////////////////// "DIVCOARSEGAIN"
	case DIV_COARSEGAIN:  // knob or wheel supported
	case DIV_FINEGAIN:    // knob or wheel supported
	case DIV_GAIN:        // knob or wheel supported
            switch (type) {
              case MIDI_KNOB:
                if (action == DIV_COARSEGAIN || action == DIV_GAIN) {
		  // -25 to +25 dB in steps of 0.5 dB
		  dnew = 10.0*(-25.0 + 0.5*val - div_gain);
		} else {
		  // round gain to a multiple of 0.5 dB and apply a +/- 0.5 dB update
                  new = (int) (2*div_gain + 1.0) / 2;
		  dnew = 10.0*((double) new + 0.01*val - 0.5 - div_gain);
		}
                break;
              case MIDI_WHEEL:
                // coarse: increaments in steps of 0.25 dB, medium: steps of 0.1 dB fine: in steps of 0.01 dB
                if (action == DIV_GAIN) {
		  dnew = val*0.5;
		} else if (action == DIV_COARSEGAIN) {
		  dnew = val*2.5;
		} else {
		  dnew = val * 0.1;
	 	}
                break;
              default:
                // do not change
                // we should not come here anyway
		dnew = 0.0;
                break;
            }
	    // dnew is the delta times 10
	    dp=malloc(sizeof(double));
	    *dp=dnew;
            g_idle_add(ext_diversity_change_gain, dp);
            break;
        /////////////////////////////////////////////////////////// "DIVPHASE"
        case DIV_COARSEPHASE:   // knob or wheel supported
        case DIV_FINEPHASE:     // knob or wheel supported
	case DIV_PHASE:		// knob or wheel supported
            switch (type) {
              case MIDI_KNOB:
		// coarse: change phase from -180 to 180
                // fine: change from -5 to 5
                if (action == DIV_COARSEPHASE || action == DIV_PHASE) {
		  // coarse: change phase from -180 to 180 in steps of 3.6 deg
                  dnew = (-180.0 + 3.6*val - div_phase);
                } else {
		  // fine: round to multiple of 5 deg and apply a +/- 5 deg update
                  new = 5 * ((int) (div_phase+0.5) / 5);
                  dnew =  (double) new + 0.1*val -5.0 -div_phase;
                }
                break;
              case MIDI_WHEEL:
		if (action == DIV_PHASE) {
		  dnew = val*0.5; 
		} else if (action == DIV_COARSEPHASE) {
		  dnew = val*2.5;
		} else if (action == DIV_FINEPHASE) {
		  dnew = 0.1*val;
		}
                break;
              default:
                // do not change
                // we should not come here anyway
                dnew = 0.0;
                break;
            }
            // dnew is the delta
            dp=malloc(sizeof(double));
            *dp=dnew;
            g_idle_add(ext_diversity_change_phase, dp);
            break;
        /////////////////////////////////////////////////////////// "DIVTOGGLE"
        case DIV_TOGGLE:   // only key supported
            if (type == MIDI_KEY) {
                // enable/disable DIVERSITY
                diversity_enabled = diversity_enabled ? 0 : 1;
                g_idle_add(ext_vfo_update, NULL);
            }
            break;
*/
	/////////////////////////////////////////////////////////// "DUP"
        case MIDI_DUP:
	    if (radio->can_transmit && !isTransmitting(radio)) {
              rx->duplex=!rx->duplex;
	      update_vfo(rx);
	    }
            break;
	/////////////////////////////////////////////////////////// "FILTERDOWN"
	/////////////////////////////////////////////////////////// "FILTERUP"
	case FILTER_DOWN:
	case FILTER_UP:
	    //
	    // In filter.c, the filters are sorted such that the widest one comes first
	    // Therefore let FILTER_UP move down.
	    //
	    switch (type) {
	      case MIDI_KEY:
		new=(action == FILTER_UP) ? -1 : 1;
		break;
	      case MIDI_WHEEL:
		new=val > 0 ? -1 : 1;
		break;
	      case MIDI_KNOB:
		// cycle through all the filters: val=100 maps to filter #0
		new = ((FILTERS-1) * (val-100)) / 100 - rx->filter_a;
		break;
	      default:
		// do not change filter setting
		// we should not come here anyway
		new=0;
		break;
	    }
	    if (new != 0) {
	      new+=rx->filter_a;
	      if (new >= FILTERS) new=0;
	      if (new <0) new=FILTERS-1;
	      receiver_filter_changed(rx,new);
	    }
	    break;
	/////////////////////////////////////////////////////////// "LOCK"
	case MIDI_LOCK: // only key supported
	    if (type == MIDI_KEY) {
	      rx->locked=!rx->locked;
	      update_vfo(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "MICGAIN"
	case MIC_VOLUME: // knob or wheel supported
	    // TODO: possibly adjust linein value if that is effective
	    if(radio->transmitter) {
	      switch (type) {
  	        case MIDI_KNOB:
  	  	  dnew=-10.0 + 0.6*val;
  		  break;
  	        case MIDI_WHEEL:
  		  dnew = radio->transmitter->mic_gain + val;
  		  if (dnew < -10.0) dnew=-10.0; if (dnew > 50.0) dnew=50.0;
  		  break;
  	        default:
  		  // do not change mic gain
  		  // we should not come here anyway
  		  dnew = radio->transmitter->mic_gain;
  		  break;
	      }
	      radio->transmitter->mic_gain=dnew;
	      update_radio(radio);
	    }
	    break;
	/////////////////////////////////////////////////////////// "MODEDOWN"
	/////////////////////////////////////////////////////////// "MODEUP"
	case MODE_DOWN:
	case MODE_UP:
	    switch (type) {
	      case MIDI_KEY:
		new=(action == MODE_UP) ? 1 : -1;
		break;
	      case MIDI_WHEEL:
		new=val > 0 ? 1 : -1;
		break;
	      case MIDI_KNOB:
		// cycle through all the modes
		new = ((MODES-1) * val) / 100 - rx->mode_a;
		break;
	      default:
		// do not change
		// we should not come here anyway
		new=0;
		break;
	    }
	    if (new != 0) {
	      new+=rx->mode_a;
	      if (new >= MODES) new=0;
	      if (new <0) new=MODES-1;
	      receiver_mode_changed(rx,new);
	    }
	    break;
	/////////////////////////////////////////////////////////// "MOX"
	case MIDI_MOX: // only key supported
	    if (type == MIDI_KEY && radio->can_transmit) {
	        new = !radio->mox;
		set_mox(radio,new);
	    }
	    break;    
/*
        /////////////////////////////////////////////////////////// "MUTE"
        case MIDI_MUTE:
            if (type == MIDI_KEY) {
              g_idle_add(ext_mute_update,NULL);
	    }
            break;
*/
	/////////////////////////////////////////////////////////// "NOISEBLANKER"
	case MIDI_NB: // only key supported
	    // cycle through NoiseBlanker settings: OFF, NB, NB2
            if (type == MIDI_KEY) {
	      if (rx->nb) {
		rx->nb = FALSE;
		rx->nb2= TRUE;
	      } else if (rx->nb2) {
		rx->nb = FALSE;
		rx->nb2= FALSE;
	      } else {
		rx->nb = TRUE;
		rx->nb2= FALSE;
	      }
	      update_noise(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "NOISEREDUCTION"
	case MIDI_NR: // only key supported
	    // cycle through NoiseReduction settings: OFF, NR1, NR2
	    if (type == MIDI_KEY) {
	      if (rx->nr) {
		rx->nr = FALSE;
		rx->nr2= TRUE;
	      } else if (rx->nr2) {
		rx->nr = FALSE;
		rx->nr2= FALSE;
	      } else {
		rx->nr = TRUE;
		rx->nr2= FALSE;
	      }
	      update_noise(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "PAN"
        case MIDI_PAN:  // wheel and knob
	    switch (type) {
              case MIDI_WHEEL:
		// val = +1 or -1
		new=rx->pan+(rx->zoom*val);
		if(new<0) {
		  new=0;
		} else if(new>(rx->pixels-rx->panadapter_width)) {
	          new=rx->pixels-rx->panadapter_width;
		}
	        rx->pan=new;
                break;
	      case MIDI_KNOB:
		// val = 0..100
	        new=(int)(((double)(rx->pixels-rx->panadapter_width)/100.0)*(double)val);
	        rx->pan=new;
                break;
	      default:
		// no action for keys (we should not come here anyway)
		break;
            }
            break;
	/////////////////////////////////////////////////////////// "PANHIGH"
	case PAN_HIGH:  // wheel or knob
	    switch (type) {
	      case MIDI_WHEEL:
		if (radio->can_transmit && isTransmitting(radio)) {
		    // TX panadapter affected
		    radio->transmitter->panadapter_high += val;
		} else {
		    rx->panadapter_high += val;
		}
		break;
	    case MIDI_KNOB:
		// Adjust "high water" in the range -50 ... 0 dBm
		new = -50 + val/2;
		if (radio->can_transmit && isTransmitting(radio)) {
		    radio->transmitter->panadapter_high += val;
		} else {
		    rx->panadapter_high = new;
		}
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
	    break;
	/////////////////////////////////////////////////////////// "PANLOW"
	case PAN_LOW:  // wheel and knob
	    switch (type) {
	      case MIDI_WHEEL:
		if (radio->can_transmit && isTransmitting(radio)) {
		    // TX panadapter affected
		    radio->transmitter->panadapter_low += val;
		} else {
		    rx->panadapter_low += val;
		}
		break;
	      case MIDI_KNOB:
		if (radio->can_transmit && isTransmitting(radio)) {
		    // TX panadapter: use values -100 through -50
		    new = -100 + val/2;
		    radio->transmitter->panadapter_low =new;
		} else {
		    // RX panadapter: use values -140 through -90
		    new = -140 + val/2;
		    rx->panadapter_low = new;
		}
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
	    break;
/*
	/////////////////////////////////////////////////////////// "PREAMP"
	case PRE:	// only key supported
	    if (type == MIDI_KEY) {
	    }
	    break;
	/////////////////////////////////////////////////////////// "PURESIGNAL"
	case MIDI_PS: // only key supported
#ifdef PURESIGNAL
	    // toggle PURESIGNAL
	    if (type == MIDI_KEY) {
	      new=!(transmitter->puresignal);
	      g_idle_add(ext_tx_set_ps,GINT_TO_POINTER(new));
	    }
#endif
	    break;
	/////////////////////////////////////////////////////////// "RFGAIN"
        case MIDI_RF_GAIN: // knob or wheel supported
            if (type == MIDI_KNOB) {
                new=val;
            } else  if (type == MIDI_WHEEL) {
                new=(int)active_receiver->rf_gain+val;
            }
            g_idle_add(ext_set_rf_gain, GINT_TO_POINTER((int)new));
	    break;
	/////////////////////////////////////////////////////////// "RFPOWER"
	case TX_DRIVE: // knob or wheel supported
	    switch (type) {
	      case MIDI_KNOB:
		dnew = val;
		break;
	      case MIDI_WHEEL:
		dnew=transmitter->drive + val;
		if (dnew < 0.0) dnew=0.0; if (dnew > 100.0) dnew=100.0;
		break;
	      default:
		// do not change value
		// we should not come here anyway
		dnew=transmitter->drive;
		break;
	    }
	    dp=malloc(sizeof(double));
	    *dp=dnew;
	    g_idle_add(ext_set_drive, (gpointer) dp);
	    break;
*/
	/////////////////////////////////////////////////////////// "RITCLEAR"
	case MIDI_RIT_CLEAR:	  // only key supported
	    if (type == MIDI_KEY) {
	      // clear RIT value
	      rx->rit=0;
	      update_vfo(rx);
	    }
/*
	/////////////////////////////////////////////////////////// "RITSTEP"
        case RIT_STEP: // key or wheel supported
            // This cycles between RIT increments 1, 10, 100, 1, 10, 100, ...
            switch (type) {
              case MIDI_KEY:
                // key cycles through in upward direction
                val=1;
                // FALLTHROUGH
              case MIDI_WHEEL:
                // wheel cycles upward or downward
                if (val > 0) {
                  rit_increment=10*rit_increment;
                } else {
                  rit_increment=rit_increment/10;
                }
                if (rit_increment < 1) rit_increment=100;
                if (rit_increment > 100) rit_increment=1;
                break;
              default:
                // do nothing
                break;
            }
	    update_vfo(rx);
            break;
*/
	/////////////////////////////////////////////////////////// "RITTOGGLE"
	case RIT_TOGGLE:  // only key supported
	    if (type == MIDI_KEY) {
		// enable/disable RIT
		new=rx->rit_enabled;
		rx->rit_enabled = new ? FALSE : TRUE;
		update_vfo(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "RITVAL"
	case RIT_VAL:	// wheel or knob
	    switch (type) {
	      case MIDI_WHEEL:
		if(rx->mode_a==CWL || rx->mode_a==CWU) {
                  new=rx->rit=10*val;
                } else {
                  new=rx->rit=50*val;
                }
		if (new >  10000) new= 10000;
		if (new < -10000) new=-10000;
		rx->rit = new;
		break;
	      case MIDI_KNOB:
	 	// knob: adjust in the range +/ 50*rit_increment
		if(rx->mode_a==CWL || rx->mode_a==CWU) {
                  new=10*(val-50);
                } else {
                  new=rx->rit=50*(val-50);
                }
		rx->rit = new;
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
	    // enable/disable RIT according to RIT value
	    rx->rit_enabled = (rx->rit == 0) ? FALSE : TRUE;
	    update_vfo(rx);
	    break;
	/////////////////////////////////////////////////////////// "SAT"
        case MIDI_SAT:
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
	/////////////////////////////////////////////////////////// "SNB"
	case SNB:	// only key supported
	    if (type == MIDI_KEY) {
		rx->snb=!rx->snb;
	        update_noise(rx);
	    }
	    break;
	/////////////////////////////////////////////////////////// "SPLIT"
	case MIDI_SPLIT: // only key supported
	    // toggle split mode
	    if(rx->split==SPLIT_OFF) {
	      rx->split=SPLIT_ON;
	      if(radio->transmitter) transmitter_set_mode(radio->transmitter,rx->mode_b);
            } else {
	      rx->split=SPLIT_OFF;
	      if(radio->transmitter) transmitter_set_mode(radio->transmitter,rx->mode_a);
            }
	    update_vfo(rx);
	    break;
	case SWAP_RX:	// only key supported
	    if (type == MIDI_KEY && radio->receivers > 1) {
		new=midi_rx+1;
		if(new==MAX_RECEIVERS) new=0;
		while(radio->receiver[new]==NULL) {
	  	    new++;
		    if(new==MAX_RECEIVERS) new=0;
	        }
		midi_rx=new;
		update_vfo(rx);
		rx=radio->receiver[midi_rx];
		update_vfo(rx);
	    }
	    break;    
	/////////////////////////////////////////////////////////// "SWAPVFO"
	case SWAP_VFO:	// only key supported
	    if (type == MIDI_KEY) {
		vfo_aswapb(rx);
	    }
	    break;    
	/////////////////////////////////////////////////////////// "TUNE"
	case MIDI_TUNE: // only key supported
	    if (type == MIDI_KEY && radio->can_transmit) {
	        new = !radio->tune;
		set_tune(radio,new);
	    }
	    break;    
	/////////////////////////////////////////////////////////// "VFOA"
	/////////////////////////////////////////////////////////// "VFOB"
	case VFOA: // only wheel supported
	    if (type == MIDI_WHEEL && !rx->locked) {
              g_print("%s: VFOA: val=%d\n",__FUNCTION__,val);
              receiver_move(rx,(long long)(rx->step*val),TRUE);
	    }
	    break;
	case VFOB: // only wheel supported
	    if (type == MIDI_WHEEL && !rx->locked) {
              receiver_move_b(rx,(long long)(rx->step*-val),FALSE,TRUE);
	    }
	    break;
	/////////////////////////////////////////////////////////// "VFOSTEPDOWN"
	/////////////////////////////////////////////////////////// "VFOSTEPUP"
        case VFO_STEP_DOWN: // key or wheel supported
        case VFO_STEP_UP:
	    switch (type) {
	      case MIDI_KEY:
		new =  (action == VFO_STEP_UP) ? 1 : -1;
		receiver_move(rx,rx->step*new,TRUE);
		break;
	      case MIDI_WHEEL:
		new = (val > 0) ? 1 : -1;
		receiver_move(rx,rx->step*new,TRUE);
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
            break;
	/////////////////////////////////////////////////////////// "VOX"
	case VOX: // only key supported
	    // toggle VOX
	    if (type == MIDI_KEY) {
	      radio->vox_enabled = !radio->vox_enabled;
	      update_radio(radio);
	    }
	    break;
	/////////////////////////////////////////////////////////// "VOXLEVEL"
	case VOXLEVEL: // knob or wheel supported
            switch (type) {
              case MIDI_WHEEL:
                // This changes the value incrementally,
                // but stay within limits (0.0 through 1.0)
                radio->vox_threshold += (double) val * 0.01;
		if (radio->vox_threshold > 1.0) radio->vox_threshold=1.0;
		if (radio->vox_threshold < 0.0) radio->vox_threshold=0.0;
		update_radio(radio);
                break;
              case MIDI_KNOB:
                radio->vox_threshold = 0.01 * (double) val;
		update_radio(radio);
                break;
              default:
                // do nothing
                // we should not come here anyway
                break;
            }
	    break;
	/////////////////////////////////////////////////////////// "XITCLEAR"
        case MIDI_XIT_CLEAR:  // only key supported
            if (type == MIDI_KEY) {
                // this clears the XIT value and disables XIT
		if (radio->can_transmit && !isTransmitting(radio)) {
                  radio->transmitter->xit = 0;
                  radio->transmitter->xit_enabled = 0;
		  update_vfo(rx);
                }
            }
            break;
	/////////////////////////////////////////////////////////// "XITVAL"
        case XIT_VAL:   // wheel and knob supported.
	    if (radio->can_transmit && !isTransmitting(radio)) {
              switch (type) {
                case MIDI_WHEEL:
                  // This changes the XIT value incrementally,
                  // but we restrict the change to +/ 9.999 kHz
	          if(rx->mode_a==CWL || rx->mode_a==CWU) {
                         new=radio->transmitter->xit=10*val;
                  } else {
                         new=radio->transmitter->xit=50*val;
                  }
                  if (new >  10000) new= 10000;
                  if (new < -10000) new=-10000;
                  radio->transmitter->xit = new;
                  break;
                case MIDI_KNOB:
                  // knob: adjust in the range +/ 50*rit_increment
	          if(rx->mode_a==CWL || rx->mode_a==CWU) {
                         new=radio->transmitter->xit=10*(val-50);
                  } else {
                         new=radio->transmitter->xit=50*(val-50);
                  }
                  radio->transmitter->xit = new;
                  break;
                default:
                  // do nothing
                  // we should not come here anyway
                  break;
              }
              // enable/disable XIT according to XIT value
              radio->transmitter->xit_enabled = (radio->transmitter->xit == 0) ? FALSE : TRUE;
	      update_vfo(rx);
	    }
            break;
	/////////////////////////////////////////////////////////// "ZOOM"
        case MIDI_ZOOM:  // wheel and knob
            switch (type) {
              case MIDI_WHEEL:
		new=rx->zoom+val;
		if(new>=1 && new<=8) {
		  receiver_change_zoom(rx,new);
		}
		update_vfo(rx);
                break;
              case MIDI_KNOB:
                if(val>=1 && val<=8) {
		  receiver_change_zoom(rx,val);
		  update_vfo(rx);
		}
                break;
	      default:
		// no action for keys (should not come here anyway)
		break;
            }
            break;
	/////////////////////////////////////////////////////////// "ZOOMDOWN"
	/////////////////////////////////////////////////////////// "ZOOMUP"
        case ZOOM_UP:  // key
        case ZOOM_DOWN:  // key
	    switch (type) {
	      case MIDI_KEY:
		new = rx->zoom+(action==ZOOM_UP?1:-1);
	        if(new>=1 && new<=8) {
		  receiver_change_zoom(rx,new);
		  update_vfo(rx);
		}
		break;
	      case MIDI_WHEEL:
		new = rx->zoom+(action==ZOOM_UP?1:-1);
	        if(new>=1 && new<=8) {
		  receiver_change_zoom(rx,new);
		  update_vfo(rx);
		}
		break;
	      default:
		// do nothing
		// we should not come here anyway
		break;
	    }
            break;
	case ACTION_NONE:
	    // No error message, this is the "official" action for un-used controller buttons.
	    break;
	default:
	    // This means we have forgotten to implement an action, so we inform on stderr.
	    fprintf(stderr,"Unimplemented MIDI action: A=%d\n", (int) action);
    }
    g_free(a);
    return 0;
}

void DoTheMidi(enum MIDIaction action, enum MIDItype type, int val) {

    //
    // Handle cases in alphabetical order of the key words in midi.props
    //
    switch (action) {
       case CWRIGHT: // CW straight key
          #ifdef CWDAEMON
          // CWdaemon must be running to produce the sidetone
          if (radio->cwdaemon_running == TRUE) {
            if (val) {
              keysidetone = 1;
              g_mutex_lock(&cwdaemon_mutex);
              keytx = true;
              g_mutex_unlock(&cwdaemon_mutex);
            }
            else {
              keysidetone = 0;
              g_mutex_lock(&cwdaemon_mutex);
              keytx = false;
              g_mutex_unlock(&cwdaemon_mutex);
            }
            cw_notify_straight_key_event(keysidetone);
          }
          #endif
          break;
        // TODO: add dit dah and use unixcw built in iambic keyer
        default:
          // all other actions are performed using g_idle_add
          {
    if(midi_debug) g_print("%s: action=%d type=%d val=%d\n",__FUNCTION__,action,type,val);
          ACTION *a=g_new(ACTION,1);
          a->action=action;
          a->type=type;
          a->val=val;
          g_idle_add(midi_action,a);
          }
          break;
    }
}


