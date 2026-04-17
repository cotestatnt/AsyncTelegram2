# AsyncTelegram2 Documentation

This folder contains the human-written Markdown documentation for AsyncTelegram2.

The generated Doxygen output in [docs/html](html), [docs/latex](latex), and [docs/man](man) is kept in the repository, but the Markdown guides below are the recommended starting point.

## Start Here

- [Getting Started](getting-started.md)
- [Connection Options](connection-options.md)
- [Keyboards and Interactions](keyboards-and-interactions.md)
- [API Reference](api-reference.md)
- [Examples and Patterns](examples-and-patterns.md)
- [FAQ and Troubleshooting](faq-and-troubleshooting.md)

## Suggested Reading Order

If you are new to the library:

1. Read [Getting Started](getting-started.md)
2. Choose your TLS/client setup in [Connection Options](connection-options.md)
3. Use [Examples and Patterns](examples-and-patterns.md) as a starting template

If you already have a working sketch:

1. Use [API Reference](api-reference.md) for method lookup
2. Read [Keyboards and Interactions](keyboards-and-interactions.md) for inline/reply keyboard usage
3. Read [FAQ and Troubleshooting](faq-and-troubleshooting.md) when something is unclear or not working

## What This Documentation Covers

- supported connection patterns for ESP32, ESP8266, SSLClient, WiFiNINA, and similar transports
- polling and message flow using `getNewMessage()`
- sending text, media, documents, and channel messages
- inline keyboards, reply keyboards, callback handling, and message editing
- connection recovery, TLS certificate validation, insecure fallback, and custom recovery callbacks
- common mistakes that frequently appear in repository issues

## Legacy Documentation

- [README.md](../README.md): project overview
- [REFERENCE.md](../REFERENCE.md): older reference file kept for compatibility

The Markdown guides in this folder should be considered the up-to-date documentation set.