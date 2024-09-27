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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "property.h"
#include "debug.h"

PROPERTY* properties;

static double version=0.0;

void initProperties() {
  PROPERTY *next;
  while(properties!=NULL) {
    next=properties->next_property;
    free(properties);
    properties=next;
  }
}

/* --------------------------------------------------------------------------*/
/**
* @brief Load Properties
*
* @param filename
*/
void loadProperties(char* filename) {
    char string[256];
    char* name;
    char* value;
    FILE* f=fopen(filename,"r");
    properties=NULL;
    PROPERTY* property;

    if (DEBUG>0)
        dbgprintf("loadProperties: %s\n", filename);
    
    if(f) {
        while(fgets(string,sizeof(string),f)) {
            if(string[0]!='#') { // content is not comment line
                if(DEBUG>1) // else NOOP
                    dbgprintf("loading property:: %s", string); // lines from filename have '\n' already
                name=strtok(string,"=");
                value=strtok(NULL,"\n");
                if (name != NULL) {
                    // strtok() indicates 'no token found' by returning NULL value.
                    if (value == 0x0) // property is empty string
                        value = "";   // set value accordingly

                    property=malloc(sizeof(PROPERTY));
                    property->name=malloc(strlen(name)+1);
                    strcpy(property->name,name);
                    property->value=malloc(strlen(value)+1);
                    strcpy(property->value,value);
                    property->next_property=properties;
                    properties=property;
                    if(strcmp(name,"property_version")==0) {
                        version=atof(value);
                    }
                }
            }
        }
        fclose(f);
    }

    if(version!=PROPERTY_VERSION) {
        properties=NULL;
        if (DEBUG>0)
            dbgprintf("loadProperties: version=%f expected version=%f - ignoring\n",version,PROPERTY_VERSION);
    }
}

/* --------------------------------------------------------------------------*/
/**
* @brief Save Properties
*
* @param filename
*/
void saveProperties(char* filename) {
    PROPERTY* property;
    FILE* f=fopen(filename,"w+");
    char line[512];
    if(!f) {
        fprintf(stderr,"can't open %s\n",filename);
        return;
    }

    sprintf(line,"%0.2f",PROPERTY_VERSION);
    setProperty("property_version",line);
    property=properties;
    while(property) {
        sprintf(line,"%s=%s\n",property->name,property->value);
        fwrite(line,1,strlen(line),f);
        property=property->next_property;
    }
    fclose(f);
}

/* --------------------------------------------------------------------------*/
/**
* @brief Get Properties
*
* @param name
*
* @return
*/
char* getProperty(char* name) {
    char* value=NULL;
    PROPERTY* property=properties;
    while(property) {
        if(strcmp(name,property->name)==0) {
            value=property->value;
            break;
        }
        property=property->next_property;
    }
    return value;
}

/* --------------------------------------------------------------------------*/
/**
* @brief Set Properties
*
* @param name
* @param value
*/
void setProperty(char* name,char* value) {
    PROPERTY* property=properties;
    while(property) {
        if(strcmp(name,property->name)==0) {
            break;
        }
        property=property->next_property;
    }
    if(property) {
        // just update
        free(property->value);
        property->value=malloc(strlen(value)+1);
        strcpy(property->value,value);
    } else {
        // new property
        property=malloc(sizeof(PROPERTY));
        property->name=malloc(strlen(name)+1);
        strcpy(property->name,name);
        property->value=malloc(strlen(value)+1);
        strcpy(property->value,value);
        property->next_property=properties;
        properties=property;
    }
}

