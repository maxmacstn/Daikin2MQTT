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

String getHEXformatted(uint8_t *bytes, size_t len);

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

// Query command indices — must match position in S21queryCmds[] array.
// Used with s21SkipMask bitmask for capability detection and entity gating.
// If the array is reordered, these defines must be updated to match.
#define S21_QUERY_F2 1
#define S21_QUERY_F6 5
#define S21_QUERY_F8 7
#define S21_QUERY_FC 11
#define S21_QUERY_RD 19
#define S21_QUERY_RE 20
#define S21_QUERY_RN 26
#define S21_QUERY_RX 27
#define S21_QUERY_FM 31

// S21 query commands — probed sequentially on first sync cycle.
// NAK'd commands are skip-masked and never retried (until full reconnect).
// One-shot commands (F2, F8, FC) self-skip after first successful read.
// Unknown commands are kept for forward compatibility with newer Daikin units.
// Tested on FTKQ v0: 19 of 32 supported, 13 NAK'd.
const char* const S21queryCmds[] = {
  "F1",         // 0  - Basic state (power, mode, temp, fan)
  "F2",         // 1  - Capabilities (swing support)
  "F3",         // 2  - Timer + Powerful (fallback)
  "F4",         // 3  - Error code
  "F5",         // 4  - Swing state
  "F6",         // 5  - Powerful, comfort, quiet, streamer
  "F7",         // 6  - Demand control + econo mode [PROBE]
  "F8",         // 7  - Protocol version [PROBE]
  "F9",         // 8  - Coarse indoor/outdoor temperature
  "FA",         // 9  - Unknown (v2+)
  "FB",         // 10 - Unknown (v2+)
  "FC",         // 11 - Model identification
  "FG",         // 12 - Unknown (v2+)
  "FK",         // 13 - Unknown (v2+)
  "FN",         // 14 - Unknown (v2+)
  "FP",         // 15 - Unknown (v2+)
  "FQ",         // 16 - Unknown (v2+)
  "FS",         // 17 - Unknown (v2+)
  "FT",         // 18 - Unknown (v2+)
  "RD",         // 19 - ON timer setting (10-min periods)
  "RE",         // 20 - OFF timer setting (10-min periods) [PROBE]
  "RG",         // 21 - Fan speed (alternative)
  "RH",         // 22 - Inside temperature (high res)
  "RI",         // 23 - Coil temperature
  "RL",         // 24 - Fan RPM
  "RM",         // 25 - Target louver angle (degrees)
  "RN",         // 26 - Measured louver angle (degrees)
  "RX",         // 27 - Real target temperature (adjusted setpoint)
  "Ra",         // 28 - Outside temperature (high res)
  "Rd",         // 29 - Compressor frequency
  "Re",         // 30 - Humidity
  "FM"          // 31 - Energy Meter
  };

const char* const S21setCmds[] = {
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
  bool lastResultWasNAK(){return this->lastNAK;};

private:

  HardwareSerial *_serial;
  bool connected = false;
  bool lastNAK = false;
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