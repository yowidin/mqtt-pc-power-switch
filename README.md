# MQTT PC Power Switch [![Build Status](https://travis-ci.org/yowidin/mqtt-pc-power-switch.svg?branch=master)](https://travis-ci.org/yowidin/mqtt-pc-power-switch)
A simple MQTT power switch for ATX-Based computers. It is developed mainly for the [NodeMCU v2](https://en.wikipedia.org/wiki/NodeMCU) board.
[PlatformIO](http://platformio.org) is used as a development environment.

# Usage
- [Install](http://platformio.org/get-started) the PlatformIO CLI or IDE
- Clone this repository
- Build it with `platformio run`
- Upload it with `platformio run -t upload`.

After these steps you should be able to see the `ESP-PC` access point, connect to it and configure WiFi settings for the board.
NOTE: This project relies on [ESP8266 WiFiManager Library](https://github.com/tzapu/WiFiManager) for WiFi configuration, you don't  have to specify your credentials anywhere in source code.

# HomeKit Accessory Server
This repository contains an accessory description file for the [HomeKit Accessory Server](https://github.com/KhaosT/HAP-NodeJS).
Simply copy [PowerSwitch_accessory.js](extra/PowerSwitch_accessory.js) into `HAP-NodeJS/accessories` and restart the Server.

# Schematic
![Schematic diagram](https://raw.githubusercontent.com/yowidin/mqtt-pc-power-switch/master/doc/schematic.png)
