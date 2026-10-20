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

#include "DaikinController.h"

#define TAG "DKCtrl"

const byte S21_POWER[2] = {'0', '1'};         // Byte value
const char *S21_POWER_MAP[2] = {"OFF", "ON"}; // Actual value

const byte X50_POWER[2] = {0, 1};             // Byte value
const char *X50_POWER_MAP[2] = {"OFF", "ON"}; // Actual value

const byte S21_MODE[7] = {'0', '1', '2', '3', '4', '6'};
const char *S21_MODE_MAP[7] = {"DISABLED", "AUTO", "DRY", "COOL", "HEAT", "FAN"};

const byte X50_MODE[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const char *X50_MODE_MAP[8] = {"FAN", "HEAT", "COOL", "AUTO", "4", "5", "6", "DRY"};
//{"fan_only", "heat", "cool", "auto", "4", "5", "6", "dry"}

const byte S21_FAN[7] = {'A', 'B', '3', '4', '5', '6', '7'};
const char *S21_FAN_MAP[7] = {"AUTO", "QUIET", "1", "2", "3", "4", "5"};

// const byte X50_FAN[7] = {0, 1, 2, 3, 4, 5, 6};
const byte X50_FAN[7] = {0, 1, 2, 3, 4, 5, 6};
const char *X50_FAN_MAP[7] = {"AUTO", "1", "2", "3", "4", "5", "6"};

const byte X50_VERTICALVANE[6] = {0, 1, 2, 3, 4, 7};
const char *X50_VERTICALVANE_MAP[6] = {"0", "1", "2", "3", "4", "SWING"};
// {"auto", "1", "2", "3", "4", "5", "night"}

const byte VERTICALVANE[2] = {'0', '1'};
const char *VERTICALVANE_MAP[2] = {"HOLD", "SWING"};

const byte HORIZONTALVANE[2] = {'0', '1'};
const char *HORIZONTALVANE_MAP[2] = {"HOLD", "SWING"};


const byte S21_POWERFUL[2] = {0x00, 0x02};
const char *S21_POWERFUL_MAP[2] = {"OFF", "ON"};

int16_t bytes_to_num(uint8_t *bytes, size_t len)
{
  // <ones><tens><hundreds><neg/pos>
  int16_t val = 0;
  val = bytes[0] - '0';
  val += (bytes[1] - '0') * 10;
  val += (bytes[2] - '0') * 100;
  if (len > 3 && bytes[3] == '-')
    val *= -1;
  return val;
}

int16_t temp_bytes_to_c10(uint8_t *bytes) { return bytes_to_num(bytes, 4); }

int16_t temp_bytes_to_c10(std::vector<uint8_t> &bytes)
{
  return temp_bytes_to_c10(&bytes[0]);
}

uint8_t c10_to_setpoint_byte(int16_t setpoint)
{
  return (setpoint + 3) / 5 + 28;
}

inline float c10_c(int16_t c10) { return c10 / 10.0; }
inline float c10_f(int16_t c10) { return c10_c(c10) * 1.8 + 32.0; }

//-------------- DaikinController Class ----------------------

DaikinController::DaikinController()
{
  daikinUART = new DaikinUART();
}

bool DaikinController::connect(HardwareSerial *serial)
{
  this->_serial = serial;
  daikinUART->setSerial(serial);
  bool res = daikinUART->setup();
  if (res)
  {
    sync(); // get initial data
  }
  return res;
}

String DaikinController::daikin_climate_mode_to_string(DaikinClimateMode mode)
{
  switch (mode)
  {
  case DaikinClimateMode::Disabled:
    return "Disabled";
  case DaikinClimateMode::Auto:
    return "Auto";
  case DaikinClimateMode::Dry:
    return "Dry";
  case DaikinClimateMode::Cool:
    return "Cool";
  case DaikinClimateMode::Heat:
    return "Heat";
  case DaikinClimateMode::Fan:
    return "Fan";
  default:
    return "UNKNOWN";
  }
}

String DaikinController::daikin_fan_mode_to_string(DaikinFanMode mode)
{
  switch (mode)
  {
  case DaikinFanMode::Auto:
    return "Auto";
  case DaikinFanMode::Speed1:
    return "1";
  case DaikinFanMode::Speed2:
    return "2";
  case DaikinFanMode::Speed3:
    return "3";
  case DaikinFanMode::Speed4:
    return "4";
  case DaikinFanMode::Speed5:
    return "5";
  case DaikinFanMode::Quiet:
    return "Quiet";
  default:
    return "UNKNOWN";
  }
}

void DaikinController::onFirstQuerySuccess()
{

  if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    // Get AC Model Number
    if (daikinUART->sendCommandX50(0xBA, NULL, 0))
    {
      ACResponse response = daikinUART->getResponse();
      parseResponse(&response);
    }
  }
}

bool DaikinController::sync()
{
  // Log.ln(TAG, "Start Sync");
  // delay(100);

  static bool firstSuccess = true;
  bool success = true;
  bool res = false;

  if (millis() - lastSyncMs < SYNC_INTEVAL)
  {
    return false;
  }

  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    uint8_t size = sizeof(S21queryCmds) / sizeof(String);

    for (int i = 0; i < size; i++)
    {

      Log.ln(TAG, String("Send command: " + S21queryCmds[i]));

      res = daikinUART->sendCommandS21(S21queryCmds[i][0], S21queryCmds[i][1]);
      if (res)
      {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      }
      // ("Result: %s\n\n", res ? "Success" : "Failed");
      success = success | res;
    }
  }

  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    Log.ln(TAG, "Query X50");
    delay(100);

    uint8_t cmdSize = sizeof(X50queryCmds) / sizeof(uint8_t);

    // Periodically Query
    for (int i = 0; i < cmdSize; i++)
    {
      uint8_t payload[17] = {0};

      Log.ln(TAG, "Send command: " + String(X50queryCmds[i], HEX));

      switch (X50queryCmds[i])
      {
      case 0xCA:
        res = daikinUART->sendCommandX50(X50queryCmds[i], payload, sizeof(payload));
        break;

      default:
        res = daikinUART->sendCommandX50(X50queryCmds[i], NULL, 0);
        break;
      }

      if (res)
      {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      }
      // ("Result: %s\n\n", res ? "Success" : "Failed");
      success = success & res;
    }

    // Outdoor model: indoor unit fetches it from the outdoor unit, so keep asking until it arrives
    if (this->currentStatus.outdoorModelName.isEmpty() && daikinUART->sendCommandX50(0xB0, NULL, 0))
    {
      ACResponse response = daikinUART->getResponse();
      parseResponse(&response);
    }
  }

  if (success && firstSuccess)
  {
    onFirstQuerySuccess();
    firstSuccess = false;
  }

  lastSyncMs = millis();

  Log.ln(TAG, "End Sync");
  delay(100);
  return success;
}

const char *DaikinController::lookupByteMapValue(const char *valuesMap[], const byte byteMap[], int len, byte byteValue)
{
  for (int i = 0; i < len; i++)
  {
    if (byteMap[i] == byteValue)
    {
      return valuesMap[i];
    }
  }
  return valuesMap[0];
}

int DaikinController::lookupByteMapIndex(const int valuesMap[], int len, int lookupValue)
{
  for (int i = 0; i < len; i++)
  {
    if (valuesMap[i] == lookupValue)
    {
      return i;
    }
  }
  return -1;
}

int DaikinController::lookupByteMapIndex(const char *valuesMap[], int len, const char *lookupValue)
{
  for (int i = 0; i < len; i++)
  {
    if (strcasecmp(valuesMap[i], lookupValue) == 0)
    {
      return i;
    }
  }
  return -1;
}

bool DaikinController::parseResponse(ACResponse *response)
{

  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {
    // incoming packet should be [STX, CMD1+1, CMD2, <DATA>, CRC, ETX]

    uint8_t cmd1_in = response->data[1];
    uint8_t cmd2_in = response->data[2];
    uint8_t payload[256];
    uint8_t payloadSize = response->dataSize - 5;
    memcpy(payload, response->data + 3, payloadSize);

    if (cmd1_in != response->cmd1 + 1)
    {
      // command byte 1 of response packet should be cmd1 + 1
      Serial.println("parseResponse: responded cmd1 does not match request cmd1 ");
      return false;
    }

    switch (cmd1_in)
    {
    case 'G': // F -> G
      switch (cmd2_in)
      {
      case '1': // F1 -> Basic State
        this->currentSettings.power = lookupByteMapValue(S21_POWER_MAP, S21_POWER, 2, payload[0]);
        this->currentSettings.mode = lookupByteMapValue(S21_MODE_MAP, S21_MODE, 7, payload[1]);
        this->currentSettings.temperature = ((payload[2] - 28) * 5) / 10.0;
        if (!use_RG_fan)
        {
          this->currentSettings.fan = lookupByteMapValue(S21_FAN_MAP, S21_FAN, 7, payload[3]);
        }

        if (this->currentSettings.temperature < 10 || this->currentSettings.temperature > 35)
        { // Set default value if HVAC does not have current setpoint Eg. After power outage.
          this->currentSettings.temperature = 25;
        }
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;

      case '4': // F4 -> G4 -- Error code
        {
        bool error = payload[0] & 0x01;

        if (error){
          currentStatus.errorCode = S21errorCodeDivision[payload[1] >> 4];
          currentStatus.errorCode += S21errorCodeDetail[payload[1] & 0xF];
        }else{
          currentStatus.errorCode = "";
        }

        return true;
        }
      case '5': // F5 -> G5 -- Swing state
        this->currentSettings.verticalVane = (payload[0] & 1) ? VERTICALVANE_MAP[1] : VERTICALVANE_MAP[0];
        this->currentSettings.horizontalVane = (payload[0] & 2) ? HORIZONTALVANE_MAP[1] : HORIZONTALVANE_MAP[0];
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;
      
      case '6':
        this->currentSettings.powerful = (payload[0] == '0')? S21_POWERFUL_MAP[0]: S21_POWERFUL_MAP[1];
        return true;


      case '9': // F9 -> G9 -- Temperature
        this->currentStatus.roomTemperature = (float)((signed)payload[0] - 0x80) / 2;
        this->currentStatus.outsideTemperature = (float)((signed)payload[1] - 0x80) / 2;
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;

      case 'M':                                                                  // FM -> GM -- Energy meter
        this->currentStatus.energyMeter = s21_decode_hex_sensor(payload) / 10.0; // kWh
        return true;
      }

      break;
    case 'S': // R -> S
      switch (cmd2_in)
      {
      case 'H': // Inside temperature
        this->currentStatus.roomTemperature = temp_bytes_to_c10(payload) / 10.0;
        return true;
      case 'I': // Coil temperature
        this->currentStatus.coilTemperature = temp_bytes_to_c10(payload) / 10.0;
        return true;
      case 'a': // Outside temperature
        this->currentStatus.outsideTemperature = temp_bytes_to_c10(payload) / 10.0;
        return true;
      case 'L': // Fan speed
        this->currentStatus.fanRPM = bytes_to_num(&payload[0], payloadSize) * 10;
        return true;
      case 'e': // Humidity %
        this->currentStatus.humidity = bytes_to_num(&payload[0], payloadSize);
        return true;
      case 'b': // Demand 0-15
        this->currentStatus.demand = bytes_to_num(&payload[0], payloadSize);
        return true;
      case 'd': // Compressor state / frequency? Idle if 0.
        this->currentStatus.operating = !(payload[0] == '0' && payload[1] == '0' && payload[2] == '0');
        this->currentStatus.compressorFrequency = (payload[0] - '0') + (payload[1] - '0') * 10 + (payload[2] - '0') * 100;
        return true;
      case 'G':
        if (payloadSize == 1)
        {
          this->currentSettings.fan = lookupByteMapValue(S21_FAN_MAP, S21_FAN, 7, payload[0]);
          Log.ln(TAG, "New fan speed found " + String(this->currentSettings.fan));
          use_RG_fan = true;
          return true;
        }
        return false;


      default:
        if (payloadSize > 3)
        {
          int8_t temp = temp_bytes_to_c10(payload);
          Serial.println("Unknown temp: ");
        }
        return false;
      }
    }

    Serial.println("Unknown response ");
    return false;
  }
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    uint8_t cmd = response->cmd1;
    uint8_t payloadSize = response->dataSize;
    uint8_t payload[256];
    memcpy(payload, response->data, payloadSize);   //5th byte

    // HVAC Name (BA indoor, B0 outdoor)
    if ((cmd == 0xBA || cmd == 0xB0) && payloadSize >= 20)
    {
      char model[256];
      uint8_t modelLen = 0;
      for (int i = 0; i < payloadSize; i++)
      {
        if (isalnum(payload[i]))
        {
          model[i] = payload[i];
          modelLen++;
        }
        else
        {
          break;
        }
      }
      if (cmd == 0xB0)
        this->currentStatus.outdoorModelName = String(model, modelLen);
      else
        this->currentStatus.modelName = String(model, modelLen);
      return true;
    }
    // Main Status
    if (cmd == 0xCA && payloadSize >= 7)
    {
      this->currentSettings.power = lookupByteMapValue(X50_POWER_MAP, X50_POWER, 2, payload[0]);
      this->currentSettings.mode = lookupByteMapValue(X50_MODE_MAP, X50_MODE, 8, payload[1]);
      this->currentSettings.fan = lookupByteMapValue(X50_FAN_MAP, X50_FAN, 7, (payload[6] >> 4) & 7);
      // Log.ln(TAG, "Fan ENUM = " + String((payload[6] >> 4) & 7));
      if (this->currentSettings.temperature < 10 || this->currentSettings.temperature > 35)
      { // Set default value if HVAC does not have current setpoint Eg. After power outage.
        this->currentSettings.temperature = 25;
      }

      // Has an error
      if (payload[14]){
          this->currentStatus.errorCode = "";
          this->currentStatus.errorCode += X50errorCodeDivision[payload[12] >> 4];
          this->currentStatus.errorCode += X50errorCodeDetail[payload[12] & 0xF];
          if (payload[13]){
            this->currentStatus.errorCode += "-";
            this->currentStatus.errorCode += String(payload[13] >> 2);
          }
      }else{
          this->currentStatus.errorCode = "";
      }

      this->currentStatus.filterSign = payload[7] & 0x20;

      newSettings = currentSettings;
      return true;
    }
    if (cmd == 0xCB && payloadSize >= 2)
    { // We get all this from CA
      return true;
    }
    // Temperature
    if (cmd == 0xBD && payloadSize >= 29) // FCU Temp
    {
      float t;
      if ((t = (int16_t)(payload[0] + (payload[1] << 8)) / 128.0) && t < 100)
      {
        //  set_temp (inlet, t);
        this->currentStatus.roomTemperature = round(t * 2.0) / 2.0;
      }
      if ((t = (int16_t)(payload[2] + (payload[3] << 8)) / 128.0) && t < 100)
      {
        //  unknown
      }
      if ((t = (int16_t)(payload[4] + (payload[5] << 8)) / 128.0) && t < 100)
      {
        //  set_temp (liquid, t);
        this->currentStatus.coilTemperature = round(t * 2.0) / 2.0;
      }
      if ((t = (int16_t)(payload[6] + (payload[7] << 8)) / 128.0) && t < 100)
      {
        // Log.ln(TAG, "Unknown Temp = " + String(t));
      }
      if ((t = (int16_t)(payload[8] + (payload[9] << 8)) / 128.0) && t < 100)
      {
        this->currentSettings.temperature = round(t * 2.0) / 2.0;
        newSettings = currentSettings;
      }
      // Counter steps once per 2 operating hours (1250 counts = 2500 h filter sign)
      this->currentStatus.filterHours = (payload[14] + (payload[15] << 8)) * 2;
      this->currentStatus.filterHoursLeft = (payload[27] + (payload[28] << 8)) * 2;
      this->currentStatus.runningHours = (payload[18] + (payload[19] << 8)) * 2;
      this->currentStatus.poweredHours = (payload[24] + (payload[25] << 8)) * 2;
      // Tracks like relative humidity: absolute humidity stays flat while the room heats up
      this->currentStatus.humidity = payload[12];
      return true;
    }

    if (cmd == 0xB7 && payloadSize >= 32)
    { // CDU Status
      float t;
      if ((t = (int16_t)(payload[0] + (payload[1] << 8)) / 128.0) && t < 100)
      {
        this->currentStatus.outsideTemperature = round(t * 2.0) / 2.0;
      }
      this->currentStatus.outdoorTemp2 = round((int16_t)(payload[2] + (payload[3] << 8)) / 12.8) / 10.0;
      this->currentStatus.outdoorTemp3 = round((int16_t)(payload[4] + (payload[5] << 8)) / 12.8) / 10.0;
      this->currentStatus.outdoorTemp4 = round((int16_t)(payload[6] + (payload[7] << 8)) / 12.8) / 10.0;
      this->currentStatus.pipeTemp2 = round((int16_t)(payload[24] + (payload[25] << 8)) / 12.8) / 10.0;
      this->currentStatus.compressorLoad = payload[28] + (payload[29] << 8);
      // if ((t = (int16_t)(payload[2] + (payload[3] << 8)) / 128.0) && t < 100)
      // {
      //     //Unknown
      //     // Log.ln(TAG, "t2 = " + String(t));
      // }
      // if ((t = (int16_t)(payload[4] + (payload[5] << 8)) / 128.0) && t < 100)
      // {
      //     //Unknown
      //     // Log.ln(TAG, "t3 = " + String(t));

      // }
      // if ((t = (int16_t)(payload[6] + (payload[7] << 8)) / 128.0) && t < 100)
      // {
      //     //Unknown
      //     // Log.ln(TAG, "t4 = " + String(t));
      // }
      // if ((t = (int16_t)(payload[8] + (payload[9] << 8)) / 128.0) && t < 100)
      // {
      //     //Unknown
      //     // Log.ln(TAG, "t5 = " + String(t));
      // }

      // if ((t = (int16_t)(payload[10] + (payload[11] << 8)) / 128.0) && t < 100)
      // {
      //     //Unknown
      //     // Log.ln(TAG, "t6 = " + String(t));
      // }

      if ((t = (int16_t)((payload[26] + (payload[27] << 8)) / 10)) == 0.0 || t < 200)
      {

        // CDU Frequency?
        // Log.ln(TAG, "CF = " + String(t));
        currentStatus.compressorFrequency = int(t);
        this->currentStatus.operating = t != 0.0;
      }

      return true;
    }
    // Hour meters, low byte before each counts minutes 0-119
    if (cmd == 0xB9 && payloadSize >= 22)
    {
      this->currentStatus.compressorHours = (payload[8] + (payload[9] << 8)) * 2;
      this->currentStatus.outdoorFanHours = (payload[20] + (payload[21] << 8)) * 2;
      this->currentStatus.compressorStage = payload[2];
      this->currentStatus.compressorStop = payload[3];
      if (payloadSize >= 34)
        this->currentStatus.pipeTempDiff = round((int16_t)(payload[32] + (payload[33] << 8)) / 12.8) / 10.0;
      return true;
    }
    // Fan Speed & Vertical Vane (Flap)
    if (cmd == 0xBE && payloadSize >= 9)
    {
      int rpm = payload[2] + (payload[3] << 8);
      rpm = (rpm / 10) * 10; // round to nearest tenth
      this->currentStatus.fanRPM = rpm;
      this->currentStatus.drainPump = payload[5] & 0x01;

      this->currentSettings.verticalVane = lookupByteMapValue(X50_VERTICALVANE_MAP, X50_VERTICALVANE, 7, payload[4]);

      return true;
    }
  }

  return false;
}

bool DaikinController::readState()
{
  Log.ln(TAG, "** AC Status *****************************");
  if (!this->currentStatus.modelName.isEmpty())
    Log.ln(TAG, "\tModel: " + this->currentStatus.modelName);
  Log.ln(TAG, "\tPower: " + String(this->currentSettings.power));
  Log.ln(TAG, "\tMode: " + String(this->currentSettings.mode) + "(" + String(this->currentStatus.operating ? "active" : "idle") + ")");
  float degc = this->currentSettings.temperature;
  float degf = degc * 1.8 + 32.0;
  Log.ln(TAG, "\tTarget: " + String(degc, 1));
  Log.ln(TAG, "\tFan: " + String(this->currentSettings.fan) + " RPM:" + String(this->currentStatus.fanRPM));
  Log.ln(TAG, "\tSwing: H:" + String(this->currentSettings.horizontalVane) + " V:" + String(currentSettings.verticalVane));
  Log.ln(TAG, "\tInside: " + String(this->currentStatus.roomTemperature, 1));
  Log.ln(TAG, "\tOutside: " + String(this->currentStatus.outsideTemperature, 1));
  Log.ln(TAG, "\tCoil: " + String(this->currentStatus.coilTemperature, 1));
  Log.ln(TAG, "\tCompressor Freq: " + String(this->currentStatus.compressorFrequency) + " Hz");
  Log.ln(TAG, "\tEnergy Meter: " + String(this->currentStatus.energyMeter) + " kWh");
  Log.ln(TAG, "\tError Code: " + this->currentStatus.errorCode );
  Log.ln(TAG, "\tPowerful Cool: " + String(this->currentSettings.powerful) );

  Log.ln(TAG, "******************************************\n");

  return true;
}

bool DaikinController::setBasic(HVACSettings *settings)
{
  newSettings.power = settings->power;
  newSettings.mode = settings->mode;
  newSettings.fan = settings->fan;
  newSettings.temperature = settings->temperature;
  newSettings.verticalVane = settings->verticalVane;
  newSettings.horizontalVane = settings->horizontalVane;

  // memcpy(&newSettings, &settings, sizeof(&newSettings));
  // Log.ln(TAG,"SetBasic : Power = " + String(settings->power));
  pendingSettings.basic = true;
  return true;
}

bool DaikinController::update(bool updateAll)
{
  bool res = true;
  uint8_t payload[256];

  // if (!daikinUART->isConnected())
  // {
  //   Log.ln(TAG,"Reconnecting...");
  //   connect(this->_serial);
  // }if(!daikinUART->isConnected()){
  //   Log.ln(TAG,"Connection Failed");
  //   return false;
  // }

  if (!daikinUART->isConnected())
  {
    Log.ln(TAG, "AC is not connected!");
    return false;
  }

  // COMMANDS for S21 Protocol
  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    Log.ln(TAG, "Set new setting %s %s %.2f %s %s %s \n", newSettings.power, newSettings.mode, newSettings.temperature, newSettings.fan, newSettings.verticalVane, newSettings.horizontalVane);

    if (pendingSettings.basic || updateAll)
    {

      payload[0] = S21_POWER[lookupByteMapIndex(S21_POWER_MAP, 2, newSettings.power)];
      payload[1] = S21_MODE[lookupByteMapIndex(S21_MODE_MAP, 7, newSettings.mode)];
      payload[2] = c10_to_setpoint_byte(lroundf(round(newSettings.temperature * 2) / 2 * 10.0)),
      payload[3] = S21_FAN[lookupByteMapIndex(S21_FAN_MAP, 7, newSettings.fan)];

      res = daikinUART->sendCommandS21('D', '1', payload, 4) & res;
      pendingSettings.basic = false;
    }

    if (pendingSettings.vane || updateAll)
    {

      bool hVane = strcmp(HORIZONTALVANE_MAP[1], newSettings.horizontalVane) == 0;
      bool vVane = strcmp(VERTICALVANE_MAP[1], newSettings.verticalVane) == 0;

      // LOGD_f(TAG,"Swing state v:%d %s h:%d %s\n", vVane, newSettings.verticalVane , hVane , newSettings.horizontalVane);

      payload[0] = ('0' + (hVane ? 2 : 0) + (vVane ? 1 : 0) + (hVane && vVane ? 4 : 0));
      payload[1] = (vVane || hVane ? '?' : '0');
      payload[2] = '0';
      payload[3] = '0';

      res = daikinUART->sendCommandS21('D', '5', payload, 4) & res;
      pendingSettings.vane = false;
    }
    
    if (pendingSettings.specialMode || updateAll)   
    {
      // D6 not working with FTKQ/FTKC, response NAK
      payload[0] = '0' + S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)];
      payload[1] = '0';
      payload[2] = '0';
      payload[3] = '0';
      res = daikinUART->sendCommandS21('D', '6', payload, 4) & res;

      // Does not work on FTKQ/FTKC Either
      // payload[0] = '0'+ S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)];    //Timer stuff
      // payload[1] = '0'+ S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)];
      // payload[2] = '0'+ S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)];
      // payload[3] = '0' + S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)];
      // res = daikinUART->sendCommandS21('D', '3', payload, 4) & res;

      pendingSettings.specialMode = false;

    }
        
    if (pendingSettings.ACconfig || updateAll)
    {

      // Command D2
      //payload[0]: 0x30 Enable IR Remote, 0x32 Disable IR Remote 
      payload[0] = (newSettings.remoteEnable ? 0x30 : 0x32);
      payload[1] = '0';
      payload[2] = '0';
      payload[3] = '0';
      pendingSettings.ACconfig = false;
      res = daikinUART->sendCommandS21('D', '2', payload, 4) & res;
    }


  }

  // Commands for X50 Protocol
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {

    Log.ln(TAG, "Set new setting");

    if (pendingSettings.basic || updateAll)
    {

      uint8_t ca[17] = {0};
      uint8_t cb[2] = {0};

      ca[0] = 2 + X50_POWER[lookupByteMapIndex(X50_POWER_MAP, 2, newSettings.power)];
      uint8_t mode = X50_MODE[lookupByteMapIndex(X50_MODE_MAP, 8, newSettings.mode)];
      ca[1] = 0x10 + mode;
      if (mode == 1 || mode == 2 || mode == 3)
      { // Temp
        int t = lroundf(newSettings.temperature * 10);
        ca[3] = t / 10;
        ca[4] = 0x80 + (t % 10);
      }

      if (mode == 1 || mode == 2)
      {
        cb[0] = mode;
      }
      else
      {
        cb[0] = 6;
      }
      cb[1] = 0x80 + ((X50_FAN[lookupByteMapIndex(X50_FAN_MAP, 6, newSettings.fan)] & 7) << 4);                 //Fan speed, bits 5-8
      cb[1] |= 0x08 + X50_VERTICALVANE[lookupByteMapIndex(X50_VERTICALVANE_MAP, 7, newSettings.verticalVane)];   //Vertical vane, bits 1-4

      res = daikinUART->sendCommandX50(0xCA, ca, sizeof(ca)) & res;
      res = daikinUART->sendCommandX50(0xCB, cb, sizeof(cb)) & res;

      pendingSettings.basic = false;
    }

    if (pendingSettings.vane || updateAll)
    {
      // unsupported
    }
  }

  else
  {
    // Other protocol is not currently supported;
    res = false;
  }

  if (res)
  {
    // Read the unit back soon instead of waiting a full sync interval
    expectedSettings = newSettings;
    confirmPending = true;
    lastSyncMs = millis() - SYNC_INTEVAL + POLL_AFTER_SET_MS;
  }

  return res;
}

bool DaikinController::commandConfirmed()
{
  if (!confirmPending)
    return false;
  if (strcmp(currentSettings.power, expectedSettings.power) != 0)
    return false;
  // Mode, fan and setpoint don't matter while the unit is off
  if (strcmp(expectedSettings.power, "OFF") != 0 &&
      (strcmp(currentSettings.mode, expectedSettings.mode) != 0 ||
       strcmp(currentSettings.fan, expectedSettings.fan) != 0 ||
       fabs(currentSettings.temperature - expectedSettings.temperature) > 0.3))
    return false;
  confirmPending = false;
  return true;
}

void DaikinController::setPowerSetting(bool setting)
{
  if (setting)
  {
    setPowerSetting(S21_POWER_MAP[1]);
  }
  else
  {
    setPowerSetting(S21_POWER_MAP[0]);
  }
}

void DaikinController::togglePower()
{
  if (currentSettings.power == S21_POWER_MAP[0])
  {
    setPowerSetting(true);
  }
  else
  {
    setPowerSetting(false);
  }
}

void DaikinController::setPowerSetting(const char *setting)
{
  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    int index = lookupByteMapIndex(S21_POWER_MAP, 2, setting);
    if (index > -1)
    {
      newSettings.power = S21_POWER_MAP[index];
    }
    else
    {
      newSettings.power = S21_POWER_MAP[0];
    }
    pendingSettings.basic = true;
  }
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    int index = lookupByteMapIndex(X50_POWER_MAP, 2, setting);
    if (index > -1)
    {
      newSettings.power = X50_POWER_MAP[index];
    }
    else
    {
      newSettings.power = X50_POWER_MAP[0];
    }
    pendingSettings.basic = true;
  }
}

bool DaikinController::getPowerSettingBool()
{
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    return currentSettings.power == S21_POWER_MAP[1];
  }
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    return currentSettings.power == X50_POWER_MAP[1];
  }
  return false;
}

const char *DaikinController::getPowerSetting()
{
  return currentSettings.power;
}

const char *DaikinController::getModeSetting()
{
  return currentSettings.mode;
}

void DaikinController::setModeSetting(const char *setting)
{
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    int index = lookupByteMapIndex(S21_MODE_MAP, 7, setting);
    if (index > -1)
    {
      newSettings.mode = S21_MODE_MAP[index];
    }
    else
    {
      newSettings.mode = S21_MODE_MAP[0];
    }
    pendingSettings.basic = true;
  }
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    int index = lookupByteMapIndex(X50_MODE_MAP, 8, setting);
    if (index > -1)
    {
      newSettings.mode = X50_MODE_MAP[index];
    }
    else
    {
      newSettings.mode = X50_MODE_MAP[0];
    }
    pendingSettings.basic = true;
  }
}

float DaikinController::getTemperature()
{
  return currentSettings.temperature;
}
void DaikinController::setTemperature(float setting)
{
  newSettings.temperature = setting;
  pendingSettings.basic = true;
}

const char *DaikinController::getFanSpeed()
{
  return currentSettings.fan;
}
void DaikinController::setFanSpeed(const char *setting)
{
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    int index = lookupByteMapIndex(S21_FAN_MAP, 7, setting);
    if (index > -1)
    {
      newSettings.fan = S21_FAN_MAP[index];
    }
    else
    {
      newSettings.fan = S21_FAN_MAP[0];
    }
    pendingSettings.basic = true;
  }
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    int index = lookupByteMapIndex(X50_FAN_MAP, 7, setting);
    if (index > -1)
    {
      newSettings.fan = X50_FAN_MAP[index];
    }
    else
    {
      newSettings.fan = X50_FAN_MAP[0];
    }
    pendingSettings.basic = true;
  }
}

const char *DaikinController::getVerticalVaneSetting()
{
  return currentSettings.verticalVane;
}
void DaikinController::setVerticalVaneSetting(const char *setting)
{
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    int index = lookupByteMapIndex(VERTICALVANE_MAP, 2, setting);
    if (index > -1)
    {
      newSettings.verticalVane = VERTICALVANE_MAP[index];
    }
    else
    {
      newSettings.verticalVane = VERTICALVANE_MAP[0];
    }
    pendingSettings.vane = true;
  }  else if (daikinUART->currentProtocol() == PROTOCOL_X50){
    int index = lookupByteMapIndex(X50_VERTICALVANE_MAP, 7, setting);
    if (index > -1)
    {
      newSettings.verticalVane = X50_VERTICALVANE_MAP[index];
    }
    else
    {
      newSettings.verticalVane = X50_VERTICALVANE_MAP[0];
    }
    pendingSettings.basic = true;
  }

}

const char *DaikinController::getHorizontalVaneSetting()
{
  return currentSettings.horizontalVane;
}
void DaikinController::setHorizontalVaneSetting(const char *setting)
{
  if (daikinUART->currentProtocol())
  {
    int index = lookupByteMapIndex(HORIZONTALVANE_MAP, 2, setting);
    if (index > -1)
    {
      newSettings.horizontalVane = HORIZONTALVANE_MAP[index];
    }
    else
    {
      newSettings.horizontalVane = HORIZONTALVANE_MAP[0];
    }
    pendingSettings.vane = true;
  }else if (daikinUART->currentProtocol() == PROTOCOL_X50){
    //NOT SUPPORT
  }
}

void DaikinController::setSettingsChangedCallback(SETTINGS_CHANGED_CALLBACK_SIGNATURE)
{
  this->settingsChangedCallback = settingsChangedCallback;
}

void DaikinController::setStatusChangedCallback(STATUS_CHANGED_CALLBACK_SIGNATURE)
{
  this->statusChangedCallback = statusChangedCallback;
}

String DaikinController::getModelName()
{
  return this->currentStatus.modelName;
}

const char *DaikinController::getPowerfulSetting(){
  return currentSettings.powerful;
}

void DaikinController::setPowerfulSetting(const char *setting){
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    int index = lookupByteMapIndex(S21_POWERFUL_MAP, 2, setting);
    if (index > -1)
    {
      newSettings.powerful = S21_POWERFUL_MAP[index];
    }
    else
    {
      newSettings.powerful = S21_POWERFUL_MAP[0];
    }
    pendingSettings.specialMode = true;
  }
}

// Enable / Disable physical controls from IR Remote / Front button.
void DaikinController::setEnableRemote(bool isEnable){
  if (daikinUART->currentProtocol()== PROTOCOL_S21)
  {
    newSettings.remoteEnable = isEnable;
    pendingSettings.ACconfig = true;
  }
}