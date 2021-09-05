# PicoBroker

## Overview

A small MQTT and MQTT-SN port of IBM's Really Small Message Broker (RSMB).

*This is a work in progress*

### Notes

- using Espressif's ESP-IDF build system and components
- using Platform.io for build (this project should open in Platform.io and all depencencies install)

- RSMB is componentised in ./components/rsmb

## Target Hardware

The intention is to support any ESP32 based hardware.

However at present we are testing only on M5Stack boards.

In addition the existing RSMB code requires an SD Card. This is because the code

- loads a configuration file (which can be removed and options hardcoded)
- loads some debug messages on start (which can probably be removed)
- persists QoS1/2 messages (which is needed for some use-cases but may be optionally removed in future)

To prepare the target SD Card

- format the card to FAT
- copy components/rsmb/Messages.1.3.0.2 to the root of the uSD card
- copy components/rsmb/broker.cfg to the root of the uSD card

## Build

You should be able to build, flash and monitor using Platform.io tools or ESP-IDP idf.py

## Licensing

As RSMB is under Eclipse Public License - v 1.0 this project currently is too.

