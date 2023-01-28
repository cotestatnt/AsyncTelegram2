/*
  Name:        keyboards.ino
  Created:     20/06/2020
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a more complex example that do:
             1) if a "/inline_keyboard1" text message is received, show the inline custom keyboard 1,
                otherwise reply the sender with hint message
             2) if "LIGHT ON" inline keyboard button is pressed turn on the LED and show a message
             3) if "LIGHT OFF" inline keyboard button is pressed, turn off the LED and show a message
             4) if "Button 1" inline keyboard button is pressed show a "modal" message with message box
             5) if "Button 2" inline keyboard button is pressed show a message  
*/

/* 
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure 
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
*/ 
#define USE_CLIENTSSL false
  
#include <AsyncTelegram2.h>

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
  
#elif defined(ESP32)
  #include <WiFi.h>
  #if USE_CLIENTSSL
    #include <WiFiClient.h>
    #include <SSLClient.h>  
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
const char* token =  "xxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  // Telegram token

InlineKeyboard myInlineKbd1, myInlineKbd2; // inline keyboards object helper

#define LIGHT_ON_CALLBACK  "lightON"   // callback data sent when "LIGHT ON" button is pressed
#define LIGHT_OFF_CALLBACK "lightOFF"  // callback data sent when "LIGHT OFF" button is pressed
#define BUTTON1_CALLBACK   "Button1"   // callback data sent when "Button1" button is pressed
#define BUTTON2_CALLBACK   "Button2"   // callback data sent when "Button1" button is pressed

const uint8_t LED = 4;

// Callback functions definition for inline keyboard buttons
void onPressed(const TBMessage &queryMsg){
  digitalWrite(LED, HIGH);
  Serial.printf("\nON button pressed (callback);\nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "Light on", true);
}

void offPressed(const TBMessage &queryMsg){
  digitalWrite(LED, LOW);
  Serial.printf("\nOFF button pressed (callback); \nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "Light on", false);
}

void button1Pressed(const TBMessage &queryMsg){
  Serial.printf("\nButton 1 pressed (callback); \nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "You pressed Button 1", true);
}

void button2Pressed(const TBMessage &queryMsg){
  Serial.printf("\nButton 2 pressed (callback); \nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "You pressed Button 2", false);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED, OUTPUT);  
  // initialize the Serial
  Serial.begin(115200);

  // connects to access point
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
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

  // Add sample inline keyboard
  // add a button that will turn on LED on pin assigned
  myInlineKbd1.addButton("ON",  LIGHT_ON_CALLBACK, KeyboardButtonQuery, onPressed);
  // add a button that will turn off LED on pin assigned
  myInlineKbd1.addButton("OFF", LIGHT_OFF_CALLBACK, KeyboardButtonQuery, offPressed);
  // add a new empty button row
  myInlineKbd1.addRow();
  // add a button that will open browser pointing to this GitHub repository
  myInlineKbd1.addButton("GitHub", "https://github.com/cotestatnt/AsyncTelegram2/", KeyboardButtonURL);
  Serial.printf("Added %d buttons to keyboard\n", myInlineKbd1.getButtonsNumber());
  
  // Add another inline keyboard
  myInlineKbd2.addButton("Button 1", BUTTON1_CALLBACK, KeyboardButtonQuery, button1Pressed);
  myInlineKbd2.addButton("Button 2", BUTTON2_CALLBACK, KeyboardButtonQuery, button2Pressed);
  Serial.printf("Added %d buttons to keyboard\n", myInlineKbd2.getButtonsNumber());
  
  // Add pointer to this keyboard to bot (in order to run callback function)
  myBot.addInlineKeyboard(&myInlineKbd1);
  myBot.addInlineKeyboard(&myInlineKbd2);
}



void loop() {

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  printHeapStats();
  
  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    // check what kind of message I received
    String tgReply;
    MessageType msgType = msg.messageType;
    
    switch (msgType) {
      case MessageText :
        // received a text message
        tgReply = msg.text;
        Serial.print("\nText message received: ");
        Serial.println(tgReply);

        if (tgReply.equalsIgnoreCase("/inline_keyboard1")) {          
          myBot.sendMessage(msg, "This is inline keyboard 1:", myInlineKbd1);          
        }        
        else if (tgReply.equalsIgnoreCase("/inline_keyboard2")) {          
          myBot.sendMessage(msg, "This is inline keyboard 2:", myInlineKbd2);          
        } 
        else {
          // write back feedback message and show a hint
          String text = "You write: \"";
          text += msg.text;
          text += "\"\nTry /inline_keyboard1 or /inline_keyboard2";
          myBot.sendMessage(msg, text);
        }
        break;
        
        /* 
        * Telegram "inline keyboard" provide a callback_data field that can be used to fire a callback fucntion
        * associated at every inline keyboard buttons press event and everything can be handled in it's own callback function. 
        * Anyway, is still possible poll the messagetype in the same way as "reply keyboard" or both.              
        */
        case MessageQuery:
          break;
        
        default:
          break;
    }
  }
}


void printHeapStats() {
  time_t now = time(nullptr);
  struct tm tInfo = *localtime(&now);
  static uint32_t infoTime;
  if (millis() - infoTime > 10000) {
    infoTime = millis();
#ifdef ESP32
    //heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    Serial.printf("\n%02d:%02d:%02d - Total free: %6d - Max block: %6d",
      tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0) );

#elif defined(ESP8266)
    uint32_t free;
    uint16_t max;
    ESP.getHeapStats(&free, &max, nullptr);
    Serial.printf("\nTotal free: %5d - Max block: %5d", free, max);
#endif
  }
}
