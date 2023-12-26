#include "DaikinController.h"

#define TAG "DKCtrl"

const byte S21_POWER[2] = {'0', '1'};         // Byte value
const char *S21_POWER_MAP[2] = {"OFF", "ON"}; // Actual value

const byte X50_POWER[2] = {0, 1};         // Byte value
const char *X50_POWER_MAP[2] = {"OFF", "ON"}; // Actual value

const byte S21_MODE[7] = {'0', '1', '2', '3', '4', '6'};
const char *S21_MODE_MAP[7] = {"DISABLED", "AUTO", "DRY", "COOL", "HEAT", "FAN"};

const byte X50_MODE[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const char *X50_MODE_MAP[8] = {"FAN", "HEAT", "COOL", "AUTO", "4", "5", "6", "DRY"};
//{"fan_only", "heat", "cool", "auto", "4", "5", "6", "dry"}

const byte S21_FAN[6] = {'A', '3', '4', '5', '6', '7'};
const char *S21_FAN_MAP[6] = {"AUTO", "1", "2", "3", "4", "5"};

const byte X50_FAN[7] = {0, 1, 2, 3, 4, 5, 6};
const char *X50_FAN_MAP[7] = {"AUTO", "1", "2", "3", "4", "5", "AUTO"};
// {"auto", "1", "2", "3", "4", "5", "night"}

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

      Log.ln(TAG, "Send command: " + S21queryCmds[i]);
    

        res = daikinUART->sendCommandS21(S21queryCmds[i][0], S21queryCmds[i][1]);
        if (res)
        {
          ACResponse response = daikinUART->getResponse();
          parseResponse(&response);
        }
        // ("Result: %s\n\n", res ? "Success" : "Failed");

        success = success & res;
    }
  }
  
  else if (daikinUART->currentProtocol() == PROTOCOL_X50){
    uint8_t size = sizeof(X50queryCmds) / sizeof(uint8_t);

    //Periodically Query
    for (int i = 0; i < size; i++)
    {
        uint8_t payload[17] = {0};

        Log.ln(TAG, "Send command: " +  String(X50queryCmds[i],HEX));

        switch (X50queryCmds[i])
        {
        case 0xCA:
          res = daikinUART->sendCommandX50(X50queryCmds[i],payload,sizeof(payload));
          break;
        
        default:
          res = daikinUART->sendCommandX50(X50queryCmds[i],NULL,0);
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

    //Model Name Query
    if (this->currentStatus.modelName.isEmpty()){
      res = daikinUART->sendCommandX50(0xBA,NULL,0);
      if (res)
      {
      ACResponse response = daikinUART->getResponse();
      parseResponse(&response);
      }
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
        this->currentSettings.power = lookupByteMapValue(S21_POWER_MAP, S21_POWER, 2, payload[0]);
        this->currentSettings.mode = lookupByteMapValue(S21_MODE_MAP, S21_MODE, 7, payload[1]);
        this->currentSettings.fan = lookupByteMapValue(S21_FAN_MAP, S21_FAN, 6, payload[3]);
        this->currentSettings.temperature = ((payload[2] - 28) * 5) / 10.0;

        if (this->currentSettings.temperature < 10 || this->currentSettings.temperature > 35)
        { // Set default value if HVAC does not have current setpoint Eg. After power outage.
          this->currentSettings.temperature = 25;
        }
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;
      case '5': // F5 -> G5 -- Swing state
        this->currentSettings.verticalVane = (payload[0] & 1) ? VERTICALVANE_MAP[1] : VERTICALVANE_MAP[0];
        this->currentSettings.horizontalVane = (payload[0] & 2) ? HORIZONTALVANE_MAP[1] : HORIZONTALVANE_MAP[0];
        newSettings = currentSettings; // we need current AC setting for future control.
        return true;

      case '9': // F9 -> G9 -- Temperature
        this->currentStatus.roomTemperature = (float)((signed)payload[0] - 0x80) / 2;
        this->currentStatus.outsideTemperature = (float)((signed)payload[1] - 0x80) / 2;
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
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {
    uint8_t cmd = response->cmd1;
    uint8_t payloadSize = response->dataSize;
    uint8_t payload[256];
    memcpy(payload, response->data, payloadSize);

    // HVAC Name
    if (cmd == 0xBA && payloadSize >= 20)
    {
      char model[256];
      uint8_t modelLen = 0;
      for(int i = 0; i < payloadSize; i++){
        if (isalnum(payload[i])){
          model[i] = payload[i];
          modelLen ++;
        }else{
          break;
        }

      }
      this->currentStatus.modelName = String(model, modelLen) ;
      return true;
    }
    // Main Status
    if (cmd == 0xCA && payloadSize >= 7)
    {
      this->currentSettings.power = lookupByteMapValue(X50_POWER_MAP, X50_POWER, 2, payload[0]);
      this->currentSettings.mode = lookupByteMapValue(X50_MODE_MAP, X50_MODE, 8, payload[1]);
      this->currentSettings.fan = lookupByteMapValue(X50_FAN_MAP, X50_FAN, 6, (payload[6] >> 4) & 7);
      return true;
    }
    if (cmd == 0xCB && payloadSize >= 2)  
    { // We get all this from CA
      return true;
    }
    // Temperature
    if (cmd == 0xBD && payloadSize >= 29) //FCU Temp
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
      }
      return true;
    }

    if (cmd == 0xB7 && payloadSize >= 32){ // CDU Status
      float t;
      if ((t = (int16_t)(payload[0] + (payload[1] << 8)) / 128.0) && t < 100)
      {
          this->currentStatus.outsideTemperature = round(t * 2.0) / 2.0;
          // Log.ln(TAG, "Home Temp = " + String(t));
      }
      if ((t = (int16_t)(payload[2] + (payload[3] << 8)) / 128.0) && t < 100)
      {
          //Unknown
          // Log.ln(TAG, "t2 = " + String(t));
      }
      if ((t = (int16_t)(payload[4] + (payload[5] << 8)) / 128.0) && t < 100)
      {
          //Unknown
          // Log.ln(TAG, "t3 = " + String(t));

      }
      if ((t = (int16_t)(payload[6] + (payload[7] << 8)) / 128.0) && t < 100)
      {
          //Unknown
          // Log.ln(TAG, "t4 = " + String(t));
      }
      if ((t = (int16_t)(payload[8] + (payload[9] << 8)) / 128.0) && t < 100)
      {
          //Unknown
          // Log.ln(TAG, "t5 = " + String(t));
      }

      if ((t = (int16_t)(payload[10] + (payload[11] << 8)) / 128.0) && t < 100)
      {
          //Unknown
          // Log.ln(TAG, "t6 = " + String(t));
      }


      if ((t = (int16_t)((payload[26] + (payload[27] << 8))/10))== 0.0 || t <200){
    
          // CDU Frequency?
          // Log.ln(TAG, "CF = " + String(t));
          currentStatus.compressorFrequency = int(t);
          this->currentStatus.operating = t != 0.0;
      }
      

      return true;
    }
    if (cmd == 0xBE && payloadSize >= 9){
        this->currentStatus.fanRPM = payload[2] + (payload[3] << 8);
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
    Log.ln(TAG, "AC is not connected!");
    return false;
  }

  // COMMANDS for S21 Protocol
  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    // LOGD_f(TAG,"Set new setting %s %s %.2f %s %s %s \n", newSettings.power, newSettings.mode, newSettings.temperature, newSettings.fan, newSettings.verticalVane, newSettings.horizontalVane);

    if (pendingSettings.basic || updateAll)
    {

      payload[0] = S21_POWER[lookupByteMapIndex(S21_POWER_MAP, 2, newSettings.power)];
      payload[1] = S21_MODE[lookupByteMapIndex(S21_MODE_MAP, 7, newSettings.mode)];
      payload[2] = c10_to_setpoint_byte(lroundf(round(newSettings.temperature * 2) / 2 * 10.0)),
      payload[3] = S21_FAN[lookupByteMapIndex(S21_FAN_MAP, 6, newSettings.fan)];

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
      bool vVane = strcmp(VERTICALVANE_MAP[1], newSettings.verticalVane) == 0;

      // LOGD_f(TAG,"Swing state v:%d %s h:%d %s\n", vVane, newSettings.verticalVane , hVane , newSettings.horizontalVane);

      payload[0] = ('0' + (hVane ? 2 : 0) + (vVane ? 1 : 0) + (hVane && vVane ? 4 : 0));
      payload[1] = (vVane || hVane ? '?' : '0');
      payload[2] = '0';
      payload[3] = '0';

      res = daikinUART->sendCommandS21('D', '5', payload, 4) & res;
      pendingSettings.vane = false;
    }
  }

  // Commands for X50 Protocol
  else if (daikinUART->currentProtocol() == PROTOCOL_X50)
  {

    Log.ln(TAG,"Set new setting");

    if (pendingSettings.basic || updateAll)
    {


      uint8_t ca[17] = {0};
      uint8_t cb[2] = {0};

      ca[0] = 2 + X50_POWER[lookupByteMapIndex(X50_POWER_MAP, 2, newSettings.power)];
      uint8_t mode = X50_MODE[lookupByteMapIndex(X50_MODE_MAP, 7, newSettings.mode)];
      ca[1] = 0x10 + mode;
      if (mode >= 1 && mode <= 3)
      { // Temp
        int t = lroundf(newSettings.temperature * 10);
        ca[3] = t / 10;
        ca[4] = 0x80 + (t % 10);
      }

      if (mode == 1 || mode == 2)
      {
        cb[0] = X50_MODE[lookupByteMapIndex(X50_MODE_MAP, 7, newSettings.mode)];
      }
      else
      {
        cb[0] = 6;
      }
      cb[1] = 0x80 + ((X50_FAN[lookupByteMapIndex(X50_FAN_MAP, 6, newSettings.fan)] & 7) << 4);

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

  return res;
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

bool DaikinController::getPowerSettingBool()
{
  return currentSettings.power == S21_POWER_MAP[1];
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
  int index = lookupByteMapIndex(S21_FAN_MAP, 6, setting);
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

void DaikinController::setSettingsChangedCallback(SETTINGS_CHANGED_CALLBACK_SIGNATURE)
{
  this->settingsChangedCallback = settingsChangedCallback;
}

void DaikinController::setStatusChangedCallback(STATUS_CHANGED_CALLBACK_SIGNATURE)
{
  this->statusChangedCallback = statusChangedCallback;
}

String DaikinController::getModelName(){
  return this->currentStatus.modelName;
}