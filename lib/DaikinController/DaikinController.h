#ifndef DAIKIN_CONTROLLER_H
#define DAIKIN_CONTROLLER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "DaikinUART.h"
#define TAG "DaikinController"

#define STX 2
#define ETX 3
#define ACK 6
#define NAK 21

#define S21_RESPONSE_TIMEOUT 250

#define S21_BAUD_RATE 2400
#define S21_STOP_BITS 2
#define S21_DATA_BITS 8
#define S21_PARITY uart::UART_CONFIG_PARITY_EVEN


enum class DaikinClimateMode : uint8_t {
  Disabled = '0',
  Auto = '1',
  Dry = '2',
  Cool = '3',
  Heat = '4',
  Fan = '6',
};

enum class DaikinFanMode : uint8_t {
  Auto = 'A',
  Speed1 = '3',
  Speed2 = '4',
  Speed3 = '5',
  Speed4 = '6',
  Speed5 = '7',
};


struct HVACSettings {
  const char* power;
  const char* mode;
  float temperature;
  const char* fan;
  const char* verticalVane; //vertical vane, up/down
  const char* horizontalVane; //horizontal vane, left/right
  // bool connected;
};

struct HVACStatus {
  float roomTemperature;
  float outsideTemperature;
  float coilTemperature;
  int fanRPM;
  bool operating; // if true, the heatpump is operating to reach the desired temperature
  int compressorFrequency;
};


class DaikinController{
  

    public:
        DaikinController();
        bool connect(HardwareSerial *serial);
        bool sync();

        //settings
        // HVACSettings getSettings();
        // void setSettings(HVACSettings settings);
        void setPowerSetting(bool setting);
        bool getPowerSettingBool(); 
        const char* getPowerSetting();
        void setPowerSetting(const char* setting);

        const char* getModeSetting();
        void setModeSetting(const char* setting);

        float getTemperature();
        void setTemperature(float setting);

        const char* getFanSpeed();
        void setFanSpeed(const char* setting);

        const char* getVerticalVaneSetting();
        void setVerticalVaneSetting(const char* setting);

        const char* getHorizontalVaneSetting();
        void setHorizontalVaneSetting(const char* setting);

        String daikin_climate_mode_to_string(DaikinClimateMode mode);
        String daikin_fan_mode_to_string(DaikinFanMode mode);

        // status
        HVACStatus getStatus();
        float getRoomTemperature();
        bool isConnected(){return daikinUART->isConnected();};
        // bool is_power_on() { return this->power_on; }
        bool setBasic(HVACSettings newSetting);
        bool readState();
        bool update();

    private:

    struct PendingSettings{
      bool basic ;
      bool vane;
    };

        HardwareSerial * _serial {nullptr};
        HVACStatus currentStatus {0,0,0,0,0,0};
        HVACSettings currentSettings {"OFF","COOL",25.0,"AUTO","HOLD","HOLD"};

        //Temporary setting value.
        PendingSettings pendingSettings = {false,false};
        HVACSettings newSettings {"OFF","COOL",25.0,"AUTO","HOLD","HOLD"}; //Mock data

        bool connected = false;
        DaikinUART *daikinUART {nullptr};
        bool parseResponse(ACResponse *response);

        // bool power_on = false;
        // DaikinClimateMode mode = DaikinClimateMode::Disabled;
        // DaikinFanMode fan = DaikinFanMode::Auto;
        // int16_t setpoint = 23;
        // bool swing_v = false;
        // bool swing_h = false;
        // int16_t temp_inside = 0;
        // int16_t temp_outside = 0;
        // int16_t temp_coil = 0;
        // uint16_t fan_rpm = 0;
        // bool idle = true;
        // bool ready = false;
        
        const char* lookupByteMapValue(const char* valuesMap[], const byte byteMap[], int len, byte byteValue);
        int  lookupByteMapIndex(const char* valuesMap[], int len, const char* lookupValue);
        int  lookupByteMapIndex(const int valuesMap[], int len, int lookupValue);


        // DaikinClimateMode get_climate_mode() { return this->mode; }
        // DaikinFanMode get_fan_mode() { return this->fan; }
        // float get_setpoint() { return this->setpoint / 10.0; }
    
};


#endif