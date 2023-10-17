#include "DaikinUART.h"



static const char *const TAG = "daikin_s21";

String getHEXformatted(uint8_t *bytes, size_t len)
{
  String res;
  char buf[5];
  for (size_t i = 0; i < len; i++)
  {
    if (i > 0)
      res += ':';
    sprintf(buf, "%02X", bytes[i]);
    res += buf;
  }
  return res;
}

//------------------ DakinUART Class functions -----------------

void DaikinUART::setSerial(HardwareSerial *hardwareSerial)
{
  this->_serial = hardwareSerial;
}

bool DaikinUART::setup()
{
  Serial.println("Setting up DaikinUART");

  if (_serial == nullptr)
    return false;

  _serial->begin(S21_BAUD_RATE, SERIAL_8E2);
  _serial->setTimeout(SERIAL_TIMEOUT);


  if ( testS21Protocol()){
    Serial.println("S21 protocol detected");
    protocol = PROTOCOL_S21;
    return true;
  }

  // if (testNewProtocol())
  // {
  //   Serial.println("NEW protocol detected");
  //   protocol = PROTOCOL_NEW;
  //   return true;
  // }
  
  else{
    Serial.println("Protocol unknown");
    protocol = PROTOCOL_UNKNOWN;
    return false;
  }
}

bool DaikinUART::testNewProtocol()
{
  bool res = true;
  uint8_t testData[] = {0x01};
  res = sendCommandNew(0xAA, testData, 1);
  res = sendCommandNew(0xBA, NULL, 0);
  res = sendCommandNew(0xBB, NULL, 0);
  return res;
}

bool DaikinUART::testS21Protocol(){
  return sendCommandS21('F','1');
}


uint8_t DaikinUART::S21Checksum(uint8_t *bytes, uint8_t len)
{
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < len; i++)
  {
    checksum += bytes[i];
  }
  return checksum;
}


bool DaikinUART::isS21SetCmd(uint8_t cmd1, uint8_t cmd2){
  for (int i = 0; i < sizeof(S21setCmds)/ sizeof(String); i++){
    if (S21setCmds[i][0] == cmd1 && S21setCmds[i][1] == cmd2){
      return true;
    }
  }
  return false;
}


bool DaikinUART::run_queries(String queries[], uint8_t size)
{
  bool success = true;

  // for (auto q : queries) {
  //   std::vector<uint8_t> code(q.begin(), q.end());
  //   success = this->s21_query(code) && success;
  // }

  // for (int i = 0; i < size; i++)
  // {
  //   success = (s21_query(queries[i]).rcode.size() != 0) && success;
  // }

  return success; // True if all queries successful
}

void DaikinUART::update()
{
//   String queries[] = {"F1", "F5", "RH", "RI", "Ra", "RL", "Rd"};
//   if (this->run_queries(queries, 7) && !this->ready)
//   {
//     Serial.printf("Daikin S21 Ready\n");
//     this->ready = true;
//   }
//   if (this->debug_protocol)
//   {
//     this->dump_state();
//   }

// #ifdef S21_EXPERIMENTS
//   ESP_LOGD(TAG, "** UNKNOWN QUERIES **");
//   // auto experiments = {"F2", "F3", "F4", "F8", "F9", "F0", "FA", "FB", "FC",
//   //                     "FD", "FE", "FF", "FG", "FH", "FI", "FJ", "FK", "FL",
//   //                     "FM", "FN", "FO", "FP", "FQ", "FR", "FS", "FT", "FU",
//   //                     "FV", "FW", "FX", "FY", "FZ"};
//   // Observed BRP device querying these.
//   std::vector<std::string> experiments = {"F2", "F3", "F4", "RN",
//                                           "RX", "RD", "M", "FU0F"};
//   this->run_queries(experiments);
// #endif
}

// void DaikinS21::set_daikin_climate_settings(bool power_on,
//                                             DaikinClimateMode mode,
//                                             float setpoint,
//                                             DaikinFanMode fan_mode) {
//   // clang-format off
//   std::vector<uint8_t> cmd = {
//     (uint8_t)(power_on ? '1' : '0'),
//     (uint8_t) mode,
//     c10_to_setpoint_byte(lroundf(round(setpoint * 2) / 2 * 10.0)),
//     (uint8_t) fan_mode
//   };
//   // clang-format on
//   ESP_LOGD(TAG, "Sending basic climate CMD (D1): %s", str_repr(cmd).c_str());
//   if (!this->send_cmd({'D', '1'}, cmd)) {
//     ESP_LOGW(TAG, "Failed basic climate CMD");
//   } else {
//     this->update();
//   }
// }

// void DaikinS21::set_swing_settings(bool swing_v, bool swing_h) {
//   std::vector<uint8_t> cmd = {
//       (uint8_t) ('0' + (swing_h ? 2 : 0) + (swing_v ? 1 : 0) +
//                  (swing_h && swing_v ? 4 : 0)),
//       (uint8_t) (swing_v || swing_h ? '?' : '0'), '0', '0'};
//   ESP_LOGD(TAG, "Sending swing CMD (D5): %s", str_repr(cmd).c_str());
//   if (!this->send_cmd({'D', '5'}, cmd)) {
//     ESP_LOGW(TAG, "Failed swing CMD");
//   } else {
//     this->update();
//   }
// }

// bool DaikinUART::send_cmd(std::vector<uint8_t> code,
//                           std::vector<uint8_t> payload)
// {
//   uint8_t frame[256];
//   uint8_t i = 0;

//   for (auto b : code)
//   {
//     frame[i] = b;
//     i++;
//   }
//   for (auto b : payload)
//   {
//     frame[i] = b;
//     i++;
//   }

//   this->write_frame(frame, i);

//   int byte;
//   byte = _serial->read();

//   if (byte == -1)
//   {
//     ESP_LOGW(TAG, "Timeout waiting for ACK to %s", String(frame, HEX));
//     return false;
//   }
//   if (byte == NAK)
//   {
//     ESP_LOGW(TAG, "Got NAK for frame: %s", String(frame, HEX));
//     return false;
//   }
//   if (byte != ACK)
//   {
//     ESP_LOGW(TAG, "Unexpected byte waiting for ACK: %x",
//              byte);
//     return false;
//   }

//   return true;
// }


// void DaikinUART::write_frame(uint8_t frame[], uint8_t size) {
//   _serial->write(STX);
//   _serial->write(frame,size);
//   _serial->write(S21Checksum(frame, size));
//   _serial->write(ETX);
//   _serial->flush();

//   Serial.print("Write_frame >>");
//   Serial.printf("%x:",STX);
//   Serial.print(getHEXformatted(frame,size));
//   Serial.printf(":%x:",S21Checksum(frame, size));
//   Serial.printf("%x\n",ETX);

// }

bool DaikinUART::sendCommandNew(uint8_t cmd, uint8_t *payload, uint8_t payloadLen, bool waitResponse)
{

  uint8_t buf[256];
  uint8_t len;

  buf[0] = 0x06;
  buf[1] = cmd;
  buf[2] = payloadLen + 6;
  buf[3] = 1;
  buf[4] = 0;
  if (payloadLen)
    memcpy(buf + 5, payload, payloadLen);
  uint8_t c = 0;
  for (int i = 0; i < 5 + payloadLen; i++)
    c += buf[i];
  buf[5 + payloadLen] = 0xFF - c;

  len = 6 + payloadLen;

  // Send payload
  Serial.printf("[NEW]Send Data >>%s\n", getHEXformatted(buf, len).c_str());
  _serial->write(buf, len);

  if (!waitResponse)
    return true;

  // Read incoming payload
  uint8_t buf_in[256];
  uint8_t size_in = _serial->readBytes(buf, 256);

  Serial.printf("[NEW]Received Data <<%s\n", getHEXformatted(buf_in, size_in).c_str());
  bool responseOK = checkResponseNew(cmd, buf_in, size_in);
  Serial.printf("Response %s\n", responseOK ? "YES" : "NO");

  return responseOK;
}

bool DaikinUART::sendCommandS21(uint8_t cmd1, uint8_t cmd2)
{
  return sendCommandS21(cmd1, cmd2, NULL, 0, true);
}

bool DaikinUART::sendCommandS21(uint8_t cmd1, uint8_t cmd2, uint8_t *payload, uint8_t payloadLen, bool waitResponse)
{

  uint8_t buf[256];
  uint8_t len;
  buf[0] = STX;
  buf[1] = cmd1;
  buf[2] = cmd2;
  if (payloadLen)
    memcpy(buf + 3, payload, payloadLen);

  buf[3 + payloadLen] = S21Checksum(buf+1,  payloadLen+2);
  buf[4 + payloadLen] = ETX;

  len = S21_MIN_PKT_LEN + payloadLen;

  // Send payload
  Serial.printf("[S21]Send Data     >>%s\n", getHEXformatted(buf, len).c_str());
  _serial->write(buf, len);

  if (!waitResponse)
    return true;

  // Read incoming payload
  uint8_t buf_in[256];
  // uint8_t size_in = _serial->readBytes(buf_in, 256);
  uint8_t size_in = 0;

  unsigned long tStart = millis();
  bool isSetCMD = isS21SetCmd(cmd1,cmd2);
  while(1){
    if (_serial->available() > 0) {
      int c = _serial->read();
      if (c == -1)
        break;

      buf_in[size_in] = c;
      size_in ++;

      if (c == NAK || c == ETX || (c== ACK && isSetCMD)){
        _serial->flush();
        break;
      }
    }
    if(millis() - tStart > SERIAL_TIMEOUT){
      Serial.println("Serial read timedout");
      break;
    }
  }

  Serial.printf("[S21]Received Data <<%s\n", getHEXformatted(buf_in, size_in).c_str());
  bool responseOK = (checkResponseS21(cmd1, cmd2, buf_in, size_in) == S21_OK);
  // Serial.printf("Response %s\n", responseOK ? "YES" : "NO");

  if (responseOK){
    lastResponse.cmd1 = cmd1;
    lastResponse.cmd2 = cmd2;
    lastResponse.dataSize = size_in -1; 
    memcpy(lastResponse.data, buf_in+1, size_in-1);   //Remove 1st byte (ACK)
  }

  return responseOK;
}

// Check integrety of the new protocol
bool DaikinUART::checkResponseNew(uint8_t cmd, uint8_t *buf, uint8_t size)
{
  if (size <= 0)
  {
    Serial.println("checkResponseNew: Empty Response");
    return false;
  }

  // Check checksum
  uint8_t checksum = 0;
  for (int i = 0; i < size; i++)
  {
    checksum += buf[i];
  }
  if (checksum != 0xFF)
  {
    Serial.println("checkResponseNew: Bad Checksum");
    return false;
  }

  // Process response
  if (buf[1] == 0xFF)
  {
    Serial.println("checkResponseNew: Wrong byte 1");
    return false;
  }
  if (size < 6 || buf[0] != 0x06 || buf[1] != cmd || buf[2] != size || buf[3] != 1)
  { // Basic checks
    if (buf[0] != 0x06)
      Serial.println("checkResponseNew: Bad Header");
    if (buf[1] != cmd)
      Serial.println("checkResponseNew: Received response mismatch sent command");
    if (buf[2] != size || size < 6)
      Serial.println("checkResponseNew: Bad size");
    if (buf[3] != 1)
      Serial.println("checkResponseNew: Bad form");
    return false;
  }
  if (!buf[4]) // Loopback
  {
    Serial.println("checkResponseNew: Loopback Detected");
    return false;
  }

  return true;
}

// Check integregity of the new protocol
int DaikinUART::checkResponseS21(uint8_t cmd1, uint8_t cmd2, uint8_t *buf, uint8_t size)

{
  uint8_t idx = 0;

  //incoming packet should be [ACK, STX, CMD1+1, CMD2, <DATA>, CRC, ETX]

  if (size <= 0)
  {
    Serial.println("checkResponseS21: Empty response");
    connected = false;
    return S21_BAD;
  }

  if (buf[idx] != ACK && buf[idx] != STX)
  {

    if (buf[idx] == NAK)
    {
      // Got an explicit NAK
      Serial.println("checkResponseS21: Got explicted NAK");
      connected = false;
      return S21_NAK;
    }

    Serial.println("checkResponseS21: Got unexpected data, protocol broken.");
    connected = false;
    return S21_NOACK;
  }

  // Check Start of text (STX)
  if (buf[idx] != STX)
  {
    if (cmd1 == 'D')
    {
      connected = true;
      return S21_OK; // No response expected
    }

    bool foundSTX = false;
    for (idx; idx < size; idx++)
    {
      if (buf[idx] == STX)
      { // Found STX
        foundSTX = true;
        break;
      }
    }
    if (!foundSTX)
    {
      connected = false;
      Serial.println("checkResponseS21: Got no STX");
      return S21_NOACK;
    }
  }

  // Get data and check for End of text (ETX)
  uint8_t data[256];
  uint8_t dataLen = 0;
  bool foundETX = false;
  for (idx; idx < size; idx++)
  {
    if (buf[idx] == ETX)
    { // Found ETX
      foundETX = true;
      break;
    }
    data[dataLen] = buf[idx];
    dataLen++;
  }
  if (!foundETX)
  {
    connected = false;
    Serial.println("checkResponseS21: Got no ETX");
    return S21_NOACK;
  }
  // Send confirm receive to AC
  _serial->write(ACK);

  
  // Check Checksum
  if (S21Checksum(buf+2, size-4) != buf[size-2])        //Offset front: ACK STX, Offset back: CRC ETX
  {

    connected = false;
    Serial.println("checkResponseS21: Checksum Error");
    return S21_BAD;

  }
  // An expected S21 reply contains the first character of the command
  // incremented by 1, the second character is left intact
  // Serial.printf("%d %d %d %d %d\n", size < S21_MIN_PKT_LEN + 1 , buf[S21_STX_OFFSET + 1] != STX , buf[size - 1] != ETX , buf[S21_CMD1_OFFSET+1] != cmd1 + 1 , buf[S21_CMD2_OFFSET+1] != cmd2);
  if (size < S21_MIN_PKT_LEN + 1 || buf[S21_STX_OFFSET + 1] != STX || buf[size - 1] != ETX || buf[S21_CMD1_OFFSET+1] != cmd1 + 1 || buf[S21_CMD2_OFFSET+1] != cmd2)
  {
    connected = false;
    Serial.println("checkResponseS21: Message Malformed");
    return S21_BAD;
  }

  connected = true;
  return S21_OK;

}


ACResponse DaikinUART::getResponse(){
  return lastResponse;
}