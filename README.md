# AsyncTelegram2

Thi library is the successor to the AsyncTelegram library.
It was chosen to change the name in addition to the major version to mark clearly a breaking point.
AsyncTelegram2 is now no longer tied to the Espressif ESP8266 / ESP32 hardware, but can virtually work with any MCU capable of a SSL connection and with any kind of transport layer like WiFi, Ethernet, GSM module (still to be tested).

When dealing with IoT systems, connection security is often an underestimated issue.
Due to this reason in AsyncTelegram2 insecure connections or verification method based on the server fingerprint (not strong secure, and moreover with short validity, typically 1 year) are no longer supported.

___
### Introduction
AsyncTelegram2 is an Arduino class for managing Telegram Bot on embedded system.
Don't you know Telegram bots and how to setup one? Check [this](https://core.telegram.org/bots#6-botfather).

When you add the possibility to send a message from your IoT application to a Telegram Bot, this should be only an additional "features" and not the core of your firmware.
Unfortunately, most of Telegram libraries, stucks your micro while communicating with the Telegram Server in order to read properly the response and parse it.

AsyncTelegram2 do this job in async way and not interfee with the rest of code!

It relies on [ArduinoJson](https://github.com/bblanchon/ArduinoJson) v6 library so, in order to use a AsyncTelegram2 object, you need to install the ArduinoJson library first.

With Ethernet adapter, but not only, i wold suggest to use this library for secure connection [SSLClient](https://github.com/OPEnSLab-OSU/SSLClient), a BearSSL based library wich is very light and fast.

+ **this library work with ArduinoJson library V6.x _**

### Features
+ Send and receive non-blocking messages to Telegram bot ([formatting HTML or Markdown V2 supported](https://core.telegram.org/bots/api#formatting-options), disable notification mode supported)
+ Send picture both from url and from any kind of filesystem (SPIFFS, LittleFS, FFAT, SD etc etc )
+ Inline keyboards (with support for callback function)
+ Reply keyboards 
+ Receive localization messages
+ Receive contacts messages 
+ Remote OTA updates (examples only for Espressif's MCU)
+ Forward any kind of message to user or group

### To do
+ Send documents (partially done with pictures)

### Supported boards
The library works virtually with any kind of board capable of SSL connection.

### Simple and responsive usage
Take a look at the examples provided in the [examples folder](https://github.com/cotestatnt/AsyncTelegram2/tree/master/examples).

### Reference
[Here how to use the library](https://github.com/cotestatnt/AsyncTelegram2/blob/master/REFERENCE.md). 

+ 2.0.0   Initial version, most of functionality of AsyncTelegram
+ 2.0.1   Added more overload functions to cover a lot of possible usage and mantain a little backward compatibility
+ 2.0.2   Increased upload speed. Now is possible send photo directly from a raw data buffer (e.g. ESP32-CAM)
+ 2.0.3   Added support for [custom commands](https://core.telegram.org/bots#commands) (`getMyCommands()`, `setMyCommands()`, `deleteMyCommands()`)
