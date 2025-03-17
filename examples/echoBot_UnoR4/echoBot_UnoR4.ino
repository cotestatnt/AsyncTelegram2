/*
  Name:        echoBot.ino
  Created:     07/03/2025
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a simple example that check for incoming messages and reply the sender with the received message.
               The message will be forwarded also in a public channel and to a specific userid.
*/

#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include <AsyncTelegram2.h>

int status = WL_IDLE_STATUS;

WiFiSSLClient client;
AsyncTelegram2 myBot(client);

const char* ssid = "xxxxxxxxxx";                         // SSID WiFi network
const char* pass = "xxxxxxxxxx";                         // Password  WiFi network
const char* token = "xxxxxxxxxx:xxxxxxxxxx-xxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("\nStarting TelegramBot...");

  while (!Serial) ; // wait for serial port to connect. Needed for native USB port only

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network.
    delay(5000);  // wait 5 seconds for connection:
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

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