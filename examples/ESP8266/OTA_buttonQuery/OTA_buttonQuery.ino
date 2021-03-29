/*
  Name:	       OTA_buttonQuery.ino
  Created:     29/03/2021
  Author:       Tolentino Cotesta <cotestatnt@yahoo.com>
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

#define CANCEL  "CANCEL"
#define CONFIRM "FLASH_FW"

const char* firmware_version = __TIME__;

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
  client.setBufferSizes(TCP_MSS, TCP_MSS);
  
  // Set the Telegram bot properies
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  
  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  int32_t chat_id = 436865110; // You can discover your own chat id, with "Json Dump Bot"
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
    
    switch (msg.messageType) 
    {
      case MessageDocument :
        document = msg.document.file_path;
        if (msg.document.file_exists) {
            
            // Check file extension of received document (firmware must be .bin)
            if(document.endsWith(".bin") > -1 ){
                String report = "Start firmware update?\nFile name: "
                                + String(msg.document.file_name)
                                + "\nFile size: "
                                + String(msg.document.file_size);
        
                // Query user for flash confirmation
                InlineKeyboard confirmKbd;
                confirmKbd.addButton("FLASH", CONFIRM, KeyboardButtonQuery);
                confirmKbd.addButton("CANCEL", CANCEL, KeyboardButtonQuery);            
                myBot.sendMessage(msg, report.c_str(), confirmKbd);
            }
        } 
        else {
            myBot.sendMessage(msg, "File is unavailable. Maybe size limit 20MB was reached or file deleted");
        }
        break;
    
      case MessageQuery:
        // received a callback query message
        tgReply = msg.callbackQueryData;
    
        // User has confirmed flash start
        if (tgReply.equalsIgnoreCase(CONFIRM)) {            
            myBot.endQuery(msg, "Start flashing... please wait (~30/60s)", true);
            handleUpdate(msg, document);
            document.clear();
        }
        // User has canceled the command
        else if (tgReply.equalsIgnoreCase(CANCEL)) {
            myBot.endQuery(msg, "Flashing canceled");
        }
        break;
    
      default:
        tgReply = msg.text;
        if (tgReply.equalsIgnoreCase("/version")) {
            String fw = "Version: " ;
            fw += firmware_version;
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



// Install firmware update
void handleUpdate(TBMessage msg, String &file_path) {

  // Create client for rom download
  WiFiClientSecure client;
  client.setInsecure();
 
  String report;
  Serial.print("Firmware path: ");
  Serial.println(file_path);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, file_path);
  Serial.println("Update done!");
  client.stop();
  file_path.clear();

  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      report = "HTTP_UPDATE_FAILED Error ("
               + ESPhttpUpdate.getLastError();
               + "): "
               + ESPhttpUpdate.getLastErrorString();
      myBot.sendMessage(msg, report.c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      myBot.sendMessage(msg, "HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      myBot.sendMessage(msg, "UPDATE OK.\nRestarting...");
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