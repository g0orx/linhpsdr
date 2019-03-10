#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SoapySDR/Device.h>
#include <SoapySDR/Formats.h>
#include "discovered.h"
#include "soapy_discovery.h"

static void get_info(char *driver) {
  size_t rx_rates_length, tx_rates_length, gains_length, ranges_length, rx_antennas_length, tx_antennas_length;
  int i;
  SoapySDRKwargs args={};
  int version=0;

  fprintf(stderr,"soapy_discovery: get_info: %s\n", driver);
  SoapySDRKwargs_set(&args, "driver", driver);
  SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
  SoapySDRKwargs_clear(&args);
  version=0;
  SoapySDRKwargs info=SoapySDRDevice_getHardwareInfo(sdr);
  for(i=0;i<info.size;i++) {
    fprintf(stderr,"soapy_discovery: hardware info key=%s val=%s\n",info.keys[i], info.vals[i]);
    if(strcmp(info.keys[i],"firmwareVersion")==0) {
      version+=atoi(info.vals[i])*100;
    }
    if(strcmp(info.keys[i],"hardwareVersion")==0) {
      version+=atoi(info.vals[i])*10;
    }
    if(strcmp(info.keys[i],"protocolVersion")==0) {
      version+=atoi(info.vals[i]);
    }
  }

  SoapySDRRange *rx_rates=SoapySDRDevice_getSampleRateRange(sdr, SOAPY_SDR_RX, 0, &rx_rates_length);
  fprintf(stderr,"Rx sample rates: ");
  for (size_t i = 0; i < rx_rates_length; i++) {
    fprintf(stderr,"%f -> %f (%f),", rx_rates[i].minimum, rx_rates[i].maximum, rx_rates[i].minimum/48000.0);
  }
  fprintf(stderr,"\n");
  free(rx_rates);

  SoapySDRRange *tx_rates=SoapySDRDevice_getSampleRateRange(sdr, SOAPY_SDR_TX, 0, &tx_rates_length);
  fprintf(stderr,"Tx sample rates: ");
  for (size_t i = 0; i < tx_rates_length; i++) {
    fprintf(stderr,"%f -> %f (%f),", tx_rates[i].minimum, tx_rates[i].maximum, tx_rates[i].minimum/48000.0);
  }
  fprintf(stderr,"\n");
  free(tx_rates);

  SoapySDRRange *ranges = SoapySDRDevice_getFrequencyRange(sdr, SOAPY_SDR_RX, 0, &ranges_length);
  fprintf(stderr,"Rx freq ranges: ");
  for (size_t i = 0; i < ranges_length; i++) fprintf(stderr,"[%f Hz -> %f Hz step=%f], ", ranges[i].minimum, ranges[i].maximum,ranges[i].step);
  fprintf(stderr,"\n");

  char** rx_antennas = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_RX, 0, &rx_antennas_length);
  fprintf(stderr, "Rx antennas: ");
  for (size_t i = 0; i < rx_antennas_length; i++) fprintf(stderr, "%s, ", rx_antennas[i]);
  fprintf(stderr,"\n");

  char** tx_antennas = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_TX, 0, &tx_antennas_length);
  fprintf(stderr, "Tx antennas: ");
  for (size_t i = 0; i < tx_antennas_length; i++) fprintf(stderr, "%s, ", tx_antennas[i]);
  fprintf(stderr,"\n");

  char **gains = SoapySDRDevice_listGains(sdr, SOAPY_SDR_RX, 0, &gains_length);
  fprintf(stderr,"Rx gains: \n");

  gboolean has_automatic_gain=SoapySDRDevice_hasGainMode(sdr, SOAPY_SDR_RX, 0);
  fprintf(stderr,"has_automaic_gain=%d\n",has_automatic_gain);

  gboolean has_automatic_dc_offset_correction=SoapySDRDevice_hasDCOffsetMode(sdr, SOAPY_SDR_RX, 0);
  fprintf(stderr,"has_automaic_dc_offset_correction=%d\n",has_automatic_dc_offset_correction);

  if(devices<MAX_DEVICES) {
    discovered[devices].device=DEVICE_SOAPYSDR_USB;
    discovered[devices].protocol=PROTOCOL_SOAPYSDR;
    strcpy(discovered[devices].name,driver);
    discovered[devices].supported_receivers=1;
    discovered[devices].adcs=1;
    discovered[devices].status=STATE_AVAILABLE;
    discovered[devices].software_version=version;
    discovered[devices].frequency_min=ranges[0].minimum;
    discovered[devices].frequency_max=ranges[0].maximum;
    discovered[devices].info.soapy.gains=gains_length;
    discovered[devices].info.soapy.gain=gains;
    discovered[devices].info.soapy.range=malloc(gains_length*sizeof(SoapySDRRange));
    for (size_t i = 0; i < gains_length; i++) {
      fprintf(stderr,"%s ", gains[i]);
      SoapySDRRange range=SoapySDRDevice_getGainElementRange(sdr, SOAPY_SDR_RX, 0, gains[i]);
      fprintf(stderr,"%f -> %f step=%f\n",range.minimum,range.maximum,range.step);
      discovered[devices].info.soapy.range[i]=range;
    }
    discovered[devices].info.soapy.has_automatic_gain=has_automatic_gain;
    discovered[devices].info.soapy.has_automatic_dc_offset_correction=has_automatic_dc_offset_correction;
    discovered[devices].info.soapy.antennas=rx_antennas_length;
    discovered[devices].info.soapy.antenna=rx_antennas;
    devices++;
  }

  free(ranges);

}

void soapy_discovery() {
  size_t length;
  int i,j;
  SoapySDRKwargs args={};

  SoapySDRKwargs *results = SoapySDRDevice_enumerate(NULL, &length);
  for (i = 0; i < length; i++) {
    for (size_t j = 0; j < results[i].size; j++) {
      if(strcmp(results[i].keys[j],"driver")==0 && strcmp(results[i].vals[j],"audio")!=0) {
        get_info(results[i].vals[j]);
      }
    }
  }
  SoapySDRKwargsList_clear(results, length);

#ifdef INCLUDED
  SoapySDRKwargs_set(&args, "driver", "lime");
  SoapySDRDevice *sdr = SoapySDRDevice_make(&args);
  SoapySDRKwargs_clear(&args);

  if(sdr==NULL) {
    fprintf(stderr, "lime_discovery: no devices found\n");
    return;
  }

  SoapySDRKwargs info=SoapySDRDevice_getHardwareInfo(sdr);
  int version=0;
  for(i=0;i<info.size;i++) {
fprintf(stderr,"lime_discovery: info key=%s val=%s\n",info.keys[i], info.vals[i]);
    if(strcmp(info.keys[i],"firmwareVersion")==0) {
      version+=atoi(info.vals[i])*100;
    }
    if(strcmp(info.keys[i],"hardwareVersion")==0) {
      version+=atoi(info.vals[i])*10;
    }
    if(strcmp(info.keys[i],"protocolVersion")==0) {
      version+=atoi(info.vals[i]);
    }
  }

  if(devices<MAX_DEVICES) {
    discovered[devices].device=DEVICE_LIMESDR_USB;
    discovered[devices].protocol=PROTOCOL_LIMESDR;
    strcpy(discovered[devices].name,"SoapySDR");
    discovered[devices].supported_receivers=2;
    discovered[devices].adcs=2;
    discovered[devices].status=STATE_AVAILABLE;
    discovered[devices].software_version=version;
    devices++;
  }

  //query device info
  char** names = SoapySDRDevice_listAntennas(sdr, SOAPY_SDR_RX, 0, &length);
  fprintf(stderr, "Rx antennas: ");
  for (size_t i = 0; i < length; i++) fprintf(stderr, "%s, ", names[i]);
  fprintf(stderr,"\n");
  SoapySDRStrings_clear(&names, length);

  names = SoapySDRDevice_listGains(sdr, SOAPY_SDR_RX, 0, &length);
  fprintf(stderr,"Rx gains: ");
  for (size_t i = 0; i < length; i++) fprintf(stderr,"%s, ", names[i]);
  fprintf(stderr,"\n");
  SoapySDRStrings_clear(&names, length);

  SoapySDRRange *ranges = SoapySDRDevice_getFrequencyRange(sdr, SOAPY_SDR_RX, 0, &length);
  fprintf(stderr,"Rx freq ranges: ");
  for (size_t i = 0; i < length; i++) fprintf(stderr,"[%f Hz -> %f Hz], ", ranges[i].minimum, ranges[i].maximum);
  fprintf(stderr,"\n");
  free(ranges);


  SoapySDRRange *rates=SoapySDRDevice_getSampleRateRange(sdr, SOAPY_SDR_RX, 0, &length);
  fprintf(stderr,"Rx sample rates: ");
  for (size_t i = 0; i < length; i++) fprintf(stderr,"%f -> %f,", rates[i].minimum, rates[i].maximum);
  fprintf(stderr,"\n");
  free(ranges);

  fprintf(stderr,"lime_discovery found %ld devices\n",length);
#endif
}
