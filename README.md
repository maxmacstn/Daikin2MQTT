# Daikin2MQTT
Control your Daikin Air Conditioner locally with Home Assistant using ESP32. Communicates directly with A/C using Serial Communication via S21/X50A port, replacing the original BRP072C42/BRP072C42-1 adapter from Daikin.

## Features
- Bi-Directional control, sync true A/C status to Home Assistant.
- Support Daikin wall-type A/C (S21 Connector) and SkyAir (X50A Connector).
- Automatic protocol selection.
- Support Home Assistant MQTT Discovery.

## Functions
- Basic Operation: Power, Mode, Temp setpoint, Fan speed, Vertical swing.
- Status Monitor: Room Temp, Outside Temp, FCU Coil Temp, FCU Fan RPM, Compressor Frequency.

## Software
The code in this repository is based on Arduino framework, and implemented on PlatformIO IDE. 

## Hardware
![wifikitserial](https://github.com/maxmacstn/Daikin2MQTT/blob/cec3a3b90637d39c1544068fdf562466f342510a/hardware-wifikitserial/header.jpg)
This repository is intended to be used with [WiFiKit Serial](wifikitserial.magiapp.me) module which I'm selling on [Shopee](https://shopee.co.th/product/27525505/25976826071/). The module consists of ESP32-S3, Logic Level converter, and buck converter.

If you would like to use your own hardware, please include the Logic Level converter, and buck converter to convert 14VDC supply voltage to 3.3V, and 5V logic level to 3V.

## Links
Many thanks for all the work from the links below for a detailed guide and inspiration for building this project.
- [ESP32-Faikin](https://github.com/revk/ESP32-Faikin)
- [S21 Protocol](https://github.com/revk/ESP32-Faikin/blob/main/Manuals/S21.md)
- [esphome-daikin-s21](https://github.com/joshbenner/esphome-daikin-s21)
- [Mitsubishi2MQTT](https://github.com/maxmacstn/mitsubishi2MQTT)
