# Daikin2MQTT

Control your Daikin Air Conditioner locally with Home Assistant using ESP32. Communicates directly with A/C using Serial Communication via S21/X50A port, replacing the original BRP072C42/BRP072C42-1 adapter from Daikin.

## Features

- Bi-directional control, syncs true A/C status to Home Assistant
- Support for Daikin wall-type A/C (S21 connector) and SkyAir (X50A connector)
- Automatic protocol detection (S21 tested first, then X50)
- Home Assistant MQTT Discovery with climate entity, sensors, switches, and selects
- Powerful Mode exposed as HA climate preset (boost)
- Built-in web UI for configuration and control
- OTA firmware updates (web upload or ArduinoOTA)
- Configurable temperature offsets for inside/outside sensors
- Auto-skips unsupported S21 commands after first NAK
- 8 languages: English, Danish, Spanish, French, Italian, Japanese, Chinese, Thai

## Controls

- **Climate entity**: Power, mode, target temperature, fan speed, vertical/horizontal swing
- **Presets**: Powerful mode (boost)
- **Switches**: LED, beep, powerful mode, IR remote enable/disable
- **Selects**: Vertical vane, horizontal vane

## Sensors

- Room temperature
- Outdoor unit temperature
- Coil temperature
- Fan RPM
- Compressor frequency
- Error code
- Timer status
- Energy meter (model-dependent)
- Uptime

## Configuration

All settings are configurable via the built-in web UI:

- **MQTT**: Host, port, credentials, topic, friendly name
- **WiFi**: SSID, password, hostname, OTA password
- **Unit**: Temperature unit (C/F), min/max temp, temp step, inside/outside temp offset, fan speed levels (3/5), mode support, update interval, LED, beep
- **Others**: HA autodiscovery, availability reporting, debug mode

## Build

Requires [PlatformIO](https://platformio.org/).

```bash
# Build firmware
pio run -e wifikit-serial-esp32-s3

# Run tests
pio test -e native

# Upload via USB
pio run -e wifikit-serial-esp32-s3 --target upload

# Monitor serial
pio device monitor
```

The built firmware is automatically copied to `bin releases/` with a versioned filename.

### OTA Update

1. Open `http://<device-ip>/upgrade` in a browser
2. Upload the `.bin` file from `bin releases/`
3. Device restarts automatically

## Hardware

![wifikitserial](https://github.com/maxmacstn/Daikin2MQTT/blob/cec3a3b90637d39c1544068fdf562466f342510a/hardware-wifikitserial/header.jpg)

This repository is intended to be used with [WiFiKit Serial](https://wifikitserial.magiapp.me) module. The module consists of ESP32-S3, logic level converter, and buck converter.

If you would like to use your own hardware, you need:
- ESP32-S3 board
- Logic level converter (5V to 3.3V)
- Buck converter (14VDC supply to 3.3V)

### Pin Map

| Pin | Function |
|-----|----------|
| GPIO 43 | AC TX |
| GPIO 44 | AC RX |
| GPIO 5 | Power LED |
| GPIO 6 | Activity LED |
| GPIO 0 | Button (boot) |
| GPIO 14 | Buzzer |

## Links

- [ESP32-Faikin](https://github.com/revk/ESP32-Faikin) - Reference implementation
- [S21 Protocol](https://github.com/revk/ESP32-Faikin/blob/main/Manuals/S21.md) - Protocol documentation
- [esphome-daikin-s21](https://github.com/joshbenner/esphome-daikin-s21) - ESPHome implementation
- [Mitsubishi2MQTT](https://github.com/maxmacstn/mitsubishi2MQTT) - Inspiration for this project
