# Reference
Here you can find an explanation of the functionalities provided and how to use the library. 
Check the [examples folder](https://github.com/cotestatnt/AsyncTelegram2/tree/master/examples) for demos and examples.
___
## Table of contents
+ [Introduction and quick start](#introduction-and-quick-start)
+ [Inline Keyboards](#inline-keyboards)
  + [Using Inline Keyboards into AsyncTelegram2 class](#using-inline-keyboards-into--class)
  + [Handling callback messages](#handling-callback-messages)
+ [Data types](#data-types)
  + [TBUser](#tbuser)
  + [TBLocation](#tblocation)
  + [TBGroup](#tbgroup)
  + [TBContact](#tbcontact)
  + [TBMessage](#tbmessage)
+ [Enumerators](#enumerators)
  + [MessageType](#messagetype)
  + [InlineKeyboardButtonType](#inlinekeyboardbuttontype)
+ [Basic methods](#basic-methods)
  + [AsyncTelegram2::setTelegramToken()](#settelegramtoken)
  + [AsyncTelegram2::testConnection()](#testconnection)
  + [AsyncTelegram2::getNewMessage()](#getnewmessage)
  + [AsyncTelegram2::sendMessage()](#sendmessage)
  + [AsyncTelegram2::endQuery()](#endquery)
  + [AsyncTelegram2::removeReplyKeyboard()](#removereplykeyboard)
  + [InlineKeyboard::addButton()](#inlinekeyboardaddbutton)
  + [InlineKeyboard::addRow()](#inlinekeyboardaddrow)
  + [InlineKeyboard::flushData()](#inlinekeyboardflushdata)
  + [InlineKeyboard::getJSON()](#inlinekeyboardgetjson)
___
## Introduction and quick start
Once installed the library, you have to load it in your sketch and define wich type of HTTP client will be used for connection
```c++
#include "AsyncTelegram2.h"
BearSSL::WiFiClientSecure client;
BearSSL::Session   session;
BearSSL::X509List  certificate(telegram_cert);  // telegram_cert is a const char array define in AsyncTelegram2
 
```
...at this point you can instantiate a `AsyncTelegram2` object, passing the client as parameter
```c++
AsyncTelegram2 myBot(client);
```
...Use the `setTelegramToken()` member function to set your Telegram Bot token in order establish connections with the bot
```c++
myBot.setTelegramToken("myTelegramBotToken");
```

Set the default [formatting style](https://core.telegram.org/bots/api#formatting-options) (none, HTML, MarkdownV2)
```c++
myBot.setFormattingStyle(AsyncTelegram2::FormatStyle::HTML /* MARKDOWN */);
...
```

In order to receive messages, declare a `TBMessage` variable...
```c++
TBMessage msg;
```
...and execute the `getNewMessage()` member fuction. 
The `getNewMessage()` return a non-zero value if there is a new message and store it in the `msg` variable. See the [TBMessage](#tbmessage) data type for further details.
```c++
myBot.getNewMessage(msg);
```
To send a simple message to a Telegram user, use the `sendMessage(TBMessage msg, String text )` member function 
```c++
myBot.sendMessage(msg, "message");
```
See the [echoBot example](https://github.com/cotestatnt/AsyncTelegram2/blob/master/examples/echoBot/echoBot.ino) for further details.

[back to TOC](#table-of-contents)
___
## Inline Keyboards
The Inline Keyboards are special keyboards integrated directly into the messages they belong to: pressing buttons on inline keyboards doesn't result in messages sent to the chat. Instead, inline keyboards support buttons that work behind the scenes.
AsyncTelegram2 class implements the following buttons:
+ URL buttons: these buttons have a small arrow icon to help the user understand that tapping on a URL button will open an external link. A confirmation alert message is shown before opening the link in the browser.
+ Callback buttons: when a user presses a callback button, no messages are sent to the chat. Instead, the bot simply receives the relevant query. Upon receiving the query, the bot can display some result in a notification at the top of the chat screen or in an alert. It's also possible associate a callback function that will be executed when user press the inline keyboard button.

[back to TOC](#table-of-contents)

### Using Inline Keyboards into AsyncTelegram2 class
In order to show an inline keyboard, use the method [sendMessage()](#sendmessage) specifing the parameter `keyboard`.
The `keyboard` parameter is a string that contains a JSON structure that define the inline keyboard. See [Telegram docs](https://core.telegram.org/bots/api#sendmessage).<br>
To simplify the creation of an inline keyboard, there is an helper class called `InlineKeyboard`.
Creating an inline keyboard with a `InlineKeyboard` is straightforward:

Fristly, instantiate a `InlineKeyboard` object:
```c++
InlineKeyboard kbd;
```
then add new buttons in the first row of the inline keyboard using the member fuction `addButton()` (See [addButton()](#addbutton) member function).
```c++
kbd.addButton("First Button label", "URL for first button", KeyboardButtonURL);                // URL button
kbd.addButton("Second Button label", "Data for second button", KeyboardButtonQuery, onPress);  // callback button
...
```
If a new row of buttons is needed, call the addRow() member function...
```c++
kbd.addRow();
```
... and add buttons to the just created row:
```c++
kbd.addButton("New Row Button label", "URL for the new row button", KeyboardButtonURL); // URL button
...
```
If you plan to use callback functions associated to inlineQuery buttons, add the pointer to just defined InlineKeyboard object to the AsyncTelegram2 instance
```c++
myBot.addInlineKeyboard(&kbd);
...
```
Once finished, send the inline keyboard using the `sendMessage` method:
```c++
myBot.sendMessage(<msg>, "message", kbd);
...
```

To clear keyboard JSON content:
```c++
myBot.clear();
...
```
[back to TOC](#table-of-contents)

### Handling callback messages
Everytime an inline keyboard button is pressed, a special message is sent to the bot: the `getNewMessage()` returns `MessageQuery` value and the `TBMessage` data structure is filled with the callback data.
When query button is pressed, is mandatory to notify the Telegram Server the end of the query process by calling the `endQuery()` method.
Here an example:
```c++
.....

AsyncTelegram2 myBot(client);
#define CALLBACK_QUERY_DATA  "QueryData"  // callback data sent when the button is pressed
InlineKeyboard myKbd;  // custom inline keyboard object helper

void setup() {
  Serial.begin(115200); // initialize the serial
  WiFi.mode(WIFI_STA); 	
  WiFi.begin(ssid, pass);
  myBot.setTelegramToken("myTelegramBotToken"); // set the telegram bot token

	// inline keyboard - only a button called "My button"
	myKbd.addButton("My button", CALLBACK_QUERY_DATA, KeyboardButtonQuery);
}

void loop() {
	TBMessage msg; // a variable to store telegram message data

	// if there is an incoming message...
	if (myBot.getNewMessage(msg)) {
		// ...and if it is a callback query message
	    if (msg.messageType == MessageQuery) {
			// received a callback query message, check if it is the "My button" callback
			if (msg.callbackQueryData.equals(CALLBACK_QUERY_DATA)) {
				// pushed "My button" button --> do related things...

				// close the callback query
				myBot.endQuery(msg, "My button pressed");
			}
		} else {
			// the received message is a text message --> reply with the inline keyboard
			myBot.sendMessage(msg, "Inline Keyboard", myKbd);
		}
	}
	delay(500); // wait 500 milliseconds
}
```
See the [keyboards example](https://github.com/shurillu/AsyncTelegram2/blob/master/examples/keyboards/keyboards.ino) for further details. <br>

[back to TOC](#table-of-contents)
___
## Data types
There are several usefully data structures used to store data typically sent by the Telegram Server.
### `TBUser`
`TBUser` data type is used to store user data like Telegram userID. The data structure contains:
```c++
  bool          isBot;
  int32_t       id = 0;
  const char*   firstName;
  const char*   lastName;
  const char*   username;
  const char*   languageCode;
```
where:
+ `isBot` tells if the user ID `id` refers to a bot (`true` value) or not (`false ` value)
+ `id` is the unique Telegram user ID
+ `firstName` contains the first name (if provided) of the user ID `id`
+ `lastName` contains the last name (if provided) of the user ID `id`
+ `username` contains the username of the user ID `id`
+ `languageCode` contains the country code used by the user ID `id`

Typically, you will use predominantly the `id` field.

[back to TOC](#table-of-contents)
### `TBLocation`
`TBLocation` data type is used to store the longitude and the latitude. The data structure contains:
```c++
float longitude;
float latitude;
```
where:
+ `longitude` contains the value of the longitude
+ `latitude` contains the value of the latitude

For localization messages, see [TBMessage](#tbmessage)

[back to TOC](#table-of-contents)
### `TBGroup`
`TBGroup` data type is used to store the group chat data. The data structure contains:
```c++
int64_t       id;
const char*   title;
```
where:
+ `id` contains the ID of the group chat
+ `title` contains the title of the group chat

[back to TOC](#table-of-contents)


### `TBContact`
`TBContact` data type is used to store the contact data. The data structure contains:
```c++
  int32_t 	   id;
  const char*  phoneNumber;
  const char*  firstName;
  const char*  lastName;
  const char*  vCard;
```
where:
+ `id` contains the ID of the contact
+ `phoneNumber` contains the phone number of the contact
+ `firstName` contains the first name of the contact
+ `lastName` contains the last name of the contact
+ `vCard` contains the vCard of the contact

[back to TOC](#table-of-contents)


### `TBMessage`
`TBMessage` data type is used to store new messages. The data structure contains:
```c++
  MessageType 	  messageType;
  bool			      isHTMLenabled = false;
  bool       		  isMarkdownEnabled = false;
  bool 	     	    disable_notification = false;
  bool			      force_reply = false;
  int32_t         date;
  int32_t         chatInstance;
  int64_t         chatId;
  int32_t         messageID;
  TBUser          sender;
  TBGroup         group;
  TBLocation      location;
  TBContact       contact;
  TBDocument      document;
  const char*     callbackQueryData;
  const char*   	callbackQueryID;
  String      	  text;
```
where:
+ `isHTMLenabled` enable HTML-style messages [https://core.telegram.org/bots/api#formatting-options](formatting options)
+ `isMarkdownEnabled` enable markdown-style v2 messages [https://core.telegram.org/bots/api#formatting-options](formatting options)
+ `disable_notification` send a message in "silent mode"
+ `force_reply` send a message as reply to a message
+ `messageType` contains the message type. See [CTBotMessageType](#messagetype)
+ `messageID` contains the unique message identifier associated to the received message
+ `sender` contains the sender data in a [TBUser](#tbuser) structure
+ `group` contains the group chat data in a [TBGroup](#tbgroup) structure
+ `date` contains the date when the message was sent, in Unix time
+ `text` contains the received message (if a text message is received - see [AsyncTelegram2::getNewMessage()](#getnewmessage))
+ `chatInstance` contains the unique ID corresponding to the chat to which the message with the callback button was sent
+ `callbackQueryData` contains the data associated with the callback button
+ `callbackQueryID` contains the unique ID for the query
+ `location` contains the location's longitude and latitude (if a location message is received - see [AsyncTelegram2::getNewMessage()](#getnewmessage))
+ `contact` contains the contact information a [TBContact](#tbcontact) structure

[back to TOC](#table-of-contents)
___
## Enumerators
There are several usefully enumerators used to define method parameters or method return value.

### `MessageType`
Enumerator used to define the possible message types received by [getNewMessage()](#getnewmessage) method. Used also by [TBMessage](#tbmessage).
```c++
enum MessageType {
	MessageNoData   = 0,
  MessageText     = 1,
  MessageQuery    = 2,
  MessageLocation = 3,
  MessageContact  = 4,
  MessageDocument = 5,
  MessageReply 	= 6
};
```
where:
+ `MessageNoData`: error - the [TBMessage](#tbmessage) structure contains no valid data
+ `MessageText`: the [TBMessage](#tbmessage) structure contains a text message
+ `MessageQuery`: the [TBMessage](#tbmessage) structure contains a calback query message (see [Inline Keyboards](#inline-keyboards))
+ `MessageLocation`: the [TBMessage](#tbmessage) structure contains a localization message
+ `MessageContact`: the [TBMessage](#tbmessage) structure contains a contact message
+ `MessageDocument`: the [TBMessage](#tbmessage) structure contains a document message
+ `MessageReply`: the [TBMessage](#tbmessage) structure contains a forced reply message

[back to TOC](#table-of-contents)

### `InlineKeyboardButtonType`
Enumerator used to define the possible button types. Button types are used when creating an inline keyboard with [addButton()](#addbutton) method.
```c++
enum InlineKeyboardButtonType {
	KeyboardButtonURL    = 1,
	KeyboardButtonQuery  = 2
};

```
where:
+ `KeyboardButtonURL`: define a URL button. When pressed, Telegram client will ask if open the URL in a browser
+ `KeyboardButtonQuery`: define a calback query button. When pressed, a callback query message is sent to the bot

[back to TOC](#table-of-contents)


___
## Basic methods
Here you can find the basic member function. First you have to instantiate a AsyncTelegram2 object, like ` myBot`, then call the desired member function as `myBot.myDesiredFunction()`

[back to TOC](#table-of-contents)
### `AsyncTelegram2::setTelegramToken()`
`void AsyncTelegram2::setTelegramToken(String token)` <br><br>
Set the Telegram Bot token. If you need infos about Telegram Bot and how to obtain a token, take a look  [here](https://core.telegram.org/bots#6-botfather). <br>
Parameters:
+ `token`: the token that identify the Telegram Bot

Returns: none. <br>
Example:
+ `setTelegramToken("myTelegramBotToken")`


[back to TOC](#table-of-contents)
### `AsyncTelegram2::begin()`
`bool AsyncTelegram2::begin(void)` <br><br>
Check the connection between board and the Telegram server. <br>
Parameters: none <br>
Returns: `true` if the board is able to send/receive data to/from the Telegram server. <br>
Example:
```c++
void setup() {
   Serial.begin(115200); // initialize the serial
   myBot.wifiConnect("mySSID", "myPassword"); // connect to the WiFi Network
   myBot.setTelegramToken("myTelegramBotToken"); // set the telegram bot token
   if(myBot.begin())
      Serial.println("Connection OK");
   else
      Serial.println("Connectionk NOK");
}
void loop() {
}
```

[back to TOC](#table-of-contents)
### `AsyncTelegram2::getNewMessage()`

`MessageType AsyncTelegram2::getNewMessage(TBMessage &message)` <br><br>
Get the first unread message from the message queue. Fetch text message and callback query message (for callback query messages, see [Inline Keyboards](#inline-keyboards)). This is a destructive operation: once read, the message will be marked as read so a new `getNewMessage` will fetch the next message (if any). <br>
Parameters:
+ `message`: a `TBMessage` data structure that will contains the message data retrieved

Returns:
+ `MessageType` if no error occurs


[back to TOC](#table-of-contents)
### `AsyncTelegram2::sendMessage()`
`void sendMessage(const TBMessage &msg, const char* message, String keyboard = "");` <br>
`void sendMessage(const TBMessage &msg, String &message, String keyboard = "");` <br>
`void sendMessage(const TBMessage &msg, const char* message, ReplyKeyboard  &keyboard);` <br>
`void sendMessage(const TBMessage &msg, const char* message, InlineKeyboard &keyboard);	` <br><br>

Send a message to the Telegram user ID associated with recevied msg. <br>
If `keyboard` parameter is specified, send the message and display the custom keyboard (inline or reply). 
+ Inline keyboard are defined by a JSON structure (see the Telegram API documentation [InlineKeyboardMarkup](https://core.telegram.org/bots/api#inlinekeyboardmarkup))<br>
You can also use the helper class InlineKeyboard for creating inline keyboards.<br> 
+ Reply keyboard are define by a JSON structure (see Telegram API documentation [ReplyKeyboardMarkup](https://core.telegram.org/bots/api#replykeyboardmarkup))<br>
You can also use the helper class ReplyKeyboard for creating inline keyboards.<br> 

Parameters:
+ `msg`: the TBMessage recipient structure
+ `message`: the message to send
+ `keyboard`: (optional) the inline/reply keyboard

[back to TOC](#table-of-contents)


### `AsyncTelegram2::removeReplyKeyboard()`
`bool removeReplyKeyboard(int64_t id, String message, bool selective = false)` <br><br>
Remove an active replyKeyboard for a specified user by sending a message. <br>
Parameters:
+ `msg`: the TBMessage recipient structure
+ `message`: the message to be show to the selected user ID
+ `selective`: (optional) enable the selective mode (hide the keyboard for specific users only). Useful for hiding the keyboard for users that are @mentioned in the text of the Message object or if the bot's message is a reply (has reply_to_message_id), sender of the original message

Returns: `true` if no error occurred. <br>

[back to TOC](#table-of-contents)


### `InlineKeyboard::addButton()`
`bool addButton(const char* text, const char* command, InlineKeyboardButtonType buttonType, CallbackType onClick = nullptr)` <br><br>
Add a button to the current keyboard row of an InlineKeyboard object. For a description of button types, see [Inline Keyboards](#inline-keyboards).<br>
Parameters: 
+ `text`: the botton text (label) displayed on the inline keyboard
+ `command`: depending on the button type, 
  + on URL buttons, contain the URL
  + on a query button, contain the query data
+ `buttonType`: set the behavior of the button. It can be:
  + `KeyboardButtonURL` - the added button will be a URL button
  + `KeyboardButtonQuery` - the added button will be a query button
+ `onClick`: pointer to callback function
Returns: `true` if no error occurred. <br>

[back to TOC](#table-of-contents)

### `InlineKeyboard::addRow()`
`bool InlineKeyboard::addRow(void)` <br><br>
Add a new empty row of buttons to the inline keyboard: all the new keyboard buttons will be added to this new row.
Parameters: none <br>
Returns: `true` if no error occurred. <br>


[back to TOC](#table-of-contents)

### `InlineKeyboard::getJSON()`
`String InlineKeyboard::getJSON(void)` <br><br>
Create a string that containsthe inline keyboard formatted in a JSON structure. Useful sending the inline keyboard with [sendMessage()](#sendmessage).
Parameters: none <br>
Returns: the JSON of the inline keyboard <br>

[back to TOC](#table-of-contents)
