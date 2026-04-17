# Getting Started

This guide shows the minimum steps required to get an AsyncTelegram2 bot running and explains the basic runtime model of the library.

## What AsyncTelegram2 Does

AsyncTelegram2 is a Telegram Bot library for Arduino-compatible boards.

It is designed around a polling model:

- your sketch owns the network client
- AsyncTelegram2 uses that client to talk to Telegram
- your `loop()` keeps running normally while the library checks for messages and sends replies

The library works with different transports as long as you provide a `Client` implementation capable of reaching Telegram over HTTPS.

## Main Requirements

- ArduinoJson installed
- a valid Telegram bot token created with BotFather
- a secure client configuration that can connect to `api.telegram.org`

## Minimal Workflow

Every sketch follows the same high-level flow:

1. create and configure the transport client
2. instantiate `AsyncTelegram2`
3. set the bot token
4. optionally set polling time and formatting style
5. call `begin()` once in `setup()`
6. call `getNewMessage()` repeatedly in `loop()`

## Minimal Example

```cpp
#include <AsyncTelegram2.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
  WiFiClientSecure client;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session session;
  BearSSL::X509List certificate(telegram_cert);
#endif

AsyncTelegram2 bot(client);

const char *ssid = "your-ssid";
const char *pass = "your-password";
const char *token = "123456:ABCDEF";

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

#if defined(ESP8266)
  configTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
  client.setCACert(telegram_cert);
#endif

  bot.setTelegramToken(token);
  bot.setUpdateTime(1000);
  bot.begin();
}

void loop() {
  TBMessage msg;
  if (bot.getNewMessage(msg)) {
    if (msg.messageType == MessageText) {
      bot.sendMessage(msg, msg.text);
    }
  }
}
```

For a complete runnable sketch, see [pio_examples/echoBot](../pio_examples/echoBot) and [examples/echoBot](../examples/echoBot).

## Core Concepts

### The transport client is your responsibility

AsyncTelegram2 does not create the network client for you.

That means you are responsible for:

- Wi-Fi or Ethernet connection
- TLS certificates or trust anchors
- NTP time sync when required by the TLS stack

This is intentional. It makes the library usable with different boards and transports.

### `begin()` is not optional

After configuring the client and setting the token, call `begin()`.

`begin()` verifies that the bot can connect to Telegram and loads initial bot information such as the bot username.

### `getNewMessage()` is the main receive API

Most bots spend their time inside a loop that repeatedly calls:

```cpp
TBMessage msg;
if (bot.getNewMessage(msg)) {
  // handle message
}
```

The returned `TBMessage` contains the message type and any associated payload.

### Polling interval matters

Use `setUpdateTime()` to control how often the library polls Telegram.

Recommended starting values:

- `1000` ms for responsive bots
- `1500-3000` ms for general-purpose bots
- higher values if you want less network activity

Very low intervals are rarely useful and can waste CPU time and bandwidth.

## First APIs You Will Usually Need

- `setTelegramToken()`
- `setUpdateTime()`
- `begin()`
- `getNewMessage()`
- `sendMessage()`
- `sendTo()`
- `sendToChannel()`

## Formatting Options

Use `setFormattingStyle()` to choose the default text formatting mode:

- `AsyncTelegram2::NONE`
- `AsyncTelegram2::HTML`
- `AsyncTelegram2::MARKDOWN`

HTML is often the easiest starting point because Telegram MarkdownV2 requires more escaping.

## Where To Go Next

- Read [Connection Options](connection-options.md) if you are unsure how to configure TLS on your board
- Read [API Reference](api-reference.md) for a full method reference
- Read [FAQ and Troubleshooting](faq-and-troubleshooting.md) if `begin()` fails or the bot does not receive messages