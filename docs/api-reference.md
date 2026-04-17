# API Reference

This file is a practical reference for the public API of AsyncTelegram2 and the main helper types used by the library.

## AsyncTelegram2 Class

Header: [src/AsyncTelegram2.h](../src/AsyncTelegram2.h)

### Constructors

```cpp
AsyncTelegram2(Client &client, uint32_t bufferSize = BUFFER_BIG);
AsyncTelegram2(TelegramSecureClient &client, uint32_t bufferSize = BUFFER_BIG);
```

Use the generic `Client` constructor for external transports such as `SSLClient`.

The secure-client overload is available for supported native ESP32 and ESP8266 secure clients and enables built-in insecure fallback support.

### Connection and Lifecycle

#### `bool begin()`

Starts the initial Telegram connection and loads bot information.

Call this after:

- network setup
- TLS setup
- `setTelegramToken()`

#### `bool reset()`

Resets the connection state when the connection was lost.

#### `bool checkConnection()`

Checks whether the connection is active and reconnects if required.

Usually you do not need to call this directly because message send/receive paths use it internally.

### Basic Configuration

#### `setTelegramToken(const char *token)`

Sets the bot token.

#### `setUpdateTime(uint32_t pollingTime)`

Sets the polling interval in milliseconds.

#### `setFormattingStyle(uint8_t format)`

Sets the default text formatting mode.

Available enum values:

- `AsyncTelegram2::NONE`
- `AsyncTelegram2::HTML`
- `AsyncTelegram2::MARKDOWN`

#### `setJsonBufferSize(uint32_t jsonBufferSize)`

Changes the JSON buffer size used by the parser.

Useful when a command returns larger-than-usual JSON payloads.

Related constants from [src/DataStructures.h](../src/DataStructures.h):

- `BUFFER_SMALL`
- `BUFFER_MEDIUM`
- `BUFFER_BIG`

### Connection Recovery and TLS Mode

#### `enableInsecureFallback(bool enable = true)`

Enables a retry path that switches a supported native secure client to insecure mode if the first TLS handshake fails.

#### `isInsecureFallbackEnabled()`

Returns whether built-in insecure fallback is enabled.

#### `isUsingInsecureConnection()`

Returns `true` when the current working connection is using the insecure fallback.

#### `setConnectionRecoveryCallback(ConnectionRecoveryCallback callback)`

Registers a custom recovery callback invoked after a failed connection attempt.

#### `isUsingCustomConnectionRecovery()`

Returns `true` when the last successful connection used the custom recovery path.

#### `getConnectionMode()`

Returns the current `ConnectionMode` enum value.

#### `getConnectionModeName()`

Returns a printable string describing the active connection mode.

### Receiving Messages

#### `MessageType getNewMessage(TBMessage &message)`

Fetches the first unread Telegram message and stores it in `message`.

Possible `MessageType` values are defined in [src/DataStructures.h](../src/DataStructures.h):

- `MessageNoData`
- `MessageText`
- `MessageQuery`
- `MessageLocation`
- `MessageContact`
- `MessageDocument`
- `MessageReply`
- `MessageNewMember`
- `MessageLeftMember`
- `MessageForwarded`

#### `bool noNewMessage()`

Returns `true` if there are no more unread messages to process.

### Sending Messages

#### Core text message API

```cpp
bool sendMessage(const TBMessage &msg, const char *message, char *keyboard = nullptr, bool wait = false);
```

Overloads exist for:

- `String`
- `InlineKeyboard &`
- `ReplyKeyboard &`

#### `sendTo(int64_t userid, ...)`

Sends a direct message to a known user ID.

Requirement: the user must have started the bot at least once.

#### `sendToChannel(const char *channel, const char *message, bool silent = false)`

Sends a message to a Telegram channel.

Requirement: the bot must be an admin in that channel.

#### `forwardMessage(const TBMessage &msg, int64_t to_chatid)`

Forwards a Telegram message to another user or chat.

### Media and Documents

#### `sendPhotoByUrl()` and `sendPhoto()`

Supported patterns:

- photo by URL
- photo from `Stream`
- photo from filesystem if `FS.h` support is available
- photo from raw buffer

#### `sendDocument()`

Sends a document from a `Stream`.

Document types:

- `ZIP`
- `PDF`
- `PHOTO`
- `ANIMATION`
- `AUDIO`
- `VOICE`
- `VIDEO`
- `CSV`
- `JSON`
- `TEXT`
- `BINARY`

#### `sendAnimationByUrl()`

Sends an animation or GIF by URL.

#### `getFile(TBDocument &doc)`

Resolves a Telegram document ID into a downloadable file path and file metadata.

### Message Queries and UI Flow

#### `endQuery(const TBMessage &msg, const char *message, bool alertMode = false)`

Closes a callback query triggered by an inline keyboard button.

#### `removeReplyKeyboard(const TBMessage &msg, const char *message, bool selective = false)`

Removes a reply keyboard for the target chat.

#### `addInlineKeyboard(InlineKeyboard *keyb)`

Registers an inline keyboard when you want button callbacks handled through the helper object.

### Message Editing and Deletion

#### `editMessage()`

Edits a previously sent message.

Overloads support:

- raw chat ID and message ID
- `TBMessage`
- keyboard update through `InlineKeyboard`

#### `deleteMessage(int64_t chat_id, int32_t message_id)`

Deletes a previously sent message.

### Bot Commands

#### `setMyCommands(const String &cmd, const String &desc)`

Adds or updates a bot command.

#### `getMyCommands(String &cmdList)`

Retrieves the current bot command list as JSON.

#### `deleteMyCommands()`

Clears all registered bot commands.

### Delivery Callback

#### `addSentCallback(SentCallback sentcb, uint32_t timeout = 1000)`

Registers a callback fired when the library verifies that a message was sent successfully.

### Bot Information

#### `getBotName()`

Returns the bot username retrieved during initialization.

## Data Structures

Header: [src/DataStructures.h](../src/DataStructures.h)

### `TBUser`

```cpp
struct TBUser {
  bool     isBot;
  int64_t  id;
  String   firstName;
  String   lastName;
  String   username;
};
```

### `TBLocation`

```cpp
struct TBLocation {
  float longitude;
  float latitude;
};
```

### `TBContact`

```cpp
struct TBContact {
  int64_t id;
  String  phoneNumber;
  String  firstName;
  String  lastName;
  String  vCard;
};
```

### `TBDocument`

```cpp
struct TBDocument {
  bool    file_exists;
  int32_t file_size;
  String  file_id;
  String  file_name;
  String  file_path;
};
```

### `TBMessage`

`TBMessage` is the main payload container returned by `getNewMessage()`.

Important fields:

- `messageType`
- `chatId`
- `messageID`
- `text`
- `sender`
- `member`
- `location`
- `contact`
- `document`
- `callbackQueryID`
- `callbackQueryData`
- `disable_notification`
- `force_reply`

## Keyboard Helpers

See also [Keyboards and Interactions](keyboards-and-interactions.md).

### `InlineKeyboard`

Header: [src/InlineKeyboard.h](../src/InlineKeyboard.h)

Methods:

- constructor with optional buffer size
- constructor from existing JSON
- `getButtonsNumber()`
- `addRow()`
- `addButton()`
- `getJSON()`
- `getJSONPretty()`
- `clear()`

### `ReplyKeyboard`

Header: [src/ReplyKeyboard.h](../src/ReplyKeyboard.h)

Methods:

- constructor with optional buffer size
- `addRow()`
- `addButton()`
- `enableResize()`
- `enableOneTime()`
- `enableSelective()`
- `getJSON()`
- `getJSONPretty()`
- `clear()`