# Keyboards and Interactions

Telegram keyboards are one of the most useful parts of AsyncTelegram2 because they let you build guided user interactions instead of relying only on free-text commands.

This guide explains the two helper classes provided by the library:

- `InlineKeyboard`
- `ReplyKeyboard`

## Inline vs Reply Keyboard

Use an `InlineKeyboard` when:

- buttons should be attached to a specific message
- you want callback queries instead of normal chat messages
- you want URL buttons

Use a `ReplyKeyboard` when:

- you want Telegram to show a custom keyboard in the chat input area
- pressing a button should send text, contact, or location information

## InlineKeyboard Basics

Header: [src/InlineKeyboard.h](../src/InlineKeyboard.h)

Main methods:

- `addRow()`
- `addButton(text, command, buttonType, onClick)`
- `getJSON()`
- `getJSONPretty()`
- `clear()`

Button types:

- `KeyboardButtonURL`
- `KeyboardButtonQuery`

### Example

```cpp
InlineKeyboard keyboard;

keyboard.addButton("Open Docs", "https://github.com/cotestatnt/AsyncTelegram2", KeyboardButtonURL);
keyboard.addRow();
keyboard.addButton("Status", "status", KeyboardButtonQuery);
```

Then send it with:

```cpp
bot.sendMessage(msg, "Choose an action", keyboard);
```

## Callback Queries

When a user presses an inline callback button:

- no normal text message is sent to the chat
- `getNewMessage()` returns a `TBMessage` with `messageType == MessageQuery`
- `callbackQueryData` contains the command payload associated with the pressed button

Typical handler:

```cpp
TBMessage msg;
if (bot.getNewMessage(msg)) {
  if (msg.messageType == MessageQuery) {
    if (msg.callbackQueryData == "status") {
      bot.endQuery(msg, "Status received");
      bot.sendMessage(msg, "Everything is OK");
    }
  }
}
```

## `endQuery()` is required

When processing an inline callback, call `endQuery()` after handling it.

This tells Telegram that the callback has been processed and closes the client-side waiting state.

If you skip it, the Telegram app can keep showing the button as pending.

## Optional callback functions on buttons

`InlineKeyboard::addButton()` also supports a callback pointer:

```cpp
void onStatusPressed(const TBMessage &msg) {
  Serial.println(msg.callbackQueryData);
}

keyboard.addButton("Status", "status", KeyboardButtonQuery, onStatusPressed);
```

If you use callback functions, register the keyboard inside the bot:

```cpp
bot.addInlineKeyboard(&keyboard);
```

This is useful when you want the keyboard object itself to route button actions.

## ReplyKeyboard Basics

Header: [src/ReplyKeyboard.h](../src/ReplyKeyboard.h)

Main methods:

- `addRow()`
- `addButton(text, buttonType)`
- `enableResize()`
- `enableOneTime()`
- `enableSelective()`
- `getJSON()`
- `getJSONPretty()`
- `clear()`

Button types:

- `KeyboardButtonSimple`
- `KeyboardButtonContact`
- `KeyboardButtonLocation`

### Example

```cpp
ReplyKeyboard keyboard;
keyboard.addButton("Light ON");
keyboard.addButton("Light OFF");
keyboard.addRow();
keyboard.addButton("Send Contact", KeyboardButtonContact);
keyboard.addButton("Send Location", KeyboardButtonLocation);
keyboard.enableResize();
```

Then send it with:

```cpp
bot.sendMessage(msg, "Choose an option", keyboard);
```

## Removing a Reply Keyboard

Use:

```cpp
bot.removeReplyKeyboard(msg, "Keyboard removed");
```

This is useful when a guided flow is finished and you want to return the chat to normal input mode.

## Editing a Message After a Button Press

AsyncTelegram2 supports message editing through `editMessage()`.

This is especially useful with inline keyboards because you can keep the interaction in a single message.

Typical uses:

- replace a menu with a result
- update the keyboard buttons based on state
- disable buttons after one action has been completed

## Common Interaction Patterns

### Text command + inline menu

1. user sends `/menu`
2. bot replies with an `InlineKeyboard`
3. user presses a callback button
4. bot handles `MessageQuery`
5. bot closes the query with `endQuery()`

### Reply keyboard for guided input

1. bot sends a `ReplyKeyboard`
2. user presses a keyboard button
3. the pressed button sends a normal message or a special contact/location payload
4. bot reads it with `getNewMessage()`

## Relevant Examples

- [examples/keyboards](../examples/keyboards)
- [examples/keyboardCallback](../examples/keyboardCallback)
- [examples/lightBot](../examples/lightBot)