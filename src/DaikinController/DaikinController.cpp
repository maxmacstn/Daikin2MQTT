#include "DaikinController.h"

#define TAG "DKCtrl"


const byte POWER[2] = {'0', '1'};
const char *POWER_MAP[2] = {"OFF", "ON"};
const byte MODE[7] = {'0', '1', '2', '3', '4', '6'};
const char *MODE_MAP[7] = {"DISABLED", "AUTO", "DRY", "COOL", "HEAT", "FAN"};
// const byte TEMP[16]            = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
// const int TEMP_MAP[16]         = {31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16};
const byte FAN[6] = {'A', '3', '4', '5', '6', '7'};
const char *FAN_MAP[6] = {"AUTO", "1", "2", "3", "4", "5"};

const byte VERTICALVANE[2] = {'0', '1'};
const char *VERTICALVANE_MAP[2] = {"HOLD", "SWING"};
const byte HORIZONTALVANE[2] = {'0', '1'};
const char *HORIZONTALVANE_MAP[2] = {"HOLD", "SWING"};

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
  default:
    return "UNKNOWN";
  }
}

bool DaikinController::sync()
{

  if (millis() - lastSyncMs < SYNC_INTEVAL){
    return false;
  }

  uint8_t size = sizeof(S21queryCmds) / sizeof(String);
  bool success = true;

  for (int i = 0; i < size; i++)
  {

    Log.ln(TAG, "Send command: "+ S21queryCmds[i]);
    if (daikinUART->currentProtocol() == PROTOCOL_S21)
    {
      bool res = daikinUART->sendCommandS21(S21queryCmds[i][0], S21queryCmds[i][1]);
      if (res)
      {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      }
      // ("Result: %s\n\n", res ? "Success" : "Failed");

      success = success & res;
    }
  }

  lastSyncMs = millis();
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
        this->currentSettings.power = lookupByteMapValue(POWER_MAP, POWER, 2, payload[0]);
        this->currentSettings.mode = lookupByteMapValue(MODE_MAP, MODE, 7, payload[1]);
        this->currentSettings.temperature = ((payload[2] - 28) * 5) / 10.0;
        this->currentSettings.fan = lookupByteMapValue(FAN_MAP, FAN, 6, payload[3]);
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;
      case '5': // F5 -> G5 -- Swing state
        this->currentSettings.verticalVane = (payload[0] & 1) ? VERTICALVANE_MAP[1] : VERTICALVANE_MAP[0];
        this->currentSettings.horizontalVane = (payload[0] & 2) ? HORIZONTALVANE_MAP[1] : HORIZONTALVANE_MAP[0];
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;
      
      case '9': // F9 -> G9 -- Temperature
        this->currentStatus.roomTemperature = (float) ((signed) payload[0] - 0x80) / 2;
        this->currentStatus.outsideTemperature = (float) ((signed) payload[1] - 0x80) / 2;
        newSettings = currentSettings; // we need current AC setting for future control.
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
      case 'd': // Compressor state / frequency? Idle if 0.
        this->currentStatus.operating = !(payload[0] == '0' && payload[1] == '0' && payload[2] == '0');
        this->currentStatus.compressorFrequency = (payload[0] - '0') + (payload[1] - '0') * 10 + (payload[2] - '0') * 100;
        return true;
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


  return false;
}

bool DaikinController::readState()
{
  Log.ln(TAG,"** AC Status *****************************");

  Log.ln(TAG,"\tPower: " + String(this->currentSettings.power));
  Log.ln(TAG,"\tMode: " + String(this->currentSettings.mode) + "("+ String(this->currentStatus.operating ? "active" : "idle") + ")");
  float degc = this->currentSettings.temperature;
  float degf = degc * 1.8 + 32.0;
  Log.ln(TAG,"\tTarget: "+ String(degc, 1));
  Log.ln(TAG,"\tFan: " + String(this->currentSettings.fan) + " RPM:" +String(this->currentStatus.fanRPM));
  Log.ln(TAG,"\tSwing: H:" + String(this->currentSettings.horizontalVane) + " V:"+ String(currentSettings.verticalVane));
  Log.ln(TAG,"\tInside: " + String(this->currentStatus.roomTemperature,1));
  Log.ln(TAG,"\tOutside: " + String(this->currentStatus.outsideTemperature,1));
  Log.ln(TAG,"\tCoil: " + String(this->currentStatus.coilTemperature,1));
  Log.ln(TAG,"\tCompressor Freq: " + String( this->currentStatus.compressorFrequency) + " Hz");

  Log.ln(TAG,"******************************************\n");

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
  // LOGD_f(TAG,"setting %s \n", settings->power);
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
    Log.ln(TAG,"AC is not connected!");
    return false;
  }


  //COMMANDS for S21 Protocol
  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    // LOGD_f(TAG,"Set new setting %s %s %.2f %s %s %s \n", newSettings.power, newSettings.mode, newSettings.temperature, newSettings.fan, newSettings.verticalVane, newSettings.horizontalVane);

    if (pendingSettings.basic || updateAll)
    {

      payload[0] = POWER[lookupByteMapIndex(POWER_MAP, 2, newSettings.power)];
      payload[1] = MODE[lookupByteMapIndex(MODE_MAP, 7, newSettings.mode)];
      payload[2] = c10_to_setpoint_byte(lroundf(round(newSettings.temperature * 2) / 2 * 10.0)),
      payload[3] = FAN[lookupByteMapIndex(FAN_MAP, 6, newSettings.fan)];
      
      // std::vector<uint8_t> cmd = {
      //     (uint8_t)'0',
      //     (uint8_t)'3',
      //     c10_to_setpoint_byte(lroundf(round(newSettings.temperature * 2) / 2 * 10.0)),
      //     (uint8_t)'2'};

      // LOGD_f(TAG,"Setting payload %x %x %x %x\n", cmd[0], cmd[1] ,cmd[2], cmd[3]);

      // Log.ln(TAG, "sending command");
      // Log.ln(TAG, "Free Stack Space:" + String(uxTaskGetStackHighWaterMark(NULL)));
      // delay(50);
      res = daikinUART->sendCommandS21('D', '1', payload, 4) & res;
      pendingSettings.basic = false;
    }

    if (pendingSettings.vane || updateAll)
    {

      bool hVane = strcmp(HORIZONTALVANE_MAP[1], newSettings.horizontalVane) == 0;
      bool vVane = strcmp(VERTICALVANE_MAP[1] , newSettings.verticalVane) == 0;

      // LOGD_f(TAG,"Swing state v:%d %s h:%d %s\n", vVane, newSettings.verticalVane , hVane , newSettings.horizontalVane);

      payload[0] = ('0' + (hVane ? 2 : 0) + (vVane ? 1 : 0) + (hVane && vVane ? 4 : 0));
      payload[1] = (vVane || hVane ? '?' : '0');
      payload[2] = '0';
      payload[3] = '0';

      res = daikinUART->sendCommandS21('D', '5', payload, 4) & res;
      pendingSettings.vane = false;
    }


  }

  else{
    //Other protocol is not currently supported;
    res = false;
  }

  return res;
}

void DaikinController::setPowerSetting(bool setting)
{
  if (setting)
  {
    setPowerSetting(POWER_MAP[1]);
  }
  else
  {
    setPowerSetting(POWER_MAP[0]);
  }
}

void DaikinController::togglePower(){
  if (currentSettings.power == POWER_MAP[0]){
      setPowerSetting(true);
  }else{
    setPowerSetting(false);
  }
}

void DaikinController::setPowerSetting(const char *setting)
{
  int index = lookupByteMapIndex(POWER_MAP, 2, setting);
  if (index > -1)
  {
    newSettings.power = POWER_MAP[index];
  }
  else
  {
    newSettings.power = POWER_MAP[0];
  }
  pendingSettings.basic = true;
}

bool DaikinController::getPowerSettingBool()
{
  return currentSettings.power == POWER_MAP[1];
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
  int index = lookupByteMapIndex(MODE_MAP, 7, setting);
  if (index > -1)
  {
    newSettings.mode = MODE_MAP[index];
  }
  else
  {
    newSettings.mode = MODE_MAP[0];
  }
  pendingSettings.basic = true;
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
  int index = lookupByteMapIndex(FAN_MAP, 6, setting);
  if (index > -1)
  {
    newSettings.fan = FAN_MAP[index];
  }
  else
  {
    newSettings.fan = FAN_MAP[0];
  }
  pendingSettings.basic = true;
}

const char *DaikinController::getVerticalVaneSetting()
{
  return currentSettings.verticalVane;
}
void DaikinController::setVerticalVaneSetting(const char *setting)
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
}

const char *DaikinController::getHorizontalVaneSetting()
{
  return currentSettings.horizontalVane;
}
void DaikinController::setHorizontalVaneSetting(const char *setting)
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
}

void DaikinController::setSettingsChangedCallback(SETTINGS_CHANGED_CALLBACK_SIGNATURE) {
  this->settingsChangedCallback = settingsChangedCallback;
}

void DaikinController::setStatusChangedCallback(STATUS_CHANGED_CALLBACK_SIGNATURE) {
  this->statusChangedCallback = statusChangedCallback;
}