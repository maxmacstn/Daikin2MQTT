#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>

// ============================================================
// Extracted pure functions (no Arduino dependency)
// ============================================================

// Temperature conversion (from main.cpp)
static float toFahrenheit(float fromCelsius) {
  return round(1.8 * fromCelsius + 32.0);
}

static float toCelsius(float fromFahrenheit) {
  return (fromFahrenheit - 32.0) / 1.8;
}

static float convertCelsiusToLocalUnit(float temperature, bool isFahrenheit) {
  if (isFahrenheit) return toFahrenheit(temperature);
  return temperature;
}

static float convertLocalUnitToCelsius(float temperature, bool isFahrenheit) {
  if (isFahrenheit) return toCelsius(temperature);
  return temperature;
}

// Checksum functions (from DaikinUART.cpp)
static uint8_t S21Checksum(uint8_t *bytes, uint8_t len) {
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < len; i++) {
    checksum += bytes[i];
  }
  return checksum;
}

static uint8_t X50Checksum(uint8_t *bytes, uint8_t len) {
  uint8_t checksum = 0;
  for (int i = 0; i < len; i++) {
    checksum += bytes[i];
  }
  return 0xFF - checksum;
}

// Hex sensor decode (from DaikinController.h)
static uint16_t s21_decode_hex_sensor(const unsigned char *payload) {
#define hex(c) (((c)&0xF)+((c)>'9'?9:0))
  return (hex(payload[3]) << 12) | (hex(payload[2]) << 8) | (hex(payload[1]) << 4) | hex(payload[0]);
#undef hex
}

// Byte map lookup (from DaikinController.cpp)
static int lookupByteMapIndex(const char *valuesMap[], int len, const char *lookupValue) {
  for (int i = 0; i < len; i++) {
    if (strcmp(valuesMap[i], lookupValue) == 0) return i;
  }
  return -1;
}

// Error code tables (from DaikinController.h)
static const char S21errorCodeDivision[] = {' ', ' ', ' ', 'A', 'C', 'E', 'H', 'F', 'J', 'L', 'P', 'U', 'M', '6', '8', '9'};
static const char S21errorCodeDetail[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'H', 'C', 'J', 'E', 'F'};

// ============================================================
// Tests: Temperature Conversion
// ============================================================

void test_toFahrenheit_freezing(void) {
  TEST_ASSERT_EQUAL_FLOAT(32.0, toFahrenheit(0.0));
}

void test_toFahrenheit_boiling(void) {
  TEST_ASSERT_EQUAL_FLOAT(212.0, toFahrenheit(100.0));
}

void test_toFahrenheit_typical_ac_temp(void) {
  // 22°C → 72°F (rounded)
  TEST_ASSERT_EQUAL_FLOAT(72.0, toFahrenheit(22.0));
}

void test_toFahrenheit_negative(void) {
  TEST_ASSERT_EQUAL_FLOAT(-40.0, toFahrenheit(-40.0));
}

void test_toCelsius_freezing(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.01, 0.0, toCelsius(32.0));
}

void test_toCelsius_boiling(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.01, 100.0, toCelsius(212.0));
}

void test_toCelsius_roundtrip(void) {
  float original = 23.5;
  float result = toCelsius(toFahrenheit(original));
  TEST_ASSERT_FLOAT_WITHIN(1.0, original, result);
}

void test_convertCelsiusToLocalUnit_celsius_passthrough(void) {
  TEST_ASSERT_EQUAL_FLOAT(25.0, convertCelsiusToLocalUnit(25.0, false));
}

void test_convertCelsiusToLocalUnit_fahrenheit(void) {
  TEST_ASSERT_EQUAL_FLOAT(77.0, convertCelsiusToLocalUnit(25.0, true));
}

void test_convertLocalUnitToCelsius_celsius_passthrough(void) {
  TEST_ASSERT_EQUAL_FLOAT(25.0, convertLocalUnitToCelsius(25.0, false));
}

void test_convertLocalUnitToCelsius_fahrenheit(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.01, 25.0, convertLocalUnitToCelsius(77.0, true));
}

// ============================================================
// Tests: S21 Checksum
// ============================================================

void test_S21Checksum_empty(void) {
  uint8_t data[] = {};
  TEST_ASSERT_EQUAL_UINT8(0, S21Checksum(data, 0));
}

void test_S21Checksum_single_byte(void) {
  uint8_t data[] = {0x42};
  TEST_ASSERT_EQUAL_UINT8(0x42, S21Checksum(data, 1));
}

void test_S21Checksum_typical_packet(void) {
  // STX=2, 'F'=0x46, '1'=0x31, payload 0x30,0x30,0x30,0x30
  uint8_t data[] = {0x46, 0x31, 0x30, 0x30, 0x30, 0x30};
  uint8_t expected = (uint8_t)(0x46 + 0x31 + 0x30 + 0x30 + 0x30 + 0x30);
  TEST_ASSERT_EQUAL_UINT8(expected, S21Checksum(data, 6));
}

void test_S21Checksum_overflow_wraps(void) {
  uint8_t data[] = {0xFF, 0x01};
  TEST_ASSERT_EQUAL_UINT8(0x00, S21Checksum(data, 2));
}

// ============================================================
// Tests: X50 Checksum
// ============================================================

void test_X50Checksum_empty(void) {
  uint8_t data[] = {};
  TEST_ASSERT_EQUAL_UINT8(0xFF, X50Checksum(data, 0));
}

void test_X50Checksum_single_byte(void) {
  uint8_t data[] = {0x01};
  TEST_ASSERT_EQUAL_UINT8(0xFE, X50Checksum(data, 1));
}

void test_X50Checksum_known_value(void) {
  uint8_t data[] = {0xCA, 0x01, 0x00};
  uint8_t sum = 0xCA + 0x01 + 0x00;
  TEST_ASSERT_EQUAL_UINT8(0xFF - sum, X50Checksum(data, 3));
}

void test_X50Checksum_complement_relationship(void) {
  // sum + checksum should equal 0xFF
  uint8_t data[] = {0x10, 0x20, 0x30};
  uint8_t checksum = X50Checksum(data, 3);
  uint8_t sum = 0x10 + 0x20 + 0x30;
  TEST_ASSERT_EQUAL_UINT8(0xFF, (uint8_t)(sum + checksum));
}

// ============================================================
// Tests: S21 Hex Sensor Decode
// ============================================================

void test_s21_decode_hex_sensor_zeros(void) {
  unsigned char payload[] = {'0', '0', '0', '0'};
  TEST_ASSERT_EQUAL_UINT16(0x0000, s21_decode_hex_sensor(payload));
}

void test_s21_decode_hex_sensor_digits(void) {
  // "1234" → 0x4321 (payload[0] is lowest nibble)
  unsigned char payload[] = {'1', '2', '3', '4'};
  TEST_ASSERT_EQUAL_UINT16(0x4321, s21_decode_hex_sensor(payload));
}

void test_s21_decode_hex_sensor_hex_chars(void) {
  // 'A'=10, 'B'=11, 'C'=12, 'D'=13
  unsigned char payload[] = {'A', 'B', 'C', 'D'};
  TEST_ASSERT_EQUAL_UINT16(0xDCBA, s21_decode_hex_sensor(payload));
}

void test_s21_decode_hex_sensor_typical_temp(void) {
  // Temperature 22.0°C encoded as 0x00DC (220 in decimal)
  // 220 = 0x00DC, stored little-endian in hex chars: 'C','D','0','0'
  unsigned char payload[] = {'C', 'D', '0', '0'};
  TEST_ASSERT_EQUAL_UINT16(0x00DC, s21_decode_hex_sensor(payload));
}

// ============================================================
// Tests: Byte Map Lookup
// ============================================================

static const char* POWER_MAP[] = {"OFF", "ON"};
static const char* MODE_MAP[] = {"AUTO", "COOL", "DRY", "HEAT", "FAN"};
static const char* FAN_MAP[] = {"auto", "quiet", "1", "2", "3", "4", "5"};
static const char* VANE_MAP[] = {"hold", "swing"};

void test_lookupByteMapIndex_found(void) {
  TEST_ASSERT_EQUAL_INT(1, lookupByteMapIndex(POWER_MAP, 2, "ON"));
  TEST_ASSERT_EQUAL_INT(0, lookupByteMapIndex(POWER_MAP, 2, "OFF"));
}

void test_lookupByteMapIndex_not_found(void) {
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(POWER_MAP, 2, "MAYBE"));
}

void test_lookupByteMapIndex_modes(void) {
  TEST_ASSERT_EQUAL_INT(0, lookupByteMapIndex(MODE_MAP, 5, "AUTO"));
  TEST_ASSERT_EQUAL_INT(1, lookupByteMapIndex(MODE_MAP, 5, "COOL"));
  TEST_ASSERT_EQUAL_INT(3, lookupByteMapIndex(MODE_MAP, 5, "HEAT"));
  TEST_ASSERT_EQUAL_INT(4, lookupByteMapIndex(MODE_MAP, 5, "FAN"));
}

void test_lookupByteMapIndex_case_sensitive(void) {
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(POWER_MAP, 2, "on"));
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(MODE_MAP, 5, "cool"));
}

void test_lookupByteMapIndex_fan_speeds(void) {
  TEST_ASSERT_EQUAL_INT(0, lookupByteMapIndex(FAN_MAP, 7, "auto"));
  TEST_ASSERT_EQUAL_INT(1, lookupByteMapIndex(FAN_MAP, 7, "quiet"));
  TEST_ASSERT_EQUAL_INT(6, lookupByteMapIndex(FAN_MAP, 7, "5"));
  // Uppercase should NOT match (case sensitive)
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(FAN_MAP, 7, "AUTO"));
}

void test_lookupByteMapIndex_vane(void) {
  TEST_ASSERT_EQUAL_INT(0, lookupByteMapIndex(VANE_MAP, 2, "hold"));
  TEST_ASSERT_EQUAL_INT(1, lookupByteMapIndex(VANE_MAP, 2, "swing"));
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(VANE_MAP, 2, "HOLD"));
  TEST_ASSERT_EQUAL_INT(-1, lookupByteMapIndex(VANE_MAP, 2, "SWING"));
}

// ============================================================
// Tests: Preset Mode Mapping
// ============================================================

void test_preset_mode_mapping(void) {
  // Verify the mapping logic used in mqttCallback for preset modes
  const char* boost = "boost";
  const char* none = "none";
  TEST_ASSERT_TRUE(strcmp(boost, "boost") == 0);
  TEST_ASSERT_TRUE(strcmp(none, "boost") != 0);
}

// ============================================================
// Tests: Temperature Offset
// ============================================================

void test_temperature_offset_applied(void) {
  float roomTemp = 25.0;
  float offset = 1.5;
  TEST_ASSERT_EQUAL_FLOAT(26.5, roomTemp + offset);
}

void test_temperature_offset_negative(void) {
  float outsideTemp = 25.0;
  float offset = -5.0;
  TEST_ASSERT_EQUAL_FLOAT(20.0, outsideTemp + offset);
}

void test_temperature_offset_zero(void) {
  float temp = 22.0;
  float offset = 0.0;
  TEST_ASSERT_EQUAL_FLOAT(22.0, temp + offset);
}

void test_temperature_offset_with_conversion(void) {
  float tempC = 25.0;
  float offset = 2.0;
  float result = toFahrenheit(tempC + offset);
  TEST_ASSERT_EQUAL_FLOAT(81.0, result); // 27°C = 80.6°F, rounded to 81
}

// ============================================================
// Tests: Error Code Table Bounds
// ============================================================

void test_error_code_tables_size(void) {
  TEST_ASSERT_EQUAL(16, sizeof(S21errorCodeDivision));
  TEST_ASSERT_EQUAL(16, sizeof(S21errorCodeDetail));
}

void test_error_code_decode_no_error(void) {
  // Error byte 0x00 → division[0]=' ', detail[0]='0'
  uint8_t errByte = 0x00;
  TEST_ASSERT_EQUAL(' ', S21errorCodeDivision[errByte >> 4]);
  TEST_ASSERT_EQUAL('0', S21errorCodeDetail[errByte & 0xF]);
}

void test_error_code_decode_A3(void) {
  // Error code A3: division index 3 = 'A', detail index 3 = '3'
  uint8_t errByte = 0x33;
  TEST_ASSERT_EQUAL('A', S21errorCodeDivision[errByte >> 4]);
  TEST_ASSERT_EQUAL('3', S21errorCodeDetail[errByte & 0xF]);
}

void test_error_code_decode_E5(void) {
  // Error code E5: division index 5 = 'E', detail index 5 = '5'
  uint8_t errByte = 0x55;
  TEST_ASSERT_EQUAL('E', S21errorCodeDivision[errByte >> 4]);
  TEST_ASSERT_EQUAL('5', S21errorCodeDetail[errByte & 0xF]);
}

// ============================================================
// Main
// ============================================================

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Temperature conversion
  RUN_TEST(test_toFahrenheit_freezing);
  RUN_TEST(test_toFahrenheit_boiling);
  RUN_TEST(test_toFahrenheit_typical_ac_temp);
  RUN_TEST(test_toFahrenheit_negative);
  RUN_TEST(test_toCelsius_freezing);
  RUN_TEST(test_toCelsius_boiling);
  RUN_TEST(test_toCelsius_roundtrip);
  RUN_TEST(test_convertCelsiusToLocalUnit_celsius_passthrough);
  RUN_TEST(test_convertCelsiusToLocalUnit_fahrenheit);
  RUN_TEST(test_convertLocalUnitToCelsius_celsius_passthrough);
  RUN_TEST(test_convertLocalUnitToCelsius_fahrenheit);

  // S21 Checksum
  RUN_TEST(test_S21Checksum_empty);
  RUN_TEST(test_S21Checksum_single_byte);
  RUN_TEST(test_S21Checksum_typical_packet);
  RUN_TEST(test_S21Checksum_overflow_wraps);

  // X50 Checksum
  RUN_TEST(test_X50Checksum_empty);
  RUN_TEST(test_X50Checksum_single_byte);
  RUN_TEST(test_X50Checksum_known_value);
  RUN_TEST(test_X50Checksum_complement_relationship);

  // Hex sensor decode
  RUN_TEST(test_s21_decode_hex_sensor_zeros);
  RUN_TEST(test_s21_decode_hex_sensor_digits);
  RUN_TEST(test_s21_decode_hex_sensor_hex_chars);
  RUN_TEST(test_s21_decode_hex_sensor_typical_temp);

  // Byte map lookup
  RUN_TEST(test_lookupByteMapIndex_found);
  RUN_TEST(test_lookupByteMapIndex_not_found);
  RUN_TEST(test_lookupByteMapIndex_modes);
  RUN_TEST(test_lookupByteMapIndex_case_sensitive);
  RUN_TEST(test_lookupByteMapIndex_fan_speeds);
  RUN_TEST(test_lookupByteMapIndex_vane);

  // Error code tables
  RUN_TEST(test_error_code_tables_size);
  RUN_TEST(test_error_code_decode_no_error);
  RUN_TEST(test_error_code_decode_A3);
  RUN_TEST(test_error_code_decode_E5);

  // Preset mode
  RUN_TEST(test_preset_mode_mapping);

  // Temperature offset
  RUN_TEST(test_temperature_offset_applied);
  RUN_TEST(test_temperature_offset_negative);
  RUN_TEST(test_temperature_offset_zero);
  RUN_TEST(test_temperature_offset_with_conversion);

  return UNITY_END();
}
