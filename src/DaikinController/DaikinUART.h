/*
  DaikinUART - Daikin HVAC communication library via Serial Port (S21, X50A)
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

// #ifndef S21_H
// #define S21_H
#pragma once

#include <Arduino.h>
#include <vector>
#include "esp32-hal-log.h"
#include "logger.h"

#define S21_BAUD_RATE 2400
#define S21_SERIAL_CONFIG SERIAL_8E2

#define X50_BAUD_RATE 9600
#define X50_SERIAL_CONFIG SERIAL_8E1

#define PROTOCOL_UNKNOWN 0
#define PROTOCOL_S21 1
#define PROTOCOL_X50 2

// Packet structure
#define S21_STX_OFFSET     0
#define S21_CMD1_OFFSET    1
#define S21_CMD2_OFFSET    2
#define S21_PAYLOAD_OFFSET 3
// A typical payload length, but there are deviations
#define S21_PAYLOAD_LEN    4
// A minimum length of a packet (with no payload)
#define S21_MIN_PKT_LEN 5

#define STX 2
#define ETX 3
#define ACK 6
#define NAK 21

#define SERIAL_TIMEOUT 250


struct ACResponse
{
  char cmd1;
  char cmd2;
  uint8_t data[256];
  uint8_t dataSize;

};

enum
{
  S21_BAD,
   S21_OK,
   S21_NAK,
   S21_NOACK,
   S21_WAIT,
};

const String S21queryCmds[] = {
  "F1", 
  "F4", 
  "F5", //Error code
  // "F9",
  "RH", 
  "RI", 
  "Ra", 
  "RL", 
  "Rd",
  "RG", //Quiet Fan
  "FM"  //Energy Meter
  };

const String S21setCmds[] = {
  "D1", 
  "D5"
  };

const uint8_t X50queryCmds[] = {0xCA, 0xCB, 0xBD, 0xBE,0xB7}; 
// const uint8_t X50queryCmds[] = {0xCA, 0xBD, 0xBE}; 
//0xCA = Main status (power,mode,fan) /
//0xCB = ??
//0xBD = FCU Temperature sensors, /
//0xB7 = CDU Temperature sensors/Compressor Frequency,
//0xBE =  FAN RPM, VANE?
//0xBA = Model Number

class DaikinUART
{
public:
  void setSerial(HardwareSerial *hardwareSerial);
  bool setup();
  void update();
  bool sendCommandX50(uint8_t cmd, uint8_t *payload, uint8_t payloadLen, bool waitResponse = true);
  bool sendCommandS21(uint8_t cmd1, uint8_t cmd2);
  bool sendCommandS21(uint8_t cmd1, uint8_t cmd_2, uint8_t *payload, uint8_t payloadLen, bool waitResponse = true);
  ACResponse getResponse();
  bool isConnected(){return this->connected;};
  uint8_t currentProtocol(){return this->protocol;};

private:

  HardwareSerial *_serial;
  bool connected = false;
  uint8_t protocol = PROTOCOL_UNKNOWN;
  ACResponse lastResponse;
  
  bool testX50Protocol();
  bool testS21Protocol();

  bool isS21SetCmd(uint8_t cmd1, uint8_t cmd2);

  uint8_t S21Checksum(uint8_t *bytes, uint8_t len);
  uint8_t X50Checksum(uint8_t *bytes, uint8_t len);
  bool checkResponseX50(uint8_t cmd, uint8_t *buff, uint8_t size);
  int checkResponseS21(uint8_t cmd1, uint8_t cmd2, uint8_t *buff, uint8_t size);
  bool checkX50ready();



};

// #endif