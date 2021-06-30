/*
  Name:        OTA_password.ino
  Created:     29/03/2021
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: an example that check for incoming messages
              and install rom update remotely.
*/

#include <AsyncTelegram2.h>

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>

BearSSL::WiFiClientSecure client;
BearSSL::Session   session;
BearSSL::X509List  certificate(telegram_cert);
  
AsyncTelegram2 myBot(client);

const char* ssid = "XXXXXXXXX";     // REPLACE mySSID WITH YOUR WIFI SSID
const char* pass = "XXXXXXXXX";     // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
const char* token = "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXX";   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN

const char* firmware_version = __TIME__;
const char* fw_password = "update";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pass);
  delay(500);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  }

  // Sync time with NTP, to check properly Telegram certificate
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  //Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
  
  // Set the Telegram bot properies
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  
  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  int64_t chat_id = 1234567890; // You can discover your own chat id, with "Json Dump Bot"
  myBot.sendTo(chat_id, welcome_msg);
  
  // We have to handle reboot manually after sync with TG server
  ESPhttpUpdate.rebootOnUpdate(false);
}



void loop() {

  static uint32_t ledTime = millis();
  if (millis() - ledTime > 300) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg))
  {
    String tgReply;
    static String document;    
    switch (msg.messageType){
      case MessageDocument :
        {
            document = msg.document.file_path;
            if (msg.document.file_exists) {
    
              // Check file extension of received document (firmware must be .bin)
              if( msg.document.file_path.endsWith(".bin")) {
                  char report [128];
                  snprintf(report, 128, "Start firmware update\nFile name: %s\nFile size: %d",
                      msg.document.file_name, msg.document.file_size);
                  // Inform user and query for flash confirmation with password
                  myBot.sendMessage(msg, report, "");
                  msg.force_reply = true;
                  myBot.sendMessage(msg, "Please insert password", "");
              }
            }
            else {
              myBot.sendMessage(msg, "File is unavailable. Maybe size limit 20MB was reached or file deleted");
            }
            break;
        }

      case MessageReply: 
        {
            tgReply = msg.text;     
            // User has confirmed flash start with right password
            if ( tgReply.equals(fw_password) ) {
              myBot.sendMessage(msg, "Start flashing... please wait (~30/60s)");
              handleUpdate(msg, document);
              document.clear();
            }
            // Wrong password
            else {
              myBot.sendMessage(msg, "You have entered wrong password");
            }
            break;
        }

      default:
        {
            if (tgReply.equalsIgnoreCase("/version")) {
              String fw = "Version: " + String(firmware_version);
              myBot.sendMessage(msg, fw);
            }
            else {
              myBot.sendMessage(msg, "Send firmware binary file ###.bin to start update\n"
                                   "/version for print the current firmware version\n");
            }
            break;
        }
    }
  }

}



// Install firmware update
void handleUpdate(TBMessage &msg, String file_path) {

  // Create client for rom download
  WiFiClientSecure client;
  client.setInsecure();

  String report;
  Serial.print("Firmware path: ");
  Serial.println(file_path);

  ESPhttpUpdate.onProgress([](int cur, int total){
    static uint32_t sendT;
    if(millis() - sendT > 1000){
      sendT = millis();
      Serial.printf("Updating %d of %d bytes...\n", cur, total);
    }
  });

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, file_path);
  Serial.println("Update done!");
  client.stop();
  file_path.clear();
  
  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      report = "HTTP_UPDATE_FAILED Error (";
      report += ESPhttpUpdate.getLastError();
      report += "): ";
      report += ESPhttpUpdate.getLastErrorString();
      myBot.sendMessage(msg, report, "");
      break;

    case HTTP_UPDATE_NO_UPDATES:
      myBot.sendMessage(msg, "HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      myBot.begin();
      myBot.sendMessage(msg, "UPDATE OK.\nRestarting in few seconds...");

      // Wait until bot synced with telegram to prevent cyclic reboot
      while (!myBot.noNewMessage()) {
        Serial.print(".");
        delay(50);
      }
      ESP.restart();

      break;
    default:
      break;
  }

}