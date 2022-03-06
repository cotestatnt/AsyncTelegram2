/*
  Name:        keyboards.ino
  Created:     05/03/2022
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a more complex example that do:
             1) if a "/inline_keyboard" text message is received, show the inline custom keyboard,
                if a "/reply_keyboard" text message is received, show the reply custom keyboard,
                otherwise reply the sender with "Try /reply_keyboard or /inline_keyboard" message
             2) if "LIGHT ON" inline keyboard button is pressed turn on the LED and show a message
             3) if "LIGHT OFF" inline keyboard button is pressed, turn off the LED and show a message
             4) if "GitHub" inline keyboard button is pressed,
                open a browser window with URL "https://github.com/cotestatnt/AsyncTelegram"
*/
#include <AsyncTelegram2.h>

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

/*
  Set true if you want use external library for SSL connection 
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library and 
  you can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet or GPRS)
*/
#define USE_CLIENTSSL false

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  WiFiClientSecure client;
  Session   session;
  X509List  certificate(telegram_cert);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #if USE_CLIENTSSL
    #include <SSLClient.h>      // https://github.com/OPEnSLab-OSU/SSLClient/
    #include "tg_certificate.h"
    WiFiClient base_client;
    SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
  #else
    #include <WiFiClientSecure.h>
    WiFiClientSecure client;
  #endif
#endif

AsyncTelegram2 myBot(client);
const char* ssid  =  "xxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;

ReplyKeyboard myReplyKbd;   // reply keyboard object helper
InlineKeyboard myInlineKbd; // inline keyboard object helper
bool isKeyboardActive;      // store if the reply keyboard is shown

#define LIGHT_ON_CALLBACK  "lightON"  // callback data sent when "LIGHT ON" button is pressed
#define LIGHT_OFF_CALLBACK "lightOFF" // callback data sent when "LIGHT OFF" button is pressed
const uint8_t LED = 4;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);

  // connects to the access point
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

#ifdef ESP8266
  // Sync time with NTP, to check properly Telegram certificate
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  //Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #if USE_CLIENTSSL == false
    client.setCACert(telegram_cert);
  #endif
#endif

  // Set the Telegram bot properties
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  Serial.print("Bot name: @");
  Serial.println(myBot.getBotName());

  // Add reply keyboard
  isKeyboardActive = false;
  // add a button that send a message with "Simple button" text
  myReplyKbd.addButton("Button1");
  myReplyKbd.addButton("Button2");
  myReplyKbd.addButton("Button3");
  // add a new empty button row
  myReplyKbd.addRow();
  // add another button that send the user position (location)
  myReplyKbd.addButton("Send Location", KeyboardButtonLocation);
  // add another button that send the user contact
  myReplyKbd.addButton("Send contact", KeyboardButtonContact);
  // add a new empty button row
  myReplyKbd.addRow();
  // add a button that send a message with "Hide replyKeyboard" text
  // (it will be used to hide the reply keyboard)
  myReplyKbd.addButton("/hide_keyboard");
  // resize the keyboard to fit only the needed space
  myReplyKbd.enableResize();

  // Add sample inline keyboard
  myInlineKbd.addButton("ON", LIGHT_ON_CALLBACK, KeyboardButtonQuery);
  myInlineKbd.addButton("OFF", LIGHT_OFF_CALLBACK, KeyboardButtonQuery);
  myInlineKbd.addRow();
  myInlineKbd.addButton("GitHub", "https://github.com/cotestatnt/AsyncTelegram2/", KeyboardButtonURL);

  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());

  // Send a message to specific user who has started your bot
  myBot.sendTo(userid, welcome_msg);
}


void loop() {

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // if there is an incoming message...
  // local variable to store telegram message data
  TBMessage msg;
  if (myBot.getNewMessage(msg)) {
    // check what kind of message I received
    MessageType msgType = msg.messageType;
    String msgText = msg.text;

    switch (msgType) {
      case MessageText :
        // received a text message
        Serial.print("\nText message received: ");
        Serial.println(msgText);

        // check if is show keyboard command
        if (msgText.equalsIgnoreCase("/reply_keyboard")) {
          // the user is asking to show the reply keyboard --> show it
          myBot.sendMessage(msg, "This is reply keyboard:", myReplyKbd);
          isKeyboardActive = true;
        }
        else if (msgText.equalsIgnoreCase("/inline_keyboard")) {
          myBot.sendMessage(msg, "This is inline keyboard:", myInlineKbd);
        }

        // check if the reply keyboard is active
        else if (isKeyboardActive) {
          // is active -> manage the text messages sent by pressing the reply keyboard buttons
          if (msgText.equalsIgnoreCase("/hide_keyboard")) {
            // sent the "hide keyboard" message --> hide the reply keyboard
            myBot.removeReplyKeyboard(msg, "Reply keyboard removed");
            isKeyboardActive = false;
          } else {
            // print every others messages received
            myBot.sendMessage(msg, msg.text);
          }
        }

        // the user write anything else and the reply keyboard is not active --> show a hint message
        else {
          myBot.sendMessage(msg, "Try /reply_keyboard or /inline_keyboard");
        }
        break;

      case MessageQuery:
        // received a callback query message
        msgText = msg.callbackQueryData;
        Serial.print("\nCallback query message received: ");
        Serial.println(msg.callbackQueryData);

        if (msgText.equalsIgnoreCase(LIGHT_ON_CALLBACK)) {
          // pushed "LIGHT ON" button...
          Serial.println("\nSet light ON");
          digitalWrite(LED, HIGH);
          // terminate the callback with an alert message
          myBot.endQuery(msg, "Light on", true);
        }
        else if (msgText.equalsIgnoreCase(LIGHT_OFF_CALLBACK)) {
          // pushed "LIGHT OFF" button...
          Serial.println("\nSet light OFF");
          digitalWrite(LED, LOW);
          // terminate the callback with a popup message
          myBot.endQuery(msg, "Light off");
        }
        break;

      case MessageLocation: {
          // received a location message
          String reply = "Longitude: ";
          reply += msg.location.longitude;
          reply += "; Latitude: ";
          reply += msg.location.latitude;
          Serial.println(reply);
          myBot.sendMessage(msg, reply);
          break;
        }

      case MessageContact: {
          // received a contact message
          String reply = "Contact information received:";
          reply += msg.contact.firstName;
          reply += " ";
          reply += msg.contact.lastName;
          reply += ", mobile ";
          reply += msg.contact.phoneNumber;
          Serial.println(reply);
          myBot.sendMessage(msg, reply);
          break;
        }

      default:
        break;
    }
  }
}