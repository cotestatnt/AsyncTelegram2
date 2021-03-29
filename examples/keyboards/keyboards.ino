/*
  Name:        keyboards.ino
  Created:     26/03/2021
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

/* 
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure 
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
  With SSLClient, be sure "certificates.h" file is present in sketch folder
*/ 
#define USE_CLIENTSSL true  

#include <AsyncTelegram2.h>

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
  
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #if USE_CLIENTSSL
    #include <SSLClient.h>  
    #include "certificates.h"
    WiFiClient base_client;
    SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
  #else
    WiFiClientSecure client;  
  #endif
#endif

AsyncTelegram2 myBot(client);
const char* ssid  =  "xxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  // Telegram token

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

  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

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
  client.setBufferSizes(TCP_MSS, TCP_MSS);
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
  myInlineKbd.addButton("GitHub", "https://github.com/cotestatnt/AsyncTelegram/", KeyboardButtonURL);
}


void loop() {

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // local variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
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
        Serial.println(msgText);
        
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

      case MessageLocation:
        // received a location message --> send a message with the location coordinates
        char bufL[50];
        snprintf(bufL, sizeof(bufL), "Longitude: %f\nLatitude: %f\n", msg.location.longitude, msg.location.latitude) ;
        myBot.sendMessage(msg, bufL);
        Serial.println(bufL);
        break;

      case MessageContact:
        char bufC[50];
        snprintf(bufC, sizeof(bufC), "Contact information received: %s - %s\n", msg.contact.firstName, msg.contact.phoneNumber ) ;
        // received a contact message --> send a message with the contact information
        myBot.sendMessage(msg, bufC);
        Serial.println(bufC);
        break;
        
      default:
        break;
    }
  }
}
