// #ifndef DAIKIN_CONTROLLER_H
// #define DAIKIN_CONTROLLER_H
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

#define SYNC_INTEVAL 5000

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
  int fanRPM;
  bool operating; // if true, the heatpump is operating to reach the desired temperature
  int compressorFrequency;
  String modelName;
};

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

  SETTINGS_CHANGED_CALLBACK_SIGNATURE{nullptr};
  STATUS_CHANGED_CALLBACK_SIGNATURE{nullptr};

  bool parseResponse(ACResponse *response);

  const char *lookupByteMapValue(const char *valuesMap[], const byte byteMap[], int len, byte byteValue);
  int lookupByteMapIndex(const char *valuesMap[], int len, const char *lookupValue);
  int lookupByteMapIndex(const int valuesMap[], int len, int lookupValue);
  void onFirstQuerySuccess();
};

// #endif