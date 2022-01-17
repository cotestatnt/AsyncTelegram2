/*
  Name:        echoBot.ino
  Created:     26/03/2021
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a simple example that check for incoming messages
               and reply the sender with the received message.
                 The message will be forwarded also in a public channel
                 anad to a specific userid.
*/

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
#include <WiFiClientSecure.h>
WiFiClientSecure client;
#endif

AsyncTelegram2 myBot(client);

const char* ssid  =  "xxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxxxxxxx";

// https://t.me/JsonDumpBot
int64_t userid = 1234567890;

/* 
 *  This function will be excuted once the message was sent
 *  if succesfull sent will be true, otherwise false
 */
void messageSent(bool sent) {
  if (sent) {
    Serial.println("Last message was delivered");
  }
  else {
    Serial.println("Last message was NOT delivered");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("\nStarting TelegramBot...");

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
  // Set certificate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  client.setCACert(telegram_cert);
#endif

  // Set the Telegram bot properies
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // Add the callback function to bot
  myBot.addSentCallback(messageSent);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  char welcome_msg[128];
  snprintf(welcome_msg, 128, 
      "BOT @%s online\n."
      "This bot will /echo all messages received,\n"
      "except thoose wich contains /noecho word inside", myBot.getBotName());

  // Send a message to specific userid
  myBot.sendTo(userid, welcome_msg);
}

void loop() {

  static uint32_t ledTime = millis();
  if (millis() - ledTime > 150) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // local variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    String message = "\n-----------------------------\n";
    message += "New message from @";
    message += myBot.getBotName();
    message += ": ";
    message += msg.text;
    Serial.println(message);

    // ...echo the received message
    if(msg.text.indexOf("/noecho")> -1) 
      myBot.sendTo(123456789, "Send this to dummy userid");
    else
      myBot.sendMessage(msg, msg.text);
  }
}
