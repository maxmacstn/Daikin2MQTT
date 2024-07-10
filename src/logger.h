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


#pragma once


#include <Arduino.h>

#define LOG_SIZE 50000 

class Logging{

    private:
        Logging() = default;
        // char logBuff[LOG_SIZE];
        // char *logBuffPTR = NULL;
        char logBuffPTR [LOG_SIZE];
        unsigned int logBuffSize = 0;
    public:
        static Logging &getInstance();
        Logging(const Logging &) = delete; // no copying
        Logging &operator=(const Logging &) = delete;

        void storeLog(char *log, size_t size);
        void f(const char* tag, char *format, ...);
        void f(const char* tag, const char *format, ...);
        void f(const char* tag,  String string);
        void ln(const char* tag, String string);
        void ln(const char* tag, const char *format, ...);
        String getLogs();

};

extern Logging &Log;