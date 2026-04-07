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
 

#define S21_RESPONSE_TIMEOUT 250

#define SYNC_INTEVAL 10000

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
  const char *powerful; 
  bool remoteEnable;
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
  uint8_t timerMode;
  float realTargetTemp;   // RX — adjusted setpoint
  float louverAngle;      // RN — measured louver angle
  int onTimerMinutes;     // RD — ON timer (minutes)
  int offTimerMinutes;    // RE — OFF timer (minutes)
};

const char X50errorCodeDivision[] = { ' ', 'A', 'C', 'E', 'H', 'F', 'J', 'L', 'P', 'U', 'M', '6', '8', '9', ' ',' '};
const char X50errorCodeDetail[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'H', 'C', 'J', 'E', 'F'};

const char S21errorCodeDivision[] = {' ', ' ', ' ', 'A', 'C', 'E', 'H', 'F', 'J', 'L', 'P', 'U', 'M', '6', '8', '9'};
const char S21errorCodeDetail[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'H', 'C', 'J', 'E', 'F'};

#define SETTINGS_CHANGED_CALLBACK_SIGNATURE std::function<void()> settingsChangedCallback
#define STATUS_CHANGED_CALLBACK_SIGNATURE std::function<void(const HVACStatus &newStatus)> statusChangedCallback

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
  const char *getPowerfulSetting();
  void setPowerfulSetting(const char *setting);
  void setEnableRemote(bool enable);
  String getModelName();

  // Converter
  String daikin_climate_mode_to_string(DaikinClimateMode mode);
  String daikin_fan_mode_to_string(DaikinFanMode mode);

  // Status accessors (const ref to avoid struct copies in 10s poll loop)
  const HVACStatus& getStatus() { return this->currentStatus; };
  const HVACSettings& getSettings() { return currentSettings; };
  float getRoomTemperature() { return this->currentStatus.roomTemperature; };
  bool isConnected() { return daikinUART->isConnected(); };

  // Capability detection — checked by haConfig() to conditionally expose HA entities.
  // Most use s21SkipMask: if the S21 query command was NAK'd, the feature isn't supported.
  // Some use runtime detection (compressor freq, outside temp) because the command ACKs
  // but returns meaningless data on certain models (e.g., Rd always returns 000 on v0 units).
  bool supportsPowerful() { return !(s21SkipMask & (1 << S21_QUERY_F6)); };
  bool supportsVerticalSwing() { return _supportsVerticalSwing; };  // from F2 capability flags
  bool supportsHorizontalSwing() { return _supportsHorizontalSwing; }; // from F2 capability flags
  bool supportsEnergyMeter() { return !(s21SkipMask & (1 << S21_QUERY_FM)); };
  bool supportsCompressorFreq() { return _compressorFreqSeen; };  // true once Rd returns non-zero
  bool supportsOutsideTemp() { return _outsideTempChanged; };     // true once Ra returns a different value
  bool supportsRealTargetTemp() { return !(s21SkipMask & (1 << S21_QUERY_RX)); };
  bool supportsLouverAngle() { return !(s21SkipMask & (1 << S21_QUERY_RN)); };
  bool supportsOnTimer() { return !(s21SkipMask & (1 << S21_QUERY_RD)); };
  bool supportsOffTimer() { return !(s21SkipMask & (1 << S21_QUERY_RE)); };

  // Runtime rediscovery — when a new capability is detected after initial haConfig(),
  // this flag triggers republishing HA discovery so the new entity appears without reboot.
  bool needsRediscovery() { return _rediscoverNeeded; };
  void clearRediscovery() { _rediscoverNeeded = false; };

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
    bool specialMode;
    bool ACconfig;
    bool hasPending() const { return basic || vane || specialMode || ACconfig; }
  };

  HardwareSerial *_serial{nullptr};

  HVACStatus currentStatus{0, 0, 0, 0, 0, 0};
  HVACSettings currentSettings{"OFF", "COOL", 25.0, "auto", "hold", "hold", "OFF", true};
  HVACSettings newSettings{"OFF", "COOL", 25.0, "auto", "hold", "hold", "OFF", true};

  // Temporary setting value.
  PendingSettings pendingSettings = {false, false, false, false};

  unsigned long lastSyncMs = 0;
  bool use_RG_fan = false;     // true once RG returns valid fan speed (overrides F1 byte 3)

  // S21 command skip mask — one bit per entry in S21queryCmds[].
  // Set when a command returns NAK (unsupported by this unit).
  // Checked in sync() to avoid re-sending unsupported commands.
  // Also used by supportsX() methods to gate HA entity discovery.
  uint32_t s21SkipMask = 0;

  // Hardware capabilities detected from F2 response (one-shot query at first sync).
  // Defaults are true so entities appear if F2 isn't supported (safe fallback).
  bool _supportsVerticalSwing = true;
  bool _supportsHorizontalSwing = true;

  bool _skipRzB2 = false;  // RzB2 is not in the query array, needs its own skip flag

  // Runtime value-change detection for sensors that ACK but return bogus data.
  // Compressor freq: some v0 units always return 000 even when compressor is running.
  // Outside temp: some units return a fixed 25.0°C regardless of actual temperature.
  // These entities are hidden until we see meaningful (changing/non-zero) values,
  // then _rediscoverNeeded triggers haConfig() republish to add the entity to HA.
  bool _compressorFreqSeen = false;
  bool _outsideTempChanged = false;
  float _firstOutsideTemp = -999;
  bool _rediscoverNeeded = false;

  // Copy currentSettings → newSettings, preserving fields S21 doesn't report back.
  void syncNewSettings() {
    bool savedRemoteEnable = newSettings.remoteEnable;
    newSettings = currentSettings;
    newSettings.remoteEnable = savedRemoteEnable;
  }

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