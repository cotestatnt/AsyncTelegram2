# pycert_bearssl

`pycert_bearssl` converts trusted certificates into the BearSSL trust-anchor format used by `SSLClient` and the `AsyncTelegram2` examples.

## Requirements

- Python 3.9 or newer
- `click`
- `pyOpenSSL`
- `certifi`

Install the dependencies with:

```bash
python -m pip install click pyOpenSSL certifi
```

or:

```bash
python -m pip install -r requirements.txt
```

## Telegram command

Use the dedicated Telegram command to resolve the current trust anchor for `api.telegram.org`, save it as a `.crt` file, and generate separate headers for the native client path and the `SSLClient` examples:

```bash
python pycert_bearssl.py update-telegram
```

By default the command writes:

- `tools/pycert_bearssl/ca.crt`
- `src/tg_certificate.h`
- `src/tg_bearssl_certificate.h`

If one or both files already exist, the script asks for confirmation before overwriting them.

Use `-y` to skip the confirmation prompt:

```bash
python pycert_bearssl.py update-telegram -y
```

You can also override the output paths:

```bash
python pycert_bearssl.py update-telegram \
  --crt-output ./telegram-ca.crt \
  --header-output ../../src/tg_certificate.h \
  --bearssl-header-output ../../src/tg_bearssl_certificate.h
```

## Notes

- The generated `.crt` file is the trusted root CA matched to Telegram's certificate chain, not the leaf server certificate.
- `tg_certificate.h` contains only the PEM certificate for the native ESP32/ESP8266 secure client path.
- `tg_bearssl_certificate.h` contains the BearSSL trust-anchor block for `SSLClient` examples.

## Other commands

The original generic commands are still available:

- `download`: fetch the trust anchor for one or more domains and generate a BearSSL header.
- `convert`: convert one or more PEM files into a BearSSL header.