/*
  Daikin2mqtt - Daikin Heat Pump to MQTT control for Home Assistant.
  Copyright (c) 2024 - MaxMacSTN
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "logger.h"


Logging &Logging::getInstance() {
  static Logging instance ;
  return instance   ;
}

void Logging:: storeLog(char *log, size_t size){
    // return;

    if (size > 512)
        size = 512;

    // Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    // Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
    // delay(100);
    // Copy log to historical buffer
    // Serial.printf("logbuffsize %d buff0 =  %c size = %d\n", logBuffSize, buff[0], size);
    // if (logBuffPTR == nullptr){
    //     logBuffPTR = (char*) ps_malloc(sizeof(char) * LOG_SIZE);
    // }
    // Serial.printf("logbuffptr %p\n", logBuffPTR);
    memcpy(logBuffPTR + logBuffSize , log, sizeof(char) * size );

    logBuffSize += size;
    

    //Rotate log if buffer full
    if (logBuffSize + 512 >= LOG_SIZE){
        // Serial.println("rotate buff");
        memmove(logBuffPTR, &logBuffPTR[512], logBuffSize);
        // memmove(logBuffPTR, logBuffPTR +512, logBuffSize);
        logBuffSize -= 512;
    }

    // Serial.printf("Buff size %d\n", logBuffSize);
    // Serial.println("---Logbuff----");
    // for (int i = 0; i < logBuffSize; i++){
    //     Serial.print((logBuffPTR)[i]);
    // }
    // Serial.println("--------------");
}


void Logging:: f(const char* tag, char *format, ...){
    va_list args;
    char buff[512];
    int size = 0 ;

    va_start(args, format);
    size += sprintf(buff, "[%s:%08d]\t",tag, millis()/1000);
    size += vsnprintf(&buff[size], 512, format, args);

    //log to serial (USB CDC)
    for (int i =0 ;  i < size; i++){
        Serial.print(buff[i]);
    }

    this->storeLog(buff, size);
}

void Logging::  f(const char* tag, const char *format, ...){
    this->f(tag, (char*) format);
}

void Logging:: f(const char* tag,  String string){
    this->f(tag, string.c_str());
}

void Logging::  ln(const char* tag, String string){
    string += "\n";
    this->f(tag, string.c_str());
}

void Logging:: ln(const char* tag, const char *format, ...){

    va_list args;
    char buff[512];
    int size = 0 ;

    va_start(args, format);
    size += sprintf(buff, "[%s:%08d]\t",tag, millis()/1000);
    size += vsnprintf(&buff[size], 512, format, args);
    buff[size] = '\n';
    size++;

    //log to serial (USB CDC)
    for (int i =0 ;  i < size; i++){
        Serial.print(buff[i])  ;     
    }

    this->storeLog(buff, size);
}

String Logging::getLogs(){
    String logs = "";

    for (int i = 0; i < logBuffSize; i++){
        logs += (logBuffPTR)[i];
    }
    return logs;
}

Logging &Log = Log.getInstance();
