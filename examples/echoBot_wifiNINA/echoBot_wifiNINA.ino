/*
  Name:        echoBot_wifiNINA.ino
  Created:     27/10/2022
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a simple example that check for incoming messages
               and reply the sender with the received message.

  !!!!!!!!!!!!!!!!!!!!!!!      IMPORTANT     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  In order to use this library with WiFiNINA modules,
  you need to upload Telegram server SSL certitficate.

  Host to be added:  api.telegram.org

  How-to:
  https://support.arduino.cc/hc/en-us/articles/360016119219-How-to-add-certificates-to-Wifi-Nina-Wifi-101-Modules-

  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <AsyncTelegram2.h>

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

WiFiSSLClient client;
AsyncTelegram2 myBot(client);

const char* ssid  =  "xxxxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxx:xxxxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;

int status = WL_IDLE_STATUS;

void printWiFiStatus() {
  Serial.print("Connected to wifi. ");
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());
  Serial.print(". IP Address: ");
  Serial.println(WiFi.localIP());

  // print the received signal strength:
  Serial.print("signal strength (RSSI):");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // init and wait for serial port to connect (needed for native USB port)
  Serial.begin(115200);
  while (!Serial) ;

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true); // don't continue
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWiFiStatus();

  Serial.println("\nStarting TelegramBot...");
  // Set the Telegram bot properies
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // Set the JSON buffer size for receeved message parsing (default 1024 bytes)
  myBot.setJsonBufferSize(1024);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());

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
    message += ": ";
    message += msg.text;
    Serial.println(message);

    // echo the received message
    myBot.sendMessage(msg, msg.text);
  }
}