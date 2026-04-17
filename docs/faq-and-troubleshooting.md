# FAQ and Troubleshooting

This page collects the problems and misunderstandings that come up most often when using AsyncTelegram2.

## The bot does not connect to Telegram

Check these points first:

1. the bot token is correct
2. the board has Internet access
3. the TLS client is configured before `begin()`
4. NTP time is synchronized if your TLS stack requires it
5. you are using the correct certificate header for your client type

Correct header choice:

- native secure clients: [tg_certificate.h](../src/tg_certificate.h)
- `SSLClient`: [tg_bearssl_certificate.h](../src/tg_bearssl_certificate.h)

## `begin()` fails but Wi-Fi is connected

This usually means one of these:

- certificate validation failed
- time is invalid
- token is wrong
- the client is not actually ready for HTTPS

Check the serial log and print `getConnectionModeName()` after `begin()`.

## Why does `sendTo()` not reach a user?

Telegram only allows the bot to message a user after that user has started the bot at least once.

If a user never opened the bot and pressed Start, `sendTo()` will not behave as expected.

## Why does `sendToChannel()` fail?

The bot must be added to the channel and usually must be an admin.

Also make sure the channel identifier is correct, for example:

```cpp
const char* channel = "@your_channel_name";
```

## Why am I not receiving any messages?

Common causes:

- `getNewMessage()` is not being called often enough
- `setUpdateTime()` is too large for your use case
- the sketch is blocked elsewhere in `loop()`
- the bot was not started correctly with `begin()`

Keep the main loop responsive. AsyncTelegram2 works best when `loop()` is short and non-blocking.

## What is the difference between `MessageText` and `MessageQuery`?

- `MessageText`: a normal chat message sent by the user
- `MessageQuery`: an inline keyboard callback

For `MessageQuery`, read `callbackQueryData` and then call `endQuery()`.

## When should I use `enableInsecureFallback()`?

Use it only when you are on a supported native secure client and you want a compatibility fallback if normal certificate validation fails.

Do not treat it as the preferred operating mode.

If security matters, fix certificate validation instead of relying on insecure mode.

## Why does insecure fallback not help with `SSLClient`?

Because `SSLClient` is not switched by `setInsecure()` the way native secure clients are.

For `SSLClient`, the correct approach is a custom recovery callback registered with `setConnectionRecoveryCallback()`.

## What is the recommended polling interval?

Good defaults:

- `1000` ms for interactive bots
- `1500-3000` ms for most applications

Going much lower usually adds noise rather than real benefit.

## Markdown or HTML?

Start with HTML unless you specifically want MarkdownV2.

MarkdownV2 is powerful, but it is easier to break because many characters must be escaped.

## How do I update Telegram certificates?

Use the helper tool:

```bash
python tools/pycert_bearssl/pycert_bearssl.py update-telegram
```

It regenerates:

- `tools/pycert_bearssl/ca.crt`
- `src/tg_certificate.h`
- `src/tg_bearssl_certificate.h`

## My board compiles, but TLS still fails at runtime

Compilation success does not prove TLS is configured correctly.

Runtime TLS failures are often caused by:

- missing or outdated certificates
- invalid time
- transport-specific setup errors
- recovery callback logic that never refreshes the client state

Follow the matching example for your transport, then compare your sketch line by line.

## Why does IntelliSense complain about symbols that still compile?

Arduino and PlatformIO sketches sometimes confuse the local editor parser, especially with `.ino` preprocessing and generated include paths.

If the real PlatformIO build succeeds, trust the build first. Then decide whether the editor warning is worth cleaning up separately.

## Which files should I read first if I am new?

1. [Getting Started](getting-started.md)
2. [Connection Options](connection-options.md)
3. [Examples and Patterns](examples-and-patterns.md)

## Where can I find a full API list?

See [API Reference](api-reference.md).