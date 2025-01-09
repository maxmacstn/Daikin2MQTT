/*
  DaikinController - Daikin HVAC communication library via Serial Port (S21, X50A)
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
#include <HardwareSerial.h>
#include "DaikinUART.h"
#include "logger.h"
 

#define STX 2
#define ETX 3
#define ACK 6
#define NAK 21

#define S21_RESPONSE_TIMEOUT 250

#define SYNC_INTEVAL 10000

#define S21_BAUD_RATE 2400
#define S21_STOP_BITS 2
#define S21_DATA_BITS 8
#define S21_PARITY uart::UART_CONFIG_PARITY_EVEN

enum class DaikinClimateMode : uint8_t
{
  Disabled = '0',
  Auto = '1',
  Dry = '2',
  Cool = '3',
  Heat = '4',
  Fan = '6',
};

enum class DaikinFanMode : uint8_t
{
  Auto = 'A',
  Speed1 = '3',
  Speed2 = '4',
  Speed3 = '5',
  Speed4 = '6',
  Speed5 = '7',
  Quiet = 'B'
};

struct HVACSettings
{
  const char *power;
  const char *mode;
  float temperature;
  const char *fan;
  const char *verticalVane;   // vertical vane, up/down
  const char *horizontalVane; // horizontal vane, left/right
  // bool connected;
};

struct HVACStatus
{
  float roomTemperature;
  float outsideTemperature;
  float coilTemperature;
  float energyMeter;
  int fanRPM;
  bool operating; // if true, the heatpump is operating to reach the desired temperature
  int compressorFrequency;
  String modelName;
  String errorCode;
};

const char X50errorCodeDivision[] = { ' ', 'A', 'C', 'E', 'H', 'F', 'J', 'L', 'P', 'U', 'M', '6', '8', '9', ' ',' '};
const char X50errorCodeDetail[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'H', 'C', 'J', 'E', 'F'};

const char S21errorCodeDivision[] = {' ', ' ', ' ', 'A', 'C', 'E', 'H', 'F', 'J', 'L', 'P', 'U', 'M', '6', '8', '9'};
const char S21errorCodeDetail[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'H', 'C', 'J', 'E', 'F'};

#define SETTINGS_CHANGED_CALLBACK_SIGNATURE std::function<void()> settingsChangedCallback
#define STATUS_CHANGED_CALLBACK_SIGNATURE std::function<void(HVACStatus newStatus)> statusChangedCallback

class DaikinController
{

public:
  DaikinController();
  bool connect(HardwareSerial *serial);
  bool sync();   // Synchronize AC State to local state
  bool update(bool updateAll = false); // Update local settings to AC.

  DaikinUART *daikinUART{nullptr};


  // Getters & Setters
  void togglePower();
  void setPowerSetting(bool setting);
  bool getPowerSettingBool();
  const char *getPowerSetting();
  void setPowerSetting(const char *setting);
  const char *getModeSetting();
  void setModeSetting(const char *setting);
  float getTemperature();
  void setTemperature(float setting);
  const char *getFanSpeed();
  void setFanSpeed(const char *setting);
  const char *getVerticalVaneSetting();
  void setVerticalVaneSetting(const char *setting);
  const char *getHorizontalVaneSetting();
  void setHorizontalVaneSetting(const char *setting);
  String getModelName();

  // Converter
  String daikin_climate_mode_to_string(DaikinClimateMode mode);
  String daikin_fan_mode_to_string(DaikinFanMode mode);

  // status
  HVACStatus getStatus() { return this->currentStatus; };
  HVACSettings getSettings() { return currentSettings; };
  float getRoomTemperature() { return this->currentStatus.roomTemperature; };
  bool isConnected() { return daikinUART->isConnected(); };
  // bool is_power_on() { return this->power_on; }
  bool setBasic(HVACSettings *newSetting);
  bool readState();


  // Callbacks
  void setSettingsChangedCallback(SETTINGS_CHANGED_CALLBACK_SIGNATURE);
  void setStatusChangedCallback(STATUS_CHANGED_CALLBACK_SIGNATURE);

private:
  struct PendingSettings
  {
    bool basic;
    bool vane;
  };

  HardwareSerial *_serial{nullptr};

  HVACStatus currentStatus{0, 0, 0, 0, 0, 0};
  HVACSettings currentSettings{"OFF", "COOL", 25.0, "AUTO", "HOLD", "HOLD"};
  HVACSettings newSettings{"OFF", "COOL", 25.0, "AUTO", "HOLD", "HOLD"}; // Mock data

  // Temporary setting value.
  PendingSettings pendingSettings = {false, false};

  unsigned long lastSyncMs = 0;
  bool use_RG_fan = false;

  SETTINGS_CHANGED_CALLBACK_SIGNATURE{nullptr};
  STATUS_CHANGED_CALLBACK_SIGNATURE{nullptr};

  bool parseResponse(ACResponse *response);

  const char *lookupByteMapValue(const char *valuesMap[], const byte byteMap[], int len, byte byteValue);
  int lookupByteMapIndex(const char *valuesMap[], int len, const char *lookupValue);
  int lookupByteMapIndex(const int valuesMap[], int len, int lookupValue);
  void onFirstQuerySuccess();

  uint16_t s21_decode_hex_sensor (const unsigned char *payload) //Copied from Faikin
  {
  #define hex(c)	(((c)&0xF)+((c)>'9'?9:0))
    return (hex (payload[3]) << 12) | (hex (payload[2]) << 8) | (hex (payload[1]) << 4) | hex (payload[0]);
  #undef hex
  }

};

// #endif