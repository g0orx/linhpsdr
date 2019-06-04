#include <gtk/gtk.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "discovered.h"
#include "adc.h"
#include "dac.h"
#include "receiver.h"
#include "transmitter.h"
#include "wideband.h"
#include "radio.h"
#include "main.h"
#include "ext.h"
#include "smartsdr_server.h"

#define DISCOVERY_PORT 4992
#define LISTEN_PORT 50000

static GThread *broadcast_thread_id;
static GThread *listen_thread_id;
static gboolean running;
static char reply[64];

unsigned long process_slice_command() {
  char *token; 
  unsigned long response=0xE2000000;
  token=strtok(NULL," \r\n");
  if(token!=NULL) {
g_print("process_slice_command: token=%s\n",token);
    if(strcmp(token,"t")==0) {
      token=strtok(NULL," \r\n");
      if(token!=NULL) {
g_print("process_slice_command: token=%s\n",token);
        int rx=atoi(token);
        if(radio->receiver[rx]!=NULL) {
          token=strtok(NULL," \r\n");
          if(token!=NULL) {
g_print("process_slice_command: token=%s\n",token);
            long long f=atol(token);
            response=0;
            RX_FREQUENCY *info=g_new0(RX_FREQUENCY,1);
            info->rx=radio->receiver[rx];
            info->frequency=f;
            g_idle_add(ext_set_frequency_a,info);
          }
        }
      }
    }
  }
  return response;
}

unsigned long process_xmit_command() {
  char *token;
  unsigned long response=0xE2000000;
  token=strtok(NULL," \r\n");
  if(token!=NULL) {
g_print("process_xmit_command: token=%s\n",token);
    if(strcmp(token,"0")==0) {
      MOX *m=g_new0(MOX,1);
      m->radio=radio;
      m->state=FALSE;
      g_idle_add(ext_set_mox,(gpointer)m);
      response=0;
    } else if(strcmp(token,"1")==0) {
      MOX *m=g_new0(MOX,1);
      m->radio=radio;
      m->state=TRUE;
      g_idle_add(ext_set_mox,(gpointer)m);
      response=0;
    }
  }
  return response;
}


unsigned long process_command() {
  char *token; 
  unsigned long response=0xE2000000;
  token=strtok(NULL," \r\n");
  if(token!=NULL) {
g_print("process_command: token=%s\n",token);
    if(strcmp(token,"slice")==0) {
     response=process_slice_command();
    } else if(strcmp(token,"xmit")==0) {
     response=process_xmit_command();
    }
  }
  return response;
}

char *parse_input(CLIENT *client,char *command) {
  char *token;
  int seq;
  unsigned long response;

  g_print("parse_input: %s\n",command);
  switch(command[0]) {
    case 'C':
      token=strtok(&command[1],"|\r\n");
      response=process_command();
      break;
    default:
      response=0xE2000000;
      break;
  }
  sprintf(reply,"R%d|%08lX",seq,response);
  return reply;
}

static void *client_thread(void *arg) {
  CLIENT *client=(CLIENT *)arg;
  char command[256];
  int bytes_read;
  char* response;

g_print("Client connected on port %d\n",client->address.sin_port);
  while(running) {
    bytes_read=recv(client->socket,command,sizeof(command),0);
    if(bytes_read<=0) {
      break;
    }
    command[bytes_read]=0;
    response=parse_input(client,command);
    send(client->socket,response,strlen(response),0);
  }
  return NULL;
}

static void *listen_thread(void *arg) {
  int s;
  int address_length;
  struct sockaddr_in address;
  CLIENT* client;
  int rc;

  // create TCP socket to listen on
  s=socket(AF_INET,SOCK_STREAM,0);
  if(s<0) {
    g_print("listen_thread: socket failed\n");
    return NULL;
  }

  // bind to listening port
  memset(&address,0,sizeof(address));
  address.sin_family=AF_INET;
  address.sin_addr.s_addr=INADDR_ANY;
  address.sin_port=htons(LISTEN_PORT);
  if(bind(s,(struct sockaddr*)&address,sizeof(address))<0) {
    g_print("listen_thread: bind failed\n");
    return NULL;
  }

  // listen for connections
g_print("Server listening on port %d\n",LISTEN_PORT);
  while(running) {
    if(listen(s,5)<0) {
      g_print("listen_thread: listen failed\n");
      break;
    }
    client=g_new0(CLIENT,1);
    client->address_length=sizeof(client->address);
    if((client->socket=accept(s,(struct sockaddr*)&client->address,&client->address_length))<0) {
      g_print("listen_thread: accept failed\n");
      break;
    }

    client->thread_id=g_thread_new("SSDR_client",client_thread,client);
  }
  return NULL;
}

static void *broadcast_thread(void *arg) {
  struct sockaddr_in address;
  VITA_EXT_DATA_DISCOVERY *message=g_new0(VITA_EXT_DATA_DISCOVERY,1);
  struct ifreq ifr;

  memset(&address,0,sizeof(address));
  address.sin_family=AF_INET;
  address.sin_addr.s_addr=htonl(INADDR_BROADCAST);
  address.sin_port=htons(DISCOVERY_PORT);

  int s = socket(AF_INET, SOCK_DGRAM, 0);
  int broadcast_enable=1;
  int ret=setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
  if(bind(s,(struct sockaddr*)&address,sizeof(address))<0) {
    g_print("broadcast_thread: bind failed\n");
    return NULL;
  }
  
  message->header=htonl(VITA_PACKET_TYPE_EXT_DATA_WITH_STREAM_ID |
                        VITA_HEADER_CLASS_ID_PRESENT |
                        VITA_TSI_OTHER |
                        VITA_TSF_SAMPLE_COUNT); 
  message->stream_id=htonl(0x800);
  message->class_id=htonl(0x534CFFFF);
  message->timestamp_int=0;
  message->timestamp_frac_h=0;
  message->timestamp_frac_l=0;
  sprintf(message->payload,"model=%s serial=%s version=%s name=%s callsign=%s ip=0.0.0.0 port=%u",
          "linHPSDR",
          "1234",
          "0.1",
          radio->discovered->name,
          "G0ORX",
          DISCOVERY_PORT);

  while(running) {
    int bytes_sent=sendto(s,message,sizeof(VITA_EXT_DATA_DISCOVERY),0,(struct sockaddr *)&address,sizeof(address));
    sleep(1);
  }
 
  g_free(message);
  return NULL;
}

int create_smartsdr_server() {
  running=TRUE;
  broadcast_thread_id = g_thread_new( "SSDR_bcast", broadcast_thread, NULL);
  listen_thread_id = g_thread_new( "SSDR_listen", listen_thread, NULL);
  return 0;
}
