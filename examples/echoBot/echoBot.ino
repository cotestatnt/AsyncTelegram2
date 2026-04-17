/*
  Name:        echoBot.ino
  Created:     17/04/2026
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a simple example that check for incoming messages
               and reply the sender with the received message.
               The message will be forwarded also in a public channel
               and to a specific userid.
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
const char* token =  "xxxxxxxx";  // Telegram token

// Target user can find it's own userid with the bot @JsonDumpBot
// https://t.me/JsonDumpBot
int64_t userid =  1234567890;  

// Name of public channel (your bot must be in admin group)
const char* channel = "@tolentino_cotesta";

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
  //Set certficate, session and some other base client properies
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

  // Enable insecure fallback (optional, but recommended for better reliability)
  myBot.enableInsecureFallback();

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