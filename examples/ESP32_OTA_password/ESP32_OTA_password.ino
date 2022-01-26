/*
  Name:        OTA_password.ino
  Created:     29/03/2021
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: an example that check for incoming messages
              and install rom update remotely.
*/
// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>

#include <SPI.h>
#include <Ethernet.h>
#include <SSLClient.h>

#include "HTTPUpdate/HTTPUpdate.h"

#include <AsyncTelegram2.h>
#include <tg_certificate.h>

#define ETH_CSPIN 15
#define LED_BUILTIN 2

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 2, 177);
IPAddress myDns(192, 168, 2, 1);

EthernetClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_NONE);
AsyncTelegram2 myBot(client);

const char* token =  "xxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxx";
 int64_t chat_id = 123456789; // You can discover your own chat id, with "Json Dump Bot"

const char* firmware_version = __TIME__;
const char* fw_password = "update";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  // Sync time with NTP, to check properly Telegram certificate
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  //SPI.begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)

  pinMode(15, OUTPUT);
  pinMode(13, OUTPUT);
  SPI.begin(14, 12, 13, 15);
  Ethernet.init(ETH_CSPIN);

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true){};
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  myBot.sendTo(chat_id, welcome_msg);

  // We have to handle reboot manually after sync with TG server
  httpUpdate.rebootOnUpdate(false);
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
            tgReply = msg.text;
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

  String report;
  Serial.print("Firmware path: ");
  Serial.println(file_path);

  httpUpdate.onProgress([](int cur, int total){
    static uint32_t sendT;
    if(millis() - sendT > 1000){
      sendT = millis();
      Serial.printf("Updating %d of %d bytes...\n", cur, total);
    }
  });

  t_httpUpdate_return ret = httpUpdate.update(client, file_path);
  Serial.println("Update done!");
  client.stop();
  file_path.clear();

  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      report = "HTTP_UPDATE_FAILED Error (";
      report += httpUpdate.getLastError();
      report += "): ";
      report += httpUpdate.getLastErrorString();
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
