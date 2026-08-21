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
const char *S21_FAN_MAP[7] = {"auto", "quiet", "1", "2", "3", "4", "5"};

// const byte X50_FAN[7] = {0, 1, 2, 3, 4, 5, 6};
const byte X50_FAN[7] = {0, 1, 2, 3, 4, 5, 6};
const char *X50_FAN_MAP[7] = {"auto", "1", "2", "3", "4", "5", "6"};

const byte X50_VERTICALVANE[6] = {0, 1, 2, 3, 4, 7};
const char *X50_VERTICALVANE_MAP[6] = {"0", "1", "2", "3", "4", "swing"};
// {"auto", "1", "2", "3", "4", "5", "night"}

const byte VERTICALVANE[2] = {'0', '1'};
const char *VERTICALVANE_MAP[2] = {"hold", "swing"};

const byte HORIZONTALVANE[2] = {'0', '1'};
const char *HORIZONTALVANE_MAP[2] = {"hold", "swing"};


const byte S21_POWERFUL[2] = {0x00, 0x02};
const char *S21_POWERFUL_MAP[2] = {"OFF", "ON"};

// FC code → friendly model family lookup. Daikin's FC command returns a 4-char
// ASCII MCU/PCB firmware identifier ("7B91" on our two units) which isn't the
// customer-visible model name. Faikout suspects the first byte encodes capacity,
// but we lack the data to bit-decode — only the family is claimed here.
// Add new entries as we encounter other units. The raw code is preserved in the
// firmware log on first GC parse, so it's always recoverable.
// manualUrl: Daikin's online manual root for this model (region-specific —
// e.g. dit-daikin.com is Daikin Thailand). Empty when no manual is mapped.
struct DaikinModelMap { const char *code; const char *family; const char *manualUrl; };
const DaikinModelMap DAIKIN_KNOWN_MODELS[] = {
  // FC sits on the indoor PCB so the most likely identification is the indoor
  // unit (FTKD15ZV2S). But indoor+outdoor are factory-paired, so the same code
  // also implies the matched outdoor RKD15ZV2S. We claim the customer-facing
  // family name FTKD-ZV2S without making a BTU/capacity claim until we see
  // another capacity variant.
  {"7B91", "FTKD-ZV2S", "https://web-manual.dit-daikin.com/ra/3P730851-16/en/"},
  // FC "A7D0" → FTKC-RV2S (confirmed via FTKC18RV2S boot log, 2026-05-20). Same v2
  // wall-unit class; NAKs all FU sub-commands and FV (FTKD-ZV2S is the only one of
  // the three that answers them — see docs/UNEXPLORED_S21_FEATURES.md).
  {"A7D0", "FTKC-RV2S", "https://web-manual.dit-daikin.com/ra/3P730851-17/en/"},
  // FC "DD11" → FTKQ-UV2S (reported by maxmacstn upstream; confirmed via FTKQ12UV2S
  // boot log, 2026-05-20). Same v2 wall-unit class; D6/special-mode writes NAK on
  // FTKQ/FTKC per upstream comments.
  {"DD11", "FTKQ-UV2S", "https://web-manual.dit-daikin.com/ra/3P658678-13/en/"},
};

// F6/D6 special-mode bit layout (FTKD-zv2s v2 — verified by remote-button probe):
//   byte 0 bit 1 (0x02): powerful  (legacy v0/v1 indicator — RzB2 authoritative on v2)
//   byte 0 bit 6 (0x40): comfort   (redirects vertical louver toward ceiling)
//   byte 0 bit 7 (0x80): quiet     (outdoor unit quiet mode — slower compressor)
//   byte 1 bit 7 (0x80): streamer  (mold/odor prevention discharge)
const byte S21_COMFORT[2]  = {0x00, 0x40};
const char *S21_COMFORT_MAP[2]  = {"OFF", "ON"};
const byte S21_QUIET[2]    = {0x00, 0x80};
const char *S21_QUIET_MAP[2]    = {"OFF", "ON"};
const byte S21_STREAMER[2] = {0x00, 0x80};
const char *S21_STREAMER_MAP[2] = {"OFF", "ON"};

// F7/D7 econo mode — outdoor-unit power cap.
//   byte 1 bit 1 (0x02): econo
const byte S21_ECONO[2]   = {0x00, 0x02};
const char *S21_ECONO_MAP[2]   = {"OFF", "ON"};

// D7 byte 0 — demand control (compressor power limit). byte 0 = '0' + (100 - percent),
// so the offset table below is (100 - percent). Verified on FTKD-zv2s (DAIKIN_GUESTROOM,
// 2026-05-20): writing 40% capped the compressor at 40Hz vs 48Hz uncapped, reversibly.
// The unit does NOT echo the active cap (G7 byte 0 always reads 100%), so this is
// write-only — never decoded back from F7; HA entity state is optimistic/local.
const byte S21_DEMAND[5]      = {0, 20, 30, 40, 60};
const char *S21_DEMAND_MAP[5] = {"Off", "80%", "70%", "60%", "40%"};

// F6/D6 byte 3 — indoor-unit display (LED) brightness. 2-bit field, mask 0x0C.
// Captured on FTKD-zv2s (DAIKIN_GUESTROOM, 2026-05-20) by cycling the remote's
// MENU brightness menu and reading F6 back: High=0x34 (bit 2), Low=0x38 (bit 3),
// Off=0x3C (bits 2+3). i.e. field 0x04=High, 0x08=Low, 0x0C=Off; 0x00 is invalid
// (an earlier guess used 0x00 for High — the unit ignored it, so High behaved the
// same as Low). The byte is the usual ASCII-offset form ('0' + bits); we match on
// the raw field (payload & 0x0C).
const byte S21_LED[3]      = {0x04, 0x08, 0x0C};
const char *S21_LED_MAP[3] = {"High", "Low", "Off"};

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

// Length-safe reverse-ASCII decimal decode for short R-class single-field reads
// (e.g. RA="1", RF="00"). bytes_to_num assumes >=3 bytes; these can be 1-2.
// Returns -1 if any byte isn't an ASCII digit so the caller can fall back to raw.
int16_t s21_ascii_decimal(const uint8_t *bytes, size_t len)
{
  int16_t val = 0, mul = 1;
  for (size_t i = 0; i < len && i < 4; i++) {
    if (bytes[i] < '0' || bytes[i] > '9') return -1;
    val += (bytes[i] - '0') * mul;
    mul *= 10;
  }
  return val;
}

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
  else if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {
    // Flush the persisted IR-remote-enable state to the AC on first connect.
    // Newer units (e.g. model 7B91, protocol v2) auto-lock the IR remote when an
    // S21 master appears; without this, the user's last saved choice is ignored
    // on reboot. syncNewSettings() first to pick up the AC's current state for
    // every other field (otherwise newSettings still has constructor defaults
    // for power/mode/fan because pendingSettings.ACconfig=true blocked the gated
    // syncNewSettings inside parseResponse).
    syncNewSettings();
    pendingSettings.ACconfig = true;
    update();
  }
}

// sync() — Main polling loop, called every ~10s from main loop.
// First cycle: probes all 32 S21 commands. NAK'd commands get skip-masked.
// Subsequent cycles: only polls supported commands (typically 17-19 on v0 units).
// One-shot commands (F2 capabilities, F8 protocol version) self-skip after first ACK.
// After sync, parseResponse() populates currentSettings/currentStatus which
// triggers the MQTT status callback in main.cpp.
bool DaikinController::sync()
{
  static bool firstSuccess = true;
  bool success = true;
  bool res = false;

  if (millis() - lastSyncMs < _syncIntervalMs)
  {
    return false;
  }

  if (daikinUART->currentProtocol() == PROTOCOL_S21)
  {

    uint8_t size = sizeof(S21queryCmds) / sizeof(S21queryCmds[0]);

    for (int i = 0; i < size; i++)
    {
      if (s21SkipMask & (1ULL << i)) continue;

      Log.ln(TAG, "Send command: %s", S21queryCmds[i]);

      res = daikinUART->sendCommandS21(S21queryCmds[i][0], S21queryCmds[i][1]);
      if (res)
      {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      }
      else if (daikinUART->lastResultWasNAK())
      {
        s21SkipMask |= (1ULL << i);
        Log.ln(TAG, "Command %s not supported, skipping", S21queryCmds[i]);
      }
      success = success | res;
    }

    // RzB2: read-only powerful/defrost status (fallback when F6 unsupported)
    if ((s21SkipMask & (1ULL << 5)) && !_skipRzB2) { // F6 at index 5
      uint8_t rzPayload[] = {'B', '2'};
      res = daikinUART->sendCommandS21('R', 'z', rzPayload, 2);
      if (res) {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      } else if (daikinUART->lastResultWasNAK()) {
        _skipRzB2 = true;
        Log.ln(TAG, "RzB2 not supported, skipping");
      }
    }

    // FU<sub> extension reads — protocol v2+. Each sub-command is a 2-char ASCII
    // selector echoed back at the start of the response payload. Self-skip on NAK.
    //   FU00 — en_spmode bitmap (which special modes are available on this unit)
    //   FU04 — telemetry vector (originally suspected lifetime kWh; sample-disproved)
    // (FU02 heat/humidity-limits dropped in 1.4-b5 — constant/useless on all models.)
    struct FUSend { const char *sub; bool *skip; } fuCmds[] = {
      {"00", &_skipFU00}, {"04", &_skipFU04}
    };
    for (auto &fu : fuCmds) {
      if (*fu.skip) continue;
      uint8_t fuPayload[] = { (uint8_t)fu.sub[0], (uint8_t)fu.sub[1] };
      res = daikinUART->sendCommandS21('F', 'U', fuPayload, 2);
      if (res) {
        ACResponse response = daikinUART->getResponse();
        parseResponse(&response);
      } else if (daikinUART->lastResultWasNAK()) {
        *fu.skip = true;
        Log.ln(TAG, "FU%s not supported, skipping", fu.sub);
      }
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
        // Copy AC's actual state to newSettings so future commands use current values.
        // Guard: don't overwrite if user has pending changes not yet sent.
        // Preserve remoteEnable: S21 doesn't report lock state, so the AC's
        // currentSettings always has the default. We must keep the user's choice.
        if (!pendingSettings.hasPending())
          syncNewSettings();
        return true;

      case '2': // F2 -> G2 -- Hardware capabilities (static, read once)
        _supportsVerticalSwing = (payload[0] & 0x04) != 0;   // bit 2
        _supportsHorizontalSwing = (payload[0] & 0x08) != 0; // bit 3
        Log.ln(TAG, "Capabilities: vSwing=%d hSwing=%d", _supportsVerticalSwing, _supportsHorizontalSwing);
        s21SkipMask |= (1ULL << S21_QUERY_F2);
        return true;

      case '3': // F3 -> G3 -- Timer + powerful status (authoritative timer source)
        // byte 0 = enable ('0'..'3'), byte 1 = on-timer, byte 2 = off-timer
        // (0x30 + 10-min periods, 0xFE = disabled). G3 is the live source — RD/RE
        // are stale ("only update when first set" per Faikout) so we read timers here.
        this->currentStatus.timerMode = payload[0] - '0';
        if (payloadSize > 1)
          this->currentStatus.onTimerMinutes  = (payload[1] == 0xFE) ? 0 : (payload[1] - 0x30) * 10;
        if (payloadSize > 2)
          this->currentStatus.offTimerMinutes = (payload[2] == 0xFE) ? 0 : (payload[2] - 0x30) * 10;
        // If F6 is unsupported, read powerful from G3 byte 3 bit 1
        if (s21SkipMask & (1ULL << S21_QUERY_F6)) {
          this->currentSettings.powerful = (payload[3] & 0x02) ? S21_POWERFUL_MAP[1] : S21_POWERFUL_MAP[0];
        }
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
        if (!pendingSettings.hasPending())
          syncNewSettings();
        return true;

      case '6': // F6 -> G6 -- Powerful / comfort / quiet / streamer / display brightness
        // Byte layout (validated on FTKD-zv2s v2 via remote-button probe rounds):
        //   byte 0 bit 1 (0x02) = powerful (legacy v0/v1; RzB2 authoritative on v2)
        //   byte 0 bit 6 (0x40) = comfort airflow (redirects louver to ceiling)
        //   byte 0 bit 7 (0x80) = quiet (outdoor unit quiet mode)
        //   byte 1 bit 7 (0x80) = streamer (mold/odor prevention discharge)
        //   byte 3 bits 2+3 (0x0C) = display brightness: High=0x04, Low=0x08, Off=0x0C
        this->currentSettings.powerful = (payload[0] & 0x02) ? S21_POWERFUL_MAP[1] : S21_POWERFUL_MAP[0];
        this->currentSettings.comfort  = (payload[0] & 0x40) ? S21_COMFORT_MAP[1]  : S21_COMFORT_MAP[0];
        this->currentSettings.quiet    = (payload[0] & 0x80) ? S21_QUIET_MAP[1]    : S21_QUIET_MAP[0];
        if (payloadSize > 1)
          this->currentSettings.streamer = (payload[1] & 0x80) ? S21_STREAMER_MAP[1] : S21_STREAMER_MAP[0];
        if (payloadSize > 3) {
          const char *led = lookupByteMapValue(S21_LED_MAP, S21_LED, 3, payload[3] & 0x0C);
          this->currentSettings.ledBrightness = led;
          // Keep the write-staging copy in sync with the unit unless a brightness
          // change is already queued — otherwise an unrelated special-mode write
          // (streamer/powerful/…) would re-send a stale brightness and clobber it.
          if (!pendingSettings.specialMode)
            this->newSettings.ledBrightness = led;
          if (!_ledBrightnessSeen) { _ledBrightnessSeen = true; _rediscoverNeeded = true; }
        }
        return true;


      case '7': // F7 -> G7 -- Demand control + econo
      {
        if (payloadSize > 1)
          this->currentSettings.econo = (payload[1] & 0x02) ? S21_ECONO_MAP[1] : S21_ECONO_MAP[0];
        return true;
      }

      case '8': // F8 -> G8 -- Protocol version (static, read once)
      {
        uint8_t protoVer = (payloadSize > 1) ? (payload[1] & ~0x30) : 0;
        this->_protocolVersion = protoVer;
        Log.ln(TAG, "G8 Protocol version=%d (raw: %02X %02X %02X)", protoVer,
          payloadSize > 0 ? payload[0] : 0, payloadSize > 1 ? payload[1] : 0,
          payloadSize > 2 ? payload[2] : 0);
        s21SkipMask |= (1ULL << S21_QUERY_F8);
        return true;
      }

      case '9': // F9 -> G9 -- byte 0 = coarse room temp, byte 2 = humidity (Faikout)
      // RH/Ra give higher-res temps so we ignore byte 0 here.
      // byte 2 humidity encoding: 0x30 + (humidity/5); 0xFF = sensor unsupported.
      // FTKD15ZV2S returns 0xFF — no humidity sensor on this SKU. The Re/Se
      // command on this same SKU returns "050" which is Faikout's documented
      // "no humidity sensor" placeholder — consistent with F9 byte 2.
      {
        if (payloadSize > 2 && payload[2] != 0xFF) {
          this->currentStatus.humidity = (payload[2] - 0x30) * 5;
          _humiditySensorPresent = true;
        }
        return true;
      }

      // FC — Model identification
      //   Direction: read (controller → AC)
      //   Response:  GC, ASCII model string (variable length, padded)
      //   Payload:   printable ASCII chars (alphanumeric, '-', ' '), stops on first non-printable
      //   Protocol:  v0+ (universal)
      //   Static:    one-shot, skip-masked after first ACK
      //   Source:    Faikout wiki S21-Protocol, verified on FTKQ + FTKD-zv2s ("7B91")
      case 'C':
      {
        char model[32] = {0};
        int len = (payloadSize > 31) ? 31 : payloadSize;
        int modelLen = 0;
        for (int i = 0; i < len; i++) {
          if (isalnum(payload[i]) || payload[i] == '-' || payload[i] == ' ') {
            model[modelLen++] = payload[i];
          } else {
            break;
          }
        }
        // Translate the raw 4-char FC code to a friendly family name if we know
        // it; fall back to the raw code so unknown units still show something.
        const char *friendly = model;
        const char *manualUrl = "";
        for (auto &m : DAIKIN_KNOWN_MODELS) {
          if (strcmp(model, m.code) == 0) { friendly = m.family; manualUrl = m.manualUrl; break; }
        }
        this->currentStatus.modelName = String(friendly);
        this->currentStatus.manualUrl = String(manualUrl);
        Log.ln(TAG, "GC Model: raw=%s friendly=%s (%d bytes)", model, friendly, payloadSize);
        s21SkipMask |= (1ULL << S21_QUERY_FC);
        return true;
      }

      case 'M':                                                                  // FM -> GM -- Energy meter
        this->currentStatus.energyMeter = s21_decode_hex_sensor(payload) / 10.0; // kWh
        return true;

      case 'A': // FA -> GA -- energy register (reversed-nibble ASCII-hex, /10 = kWh)
        // Identical to FM on FTKD (cumulative kWh, live-verified); diverges on
        // FTKC/FTKQ. Exposed as a separate diagnostic to cross-check the meter.
        this->currentStatus.energyMeterFA = s21_decode_hex_sensor(payload) / 10.0;
        return true;

      case 'K': // FK -> GK -- capability bitmap (folded into HA climate attributes)
      {
        // Keep the ASCII rendering ("qs51"/"0:11"/"0251") for reference. Bit
        // semantics are per-model; only byte3 b0 (demand-mode available) is
        // reliable across all three known models, so that's all we decode.
        String ascii;
        for (uint8_t i = 0; i < payloadSize; i++)
          ascii += isprint(payload[i]) ? (char)payload[i] : '?';
        _fkRaw = ascii;
        if (payloadSize >= 4) _demandAvailable = (payload[3] & 0x01);
        return true;
      }

      // FU<sub> -> GU<sub> -- protocol-v2 extension reads.
      // Payload[0..1] echoes the sub-command; remaining bytes are the data.
      case 'U':
      {
        if (payloadSize < 2) return false;
        if (payload[0] == '0' && payload[1] == '0') {
          _fu00Seen = true;  // v2 special-mode extension present (FTKD; not FTKC/FTKQ)
          // en_spmode bitmap: byte 0 = powerful, 1 = econo, 5 = streamer (Faikout).
          // '3' = available — drives capability gating + the climate special_modes attr.
          if (payloadSize >= 3) _hasPowerful = (payload[2] == '3');
          if (payloadSize >= 4) _hasEcono    = (payload[3] == '3');
          if (payloadSize >= 8) _hasStreamer = (payload[7] == '3');
        } else if (payload[0] == '0' && payload[1] == '4') {
          _diag.FU04 = getHEXformatted(&payload[2], payloadSize - 2);
        }
        return true;
      }

      default: // Unknown G-responses: stash raw hex in _diag for HA graphing
      {
        String hex = getHEXformatted(payload, payloadSize);
        switch (cmd2_in) {
          case 'B': _diag.FB = hex; break;
          case 'G': _diag.FG = hex; break;
          case 'N': _diag.FN = hex; break;
          case 'P': _diag.FP = hex; break;
          case 'Q': _diag.FQ = hex; break;
          case 'R': _diag.FR = hex; break;
          case 'T': _diag.FT = hex; break;
          case 'V': _diag.FV = hex; break;
          default: break;  // unmapped — only logged
        }
        Log.ln(TAG, "G%c raw (%d bytes): %s", cmd2_in, payloadSize, hex.c_str());
        return true;
      }
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
      case 'a': // Outside temperature — some v0 units return a fixed value
      {
        float temp = temp_bytes_to_c10(payload) / 10.0;
        this->currentStatus.outsideTemperature = temp;
        if (_firstOutsideTemp == -999) _firstOutsideTemp = temp;
        else if (temp != _firstOutsideTemp && !_outsideTempChanged) { _outsideTempChanged = true; _rediscoverNeeded = true; }
        return true;
      }
      case 'L': // Fan speed
        this->currentStatus.fanRPM = bytes_to_num(&payload[0], payloadSize) * 10;
        return true;
      case 'D': // RD -> SD -- ON timer. Polled only for capability detection;
        return true;  // value comes from G3 (RD is stale once a timer is cleared)
      case 'E': // RE -> SE -- OFF timer. Same — value sourced from G3, not here.
        return true;
      case 'M': // RM -> SM -- Target louver angle (degrees, diagnostic only)
        return true;
      case 'N': // RN -> SN -- Measured louver angle (degrees)
        this->currentStatus.louverAngle = temp_bytes_to_c10(payload) / 10.0;
        return true;
      case 'X': // RX -> SX -- Real target temperature (adjusted setpoint)
        this->currentStatus.realTargetTemp = temp_bytes_to_c10(payload) / 10.0;
        return true;
      case 'd': // Rd -> Sd -- Compressor frequency (Hz).
      // Some v0 units ACK this command but always return 000 even when the compressor
      // is physically running (coil temp drops to 6°C, fan at 1100+ RPM).
      // We track whether Rd has EVER returned non-zero (_compressorFreqSeen) to decide
      // whether to expose the Compressor Frequency entity in HA. If it never changes,
      // the entity stays hidden to avoid showing misleading "0 Hz" while cooling.
      {
        int freq = (payload[0] - '0') + (payload[1] - '0') * 10 + (payload[2] - '0') * 100;
        this->currentStatus.compressorFrequency = freq;
        if (freq > 0 && !_compressorFreqSeen) { _compressorFreqSeen = true; _rediscoverNeeded = true; }
        if (freq > 0) {
          this->currentStatus.operating = true;
        } else {
          // Fallback: consider operating if power ON, mode active, and fan spinning
          bool powerOn = String(this->currentSettings.power).equalsIgnoreCase("ON");
          bool modeActive = !String(this->currentSettings.mode).equalsIgnoreCase("DISABLED");
          this->currentStatus.operating = powerOn && modeActive && (this->currentStatus.fanRPM > 0);
        }
        return true;
      }
      case 'e': // Re -> Se -- Humidity reading (Faikout: payload is decimal in
      // reverse ASCII order, e.g. "970" = 79% RH; "050" = sensor not present).
      // On FTKD15ZV2S both units return "050" so the sensor is absent. Only
      // store when the reading is plausibly non-placeholder. The humidity field
      // is only exposed to HA when _humiditySensorPresent latches true (either
      // via this command or F9 byte 2).
      {
        int raw = bytes_to_num(&payload[0], payloadSize);
        if (raw != 0 && raw != 50) {  // 50 is Faikout's documented "no sensor" placeholder
          this->currentStatus.humidity = raw;
          _humiditySensorPresent = true;
        }
        return true;
      }
      case 'G':
        if (payloadSize == 1)
        {
          const char* newFan = lookupByteMapValue(S21_FAN_MAP, S21_FAN, 7, payload[0]);
          if (newFan != this->currentSettings.fan) {
            Log.ln(TAG, "Fan speed changed to %s", newFan);
            this->currentSettings.fan = newFan;
          }
          use_RG_fan = true;
          return true;
        }
        return false;


      case 'z': // Rz -> Sz -- Extended status (sub-command in payload[0..1])
        if (payloadSize >= 3 && payload[0] == 'B' && payload[1] == '2') {
          // SzB2: bit 0 = powerful, bit 1 = defrost
          this->currentSettings.powerful = (payload[2] & 0x01) ? S21_POWERFUL_MAP[1] : S21_POWERFUL_MAP[0];
          return true;
        }
        return false;

      case 'K': // RK -> SK -- Indoor fan target RPM × 10
      // Validated 2026-05-20 on FTKD-zv2s: fan=1 -> RK=720, fan=5 -> RK=1050.
      // Tracks the indoor fan setpoint, ~10 RPM ahead of measured RL.
      {
        this->currentStatus.targetFanRPM = bytes_to_num(&payload[0], payloadSize) * 10;
        return true;
      }

      case 'b': // Rb -> Sb -- Compressor load signal (per Faikout; semantics not yet user-validated)
      {
        this->currentStatus.loadSignal = bytes_to_num(&payload[0], payloadSize);
        return true;
      }

      default:
      {
        String hex = getHEXformatted(payload, payloadSize);
        // RA/RB/RF/Rg: single-field reads that appear to mirror F1 (RA=power,
        // RB=mode on FTKD). Exposed as graphable numerics so HA long-term
        // statistics can confirm/refute the duplication over weeks rather than
        // on a single sample. Decode reverse-ASCII decimal; keep raw hex if the
        // payload isn't purely numeric.
        String *slot = (cmd2_in == 'A') ? &_diag.RA : (cmd2_in == 'B') ? &_diag.RB
                     : (cmd2_in == 'F') ? &_diag.RF : (cmd2_in == 'g') ? &_diag.Rg
                     : nullptr;
        if (slot) {
          int16_t v = s21_ascii_decimal(payload, payloadSize);
          *slot = (v < 0) ? hex : String(v);
        }
        Log.ln(TAG, "S%c raw (%d bytes): %s", cmd2_in, payloadSize, hex.c_str());
        return true;
      }
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

    // HVAC Name
    if (cmd == 0xBA && payloadSize >= 20)
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

      if (!pendingSettings.hasPending())
        syncNewSettings();
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
        if (!pendingSettings.hasPending())
          newSettings = currentSettings;
      }
      return true;
    }

    if (cmd == 0xB7 && payloadSize >= 32)
    { // CDU Status
      float t;
      if ((t = (int16_t)(payload[0] + (payload[1] << 8)) / 128.0) && t < 100)
      {
        this->currentStatus.outsideTemperature = round(t * 2.0) / 2.0;
      }
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
    // Fan Speed & Vertical Vane (Flap)
    if (cmd == 0xBE && payloadSize >= 9)
    {
      int rpm = payload[2] + (payload[3] << 8);
      rpm = (rpm / 10) * 10; // round to nearest tenth
      this->currentStatus.fanRPM = rpm;

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

      bool d1ok = daikinUART->sendCommandS21('D', '1', payload, 4);
      if (d1ok) {
        currentSettings.power = newSettings.power;
        currentSettings.mode = newSettings.mode;
        currentSettings.temperature = newSettings.temperature;
        currentSettings.fan = newSettings.fan;
      }
      res = d1ok & res;
      pendingSettings.basic = false;
    }

    if (pendingSettings.vane || updateAll)
    {

      bool hVane = strcmp(HORIZONTALVANE_MAP[1], newSettings.horizontalVane) == 0;
      bool vVane = strcmp(VERTICALVANE_MAP[1], newSettings.verticalVane) == 0;

      payload[0] = ('0' + (hVane ? 2 : 0) + (vVane ? 1 : 0) + (hVane && vVane ? 4 : 0));
      payload[1] = (vVane || hVane ? '?' : '0');
      payload[2] = '0';
      payload[3] = '0';

      bool d5ok = daikinUART->sendCommandS21('D', '5', payload, 4);
      if (d5ok) {
        currentSettings.verticalVane = newSettings.verticalVane;
        currentSettings.horizontalVane = newSettings.horizontalVane;
      }
      res = d5ok & res;
      pendingSettings.vane = false;
    }
    
    // Special modes — three commands packed in this block:
    //   D6 byte 0: powerful (bit 1) + comfort (bit 6) + quiet (bit 7) — v2 units
    //   D6 byte 1: streamer (bit 7)
    //   D3 byte 4: powerful (bit 2) + quiet (bit 1) + comfort (bit 0) — always sent
    //              as an attempt to reach FTKQ, which NAKs D6. NOT WORKING, see below.
    //   D7 byte 1: econo (bit 1)
    if (pendingSettings.specialMode || updateAll)
    {
      bool sent = false;
      if (!(s21SkipMask & (1ULL << S21_QUERY_F6))) {
        uint8_t b0 = '0'
          + S21_POWERFUL[lookupByteMapIndex(S21_POWERFUL_MAP, 2, newSettings.powerful)]
          + S21_COMFORT [lookupByteMapIndex(S21_COMFORT_MAP,  2, newSettings.comfort)]
          + S21_QUIET   [lookupByteMapIndex(S21_QUIET_MAP,    2, newSettings.quiet)];
        uint8_t b1 = '0' + S21_STREAMER[lookupByteMapIndex(S21_STREAMER_MAP, 2, newSettings.streamer)];
        payload[0] = b0;
        payload[1] = b1;
        payload[2] = '0';
        // byte 3 = display brightness. Always written from the staged setting (kept
        // in sync with the unit by the G6 parser) so toggling any special mode
        // preserves the user's brightness instead of forcing it back to High.
        payload[3] = '0' + S21_LED[lookupByteMapIndex(S21_LED_MAP, 3, newSettings.ledBrightness)];
        sent = daikinUART->sendCommandS21('D', '6', payload, 4);
        if (!sent) Log.ln(TAG, "D6 failed, will try D3 fallback");
      }
      // Always send D3, carrying the special modes in byte 4. Preserves current timer state.
      //
      // KNOWN NOT WORKING on FTKQ-UV2S. Live capture (powerful ON, 2026-08-21):
      //     D6 >> 02:44:36:32:30:30:34:40:03   << 15   (explicit NAK — FTKQ rejects D6)
      //     D3 >> 02:44:33:30:FE:FE:30:34:07:03 << 06  (ACK, byte4=0x34 → bit2 powerful)
      // The unit ACKs the 5-byte D3 but ignores byte 4: F6 still reads 30:30:30:30 and
      // the status block still reports "Powerful Cool: OFF". F3 echoes only 4 bytes, so
      // byte 4 is not read back either — the unit likely never stored it.
      //
      // Note docs/S21_PROTOCOL_REFERENCE.md documents powerful on D3 as byte 3 bit 1
      // (verified), not byte 4. The byte-4 layout here is an unconfirmed hypothesis and
      // the capture above is evidence against it. Reaching special modes on FTKQ is
      // still an open problem; do not treat this path as functional.
      {
        int onMin  = currentStatus.onTimerMinutes;
        int offMin = currentStatus.offTimerMinutes;
        uint8_t enable = (onMin > 0 ? 1 : 0) | (offMin > 0 ? 2 : 0);
        payload[0] = '0' + enable;
        payload[1] = (onMin  > 0) ? (uint8_t)(0x30 + onMin  / 10) : 0xFE;
        payload[2] = (offMin > 0) ? (uint8_t)(0x30 + offMin / 10) : 0xFE;
        payload[3] = '0';
        uint8_t sp = 0;
        if (newSettings.powerful == S21_POWERFUL_MAP[1]) sp |= 0x04;
        if (newSettings.quiet    == S21_QUIET_MAP[1])    sp |= 0x02;
        if (newSettings.comfort  == S21_COMFORT_MAP[1])  sp |= 0x01;
        payload[4] = '0' + sp;
        bool d3ok = daikinUART->sendCommandS21('D', '3', payload, 5);
        if (!sent && !d3ok) Log.ln(TAG, "D3 also failed — special modes not supported");
        sent = sent || d3ok;
      }

      // D7: byte 0 = demand control (power limit), byte 1 = econo. Both share this
      // one write, so each byte is populated from current settings to avoid clobbering
      // the other. (independent of D6/D3 path)
      if (!(s21SkipMask & (1ULL << S21_QUERY_F7))) {
        uint8_t d7p[4] = {
          (uint8_t)('0' + S21_DEMAND[lookupByteMapIndex(S21_DEMAND_MAP, 5, newSettings.demandControl)]),
          (uint8_t)('0' + S21_ECONO[lookupByteMapIndex(S21_ECONO_MAP, 2, newSettings.econo)]),
          '0', '0'};
        bool d7ok = daikinUART->sendCommandS21('D', '7', d7p, 4);
        if (d7ok) {
          currentSettings.econo = newSettings.econo;
          currentSettings.demandControl = newSettings.demandControl;
        }
        sent = sent & d7ok;
      }

      if (sent) {
        currentSettings.powerful = newSettings.powerful;
        currentSettings.comfort  = newSettings.comfort;
        currentSettings.quiet    = newSettings.quiet;
        currentSettings.streamer = newSettings.streamer;
        currentSettings.ledBrightness = newSettings.ledBrightness;
      }
      res = res & sent;
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
      bool d2ok = daikinUART->sendCommandS21('D', '2', payload, 4);
      if (d2ok) {
        currentSettings.remoteEnable = newSettings.remoteEnable;  // S21 doesn't report lock state back
      }
      res = d2ok & res;
    }

    // D3 — ON/OFF timer. byte0 enable ('0'..'3'), byte1 on-timer, byte2 off-timer
    // (both 0x30 + 10-min periods, 0xFE = disabled), byte4 = special modes
    // (bit2=powerful, bit1=quiet, bit0=comfort). Timer bytes are ACK'd and verified;
    // byte 4 is the unconfirmed special-modes attempt — see the D3 block above, it has
    // no observed effect on FTKQ-UV2S.
    if (pendingSettings.timer)
    {
      int onMin  = (_desiredOnTimer  >= 0) ? _desiredOnTimer  : currentStatus.onTimerMinutes;
      int offMin = (_desiredOffTimer >= 0) ? _desiredOffTimer : currentStatus.offTimerMinutes;
      uint8_t enable = (onMin > 0 ? 1 : 0) | (offMin > 0 ? 2 : 0);
      payload[0] = '0' + enable;
      payload[1] = (onMin  > 0) ? (uint8_t)(0x30 + onMin  / 10) : 0xFE;
      payload[2] = (offMin > 0) ? (uint8_t)(0x30 + offMin / 10) : 0xFE;
      payload[3] = '0';
      uint8_t sp = 0;
      if (newSettings.powerful == S21_POWERFUL_MAP[1]) sp |= 0x04;
      if (newSettings.quiet    == S21_QUIET_MAP[1])    sp |= 0x02;
      if (newSettings.comfort  == S21_COMFORT_MAP[1])  sp |= 0x01;
      payload[4] = '0' + sp;
      bool d3ok = daikinUART->sendCommandS21('D', '3', payload, 5);
      if (d3ok) {
        currentStatus.onTimerMinutes  = onMin;
        currentStatus.offTimerMinutes = offMin;
      }
      _desiredOnTimer = -1;
      _desiredOffTimer = -1;
      pendingSettings.timer = false;
      res = res & d3ok;
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

// Map an incoming string to a slot in `map` and assign it to `field` if valid and changed.
// Returns true when the field was actually updated (caller should mark its dirty bit then).
// Rejects unknown values rather than silently falling back to map[0] — that fallback was a
// footgun where a typo (e.g. "medium" when "3" was expected) would silently nuke state to
// whatever map[0] is (typically "auto"/"OFF").
static bool assignMapped(const char *&field, const char *const map[], int mapLen, const char *value)
{
  for (int i = 0; i < mapLen; i++)
  {
    if (strcasecmp(map[i], value) == 0)
    {
      if (field == map[i]) return false;
      field = map[i];
      return true;
    }
  }
  Log.ln(TAG, "Unknown setting value '%s' — rejected", value);
  return false;
}

void DaikinController::setPowerSetting(const char *setting)
{
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.power, S21_POWER_MAP, 2, setting)) pendingSettings.basic = true;
  } else if (daikinUART->currentProtocol() == PROTOCOL_X50) {
    if (assignMapped(newSettings.power, X50_POWER_MAP, 2, setting)) pendingSettings.basic = true;
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
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.mode, S21_MODE_MAP, 7, setting)) pendingSettings.basic = true;
  } else if (daikinUART->currentProtocol() == PROTOCOL_X50) {
    if (assignMapped(newSettings.mode, X50_MODE_MAP, 8, setting)) pendingSettings.basic = true;
  }
}

float DaikinController::getTemperature()
{
  return currentSettings.temperature;
}
void DaikinController::setTemperature(float setting)
{
  if (newSettings.temperature == setting) return;
  newSettings.temperature = setting;
  pendingSettings.basic = true;
}

const char *DaikinController::getFanSpeed()
{
  return currentSettings.fan;
}
void DaikinController::setFanSpeed(const char *setting)
{
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.fan, S21_FAN_MAP, 7, setting)) pendingSettings.basic = true;
  } else if (daikinUART->currentProtocol() == PROTOCOL_X50) {
    if (assignMapped(newSettings.fan, X50_FAN_MAP, 7, setting)) pendingSettings.basic = true;
  }
}

const char *DaikinController::getVerticalVaneSetting()
{
  return currentSettings.verticalVane;
}
void DaikinController::setVerticalVaneSetting(const char *setting)
{
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.verticalVane, VERTICALVANE_MAP, 2, setting)) pendingSettings.vane = true;
  } else if (daikinUART->currentProtocol() == PROTOCOL_X50) {
    if (assignMapped(newSettings.verticalVane, X50_VERTICALVANE_MAP, 7, setting)) pendingSettings.basic = true;
  }
}

const char *DaikinController::getHorizontalVaneSetting()
{
  return currentSettings.horizontalVane;
}
void DaikinController::setHorizontalVaneSetting(const char *setting)
{
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.horizontalVane, HORIZONTALVANE_MAP, 2, setting)) pendingSettings.vane = true;
  }
  // X50 does not support horizontal vane control.
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

String DaikinController::getManualUrl()
{
  return this->currentStatus.manualUrl;
}

const char *DaikinController::getPowerfulSetting(){
  return currentSettings.powerful;
}

void DaikinController::setPowerfulSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.powerful, S21_POWERFUL_MAP, 2, setting)) pendingSettings.specialMode = true;
  }
}

const char *DaikinController::getComfortSetting(){
  return currentSettings.comfort;
}

void DaikinController::setComfortSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.comfort, S21_COMFORT_MAP, 2, setting)) pendingSettings.specialMode = true;
  }
}

const char *DaikinController::getQuietSetting()   { return currentSettings.quiet; }
const char *DaikinController::getStreamerSetting(){ return currentSettings.streamer; }
const char *DaikinController::getEconoSetting()   { return currentSettings.econo; }
const char *DaikinController::getDemandControlSetting(){ return currentSettings.demandControl; }

void DaikinController::setQuietSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.quiet, S21_QUIET_MAP, 2, setting)) pendingSettings.specialMode = true;
  }
}
void DaikinController::setStreamerSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.streamer, S21_STREAMER_MAP, 2, setting)) pendingSettings.specialMode = true;
  }
}
void DaikinController::setEconoSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.econo, S21_ECONO_MAP, 2, setting)) pendingSettings.specialMode = true;
  }
}
void DaikinController::setDemandControlSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.demandControl, S21_DEMAND_MAP, 5, setting)) pendingSettings.specialMode = true;
  }
}

const char *DaikinController::getLEDBrightnessSetting(){ return currentSettings.ledBrightness; }
void DaikinController::setLEDBrightnessSetting(const char *setting){
  if (daikinUART->currentProtocol() == PROTOCOL_S21) {
    if (assignMapped(newSettings.ledBrightness, S21_LED_MAP, 3, setting)) pendingSettings.specialMode = true;
  }
}

// Timer setters — minutes, clamped to 0..1440 and snapped to 10-min periods.
// 0 disables that timer. Both share the D3 write so we only flag pending here.
void DaikinController::setOnTimer(int minutes){
  if (daikinUART->currentProtocol() != PROTOCOL_S21) return;
  if (minutes < 0) minutes = 0;
  if (minutes > 720) minutes = 720;   // documented unit max is 12 h
  _desiredOnTimer = (minutes / 10) * 10;
  pendingSettings.timer = true;
}
void DaikinController::setOffTimer(int minutes){
  if (daikinUART->currentProtocol() != PROTOCOL_S21) return;
  if (minutes < 0) minutes = 0;
  if (minutes > 720) minutes = 720;   // documented unit max is 12 h
  _desiredOffTimer = (minutes / 10) * 10;
  pendingSettings.timer = true;
}

// Enable / Disable physical controls from IR Remote / Front button.
// Protocol-agnostic: stages the change; the actual D2 send in update() is gated to S21.
// Callable before connect() (boot path) since it doesn't depend on protocol detection.
void DaikinController::setEnableRemote(bool isEnable){
  if (newSettings.remoteEnable == isEnable) return;
  newSettings.remoteEnable = isEnable;
  pendingSettings.ACconfig = true;
}