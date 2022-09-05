# PicoBroker

## Overview

A small MQTT and MQTT-SN port of IBM's Really Small Message Broker (RSMB).

Please use the ```--recurse-submodules``` argument to obtain RSMB.

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
- copy components/rsmb/rsmb/src/Messages.1.3.0.2 to the root of the uSD card
- copy a broker.cfg file to the root of the uSD card

e.g.

```
# sample configuration on port 1883 with retained persistence in /tmp
port 1883
max_inflight_messages 50
max_queued_messages 200
persistence_location /sdcard/tmp/
retained_persistence true
max_queued_messages 1000
allow_anonymous true

# bridge to one of two possible brokers
#connection remote
#       addresses 144.76.167.54:1883
#       addresses 127.0.0.1:1883
#       topic #
```

## Build

You should be able to build, flash and monitor using Platform.io tools or ESP-IDP idf.py

## Licensing

The RSMB component is licensed under Eclipse Public License - v 1.0

Unless otherwise indicated all other PicoBroker components are licensed until GPLv3

