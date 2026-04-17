/*
  Name:        echoBot_SSLClient.ino
  Created:     17/04/2026
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: echo bot example based on SSLClient.
               This variant shows how to use AsyncTelegram2 with an
               external TLS stack and a custom connection recovery hook.
*/

const char* ssid  =  "xxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxx";  // Telegram token

// Target user can find it's own userid with the bot @JsonDumpBot
// https://t.me/JsonDumpBot
int64_t userid =  1234567890;  

// Name of public channel (your bot must be in admin group)
const char* channel = "@tolentino_cotesta";

#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <SSLClient.h>

  #include "tg_bearssl_certificate.h"
  #include <AsyncTelegram2.h>

  WiFiClient base_client;
  SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
#else
  #error "echoBot_SSLClient requires a platform configured to use SSLClient."
#endif

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

AsyncTelegram2 myBot(client);

static bool hasSaneTime() {
  time_t now = time(nullptr);
  return now >= 1704067200;  // 2024-01-01 00:00:00 UTC
}

static bool waitForSaneTime(uint32_t timeout_ms = 30000) {
  uint32_t start_time = millis();
  while ((millis() - start_time) < timeout_ms) {
    if (hasSaneTime()) {
      return true;
    }
    delay(250);
  }
  return hasSaneTime();
}

bool recoverTelegramConnection(Client &failedClient, const char *host, uint16_t port) {
  (void)failedClient;
  (void)host;
  (void)port;

  time_t now = time(nullptr);
  if (!hasSaneTime()) {
    Serial.println("Custom TLS recovery skipped: system time not synchronized");
    return false;
  }

  client.setVerificationTime((uint32_t)(now / 86400UL) + 719528UL, (uint32_t)(now % 86400UL));
  Serial.println("Custom TLS recovery: SSLClient verification time updated");
  return true;
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

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  if (waitForSaneTime()) {
    time_t now = time(nullptr);
    client.setVerificationTime((uint32_t)(now / 86400UL) + 719528UL, (uint32_t)(now % 86400UL));
  }
  else {
    Serial.println("NTP sync timeout: SSLClient will use its default verification time until recovery callback runs");
  }
  
  // Set the Telegram bot properies
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // For external TLS stacks such as SSLClient, register a custom recovery
  // hook. Here the retry updates SSLClient with the current verification time.
  myBot.setConnectionRecoveryCallback(recoverTelegramConnection);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  bool connected = myBot.begin();
  const char* connection_mode = myBot.getConnectionModeName();
  connected ? Serial.println("OK") : Serial.println("NOK");
  Serial.printf("Connection mode: %s\n", connection_mode);

  char welcome_msg[192];
  snprintf(welcome_msg, sizeof(welcome_msg), "BOT @%s online\nConnection mode: %s\n/help all commands avalaible.", myBot.getBotName(), connection_mode);

  // Send a message to specific user who has started your bot
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
    // Send a message to your public channel
    String message ;
    message += "Message from @";
    message += myBot.getBotName();
    message += ":\n";
    message += msg.text;
    Serial.println(message);
    myBot.sendToChannel(channel, message, true);

    // echo the received message
    myBot.sendMessage(msg, msg.text);
  }
}