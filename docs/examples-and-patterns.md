# Examples and Patterns

This guide maps the examples in the repository to real use cases, so you can start from the closest working sketch instead of reading the whole library API first.

## Best First Examples

### Native secure client: `echoBot`

Use [pio_examples/echoBot](../pio_examples/echoBot) when you are on:

- ESP32 with `WiFiClientSecure`
- ESP8266 with `BearSSL::WiFiClientSecure`

What it shows:

- basic Wi-Fi connection
- TLS setup with `telegram_cert`
- polling with `getNewMessage()`
- text reply flow
- optional insecure fallback
- startup message with connection mode information

### External TLS stack: `echoBot_SSLClient`

Use [pio_examples/echoBot_SSLClient](../pio_examples/echoBot_SSLClient) when you are using `SSLClient`.

What it shows:

- `tg_bearssl_certificate.h` usage
- `SSLClient` construction with trust anchors
- NTP validation before TLS verification time setup
- custom recovery callback with `setConnectionRecoveryCallback()`

## Examples by Topic

### Basic text bot

- [examples/echoBot](../examples/echoBot)
- [pio_examples/echoBot](../pio_examples/echoBot)

Use when you want to:

- receive text messages
- echo or parse simple commands
- forward messages to a channel or a known user

### Inline and reply keyboards

- [examples/keyboards](../examples/keyboards)
- [examples/keyboardCallback](../examples/keyboardCallback)

Use when you want to:

- send interactive menus
- receive callback queries
- request contact or location data from the user

### Light or relay control

- [examples/lightBot](../examples/lightBot)

Use when you want a simple command-and-control bot.

### Text formatting

- [examples/formatStyle](../examples/formatStyle)

Use when you want to send styled messages with HTML or Markdown.

### Photo sending

- [examples/sendPhoto](../examples/sendPhoto)

Use when you want to:

- send an image by URL
- send an image from filesystem
- send raw image data

### Document sending

- [examples/sendDocument](../examples/sendDocument)

Use when you want to upload logs, reports, JSON files, CSV files, and other attachments.

### Message delivery callback

- [examples/sendMessageCallback](../examples/sendMessageCallback)

Use when your application needs confirmation that a message was really sent.

### Event-driven notifications

- [examples/sendOnEvent](../examples/sendOnEvent)

Use when Telegram is used as a notification channel for sensor or button events.

### Camera integrations

- [examples/ESP32/ESP32-CAM](../examples/ESP32/ESP32-CAM)
- [examples/ESP32/ESP32-CAM-PIR](../examples/ESP32/ESP32-CAM-PIR)

Use when you want image capture or motion-triggered photo delivery.

### Ethernet and non-WiFi transport

- [examples/Ethernet](../examples/Ethernet)
- [pio_examples/echoBot_SSLClient](../pio_examples/echoBot_SSLClient)

Use when your board or transport needs `SSLClient` instead of native Wi-Fi TLS.

### Advanced scheduling and OTA patterns

- [examples/advanced/EventScheduler](../examples/advanced/EventScheduler)
- [examples/advanced/multipleTimers](../examples/advanced/multipleTimers)

Use when you need:

- periodic tasks
- timed notifications
- more structured application flow around the bot

## Choosing the Right Starting Point

### If your bot only replies to commands

Start from [pio_examples/echoBot](../pio_examples/echoBot).

### If your transport is not native Wi-Fi TLS

Start from [pio_examples/echoBot_SSLClient](../pio_examples/echoBot_SSLClient).

### If users must choose from buttons

Start from [examples/keyboards](../examples/keyboards).

### If you need to send photos or files

Start from [examples/sendPhoto](../examples/sendPhoto) or [examples/sendDocument](../examples/sendDocument).

### If you need an application pattern, not just a method example

Start from [examples/lightBot](../examples/lightBot) or [examples/advanced/EventScheduler](../examples/advanced/EventScheduler).

## Practical Sketch Pattern

Most production sketches using AsyncTelegram2 look like this:

1. connect the transport
2. configure TLS and time
3. initialize the bot in `setup()`
4. keep `loop()` short
5. poll for messages frequently
6. route text, query, and media messages explicitly by `messageType`

If you are unsure where to start, copy the smallest example that matches your transport and then add one feature at a time.