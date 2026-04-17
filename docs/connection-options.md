# Connection Options

AsyncTelegram2 accepts a generic Arduino `Client`, but Telegram requires HTTPS, so in practice you must provide a transport that can perform TLS correctly.

This is the area that causes most setup problems. This guide explains which certificate file to use, when to enable recovery APIs, and what changes depending on the client type.

## Telegram Endpoints Used by the Library

- host: `api.telegram.org`
- port: `443`

## Certificate Files in This Repository

There are now two separate certificate headers:

- [src/tg_certificate.h](../src/tg_certificate.h): PEM certificate used by native secure clients such as `WiFiClientSecure` and `BearSSL::WiFiClientSecure`
- [src/tg_bearssl_certificate.h](../src/tg_bearssl_certificate.h): BearSSL trust anchors used by `SSLClient`

This split is intentional:

- native client examples only need `telegram_cert`
- `SSLClient` examples need `TAs` and `TAs_NUM`

## Supported Connection Patterns

### Native ESP32 client

Typical setup:

```cpp
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>

WiFiClientSecure client;
AsyncTelegram2 bot(client);

client.setCACert(telegram_cert);
```

Use this when you rely on the secure client provided by the ESP32 core.

### Native ESP8266 client

Typical setup:

```cpp
#include <ESP8266WiFi.h>
#include <AsyncTelegram2.h>

BearSSL::WiFiClientSecure client;
BearSSL::Session session;
BearSSL::X509List certificate(telegram_cert);

client.setSession(&session);
client.setTrustAnchors(&certificate);
client.setBufferSizes(1024, 1024);
```

ESP8266 usually needs a valid system time to verify certificates correctly.

### SSLClient for Ethernet or other transports

Typical setup:

```cpp
#include <WiFiClient.h>
#include <SSLClient.h>
#include "tg_bearssl_certificate.h"
#include <AsyncTelegram2.h>

WiFiClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
AsyncTelegram2 bot(client);
```

Use this when the board or transport does not provide a suitable native TLS client, for example:

- Ethernet
- some WiFiNINA-based setups
- custom transports that expose a plain `Client`

## Time Synchronization

TLS certificate validation often fails if the board time is wrong.

This is especially important for:

- ESP8266 native BearSSL
- SSLClient setups where you call `setVerificationTime()`

Recommended pattern:

1. connect network first
2. sync time with NTP
3. configure certificate validation
4. call `begin()`

## Built-in Connection Recovery APIs

AsyncTelegram2 now supports two recovery mechanisms.

### 1. Insecure fallback for supported native secure clients

Use:

```cpp
bot.enableInsecureFallback();
```

What it does:

- first tries normal certificate validation
- if the TLS connection fails and the client supports `setInsecure()`, retries in insecure mode

This is only available for supported native secure client types.

Important:

- it is a compatibility fallback, not the preferred mode
- when it is used, certificate validation is no longer protecting the connection

You can inspect the result with:

- `isUsingInsecureConnection()`
- `getConnectionMode()`
- `getConnectionModeName()`

### 2. Custom recovery callback for external TLS stacks

Use:

```cpp
bot.setConnectionRecoveryCallback(recoverTelegramConnection);
```

This is the recommended pattern for `SSLClient` and similar transports.

The callback signature is:

```cpp
bool recoverTelegramConnection(Client &client, const char *host, uint16_t port)
```

The callback is invoked after a failed connection attempt and before the library gives up.

Use cases:

- refresh SSLClient verification time
- restore trust anchors or transport state
- reinitialize parts of the network stack

If the callback returns `true`, the library retries the Telegram connection.

## Connection Modes

The library exposes the active connection mode through:

- `getConnectionMode()`
- `getConnectionModeName()`

Possible values:

- `ConnectionModeCertificateValidation`
- `ConnectionModeInsecureFallback`
- `ConnectionModeCustomRecovery`

This is useful for startup logs or a status message sent by the bot when it comes online.

## Updating Certificates

Use the Python tool in [tools/pycert_bearssl](../tools/pycert_bearssl) to refresh the Telegram certificate files.

Command:

```bash
python pycert_bearssl.py update-telegram
```

Generated outputs:

- `tools/pycert_bearssl/ca.crt`
- `src/tg_certificate.h`
- `src/tg_bearssl_certificate.h`

## Which Example Should I Follow?

- use [pio_examples/echoBot](../pio_examples/echoBot) if you are on ESP32 or ESP8266 with native secure clients
- use [pio_examples/echoBot_SSLClient](../pio_examples/echoBot_SSLClient) if you are using `SSLClient`
- use [examples/Ethernet](../examples/Ethernet) as a starting point for non-WiFi transports

## Common Mistakes

- calling `begin()` before setting the token
- forgetting NTP time sync before certificate validation
- using `tg_certificate.h` where `tg_bearssl_certificate.h` is required
- expecting insecure fallback to work with `SSLClient`
- using `sendTo()` with a user that has never started the bot