/*
  https://github.com/cotestatnt/AsyncTelegram2
  Name:         sendPhoto.ino
  Created:      29/03/2021
  Author:       Tolentino Cotesta <cotestatnt@yahoo.com>
  Description:  an example to show how send a picture from bot.

  Note:
  Sending image to Telegram take some time (as longer as bigger are picture files), if possible set lwIP Variant to "Higher Bandwidth" mode.
  In this example image files will be sent in tree ways and for two of them, LittleFS filesystem is required
  (SPIFFS is actually deprecated, so even if is possible, will not be supported from AsyncTelegram).
  Please follow this istructions to upload files on your board with the tool ESP8266FS
  https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system

    - with command /photofs, bot will send an example image stored in filesystem (or in an external SD)
    - with command /photohost:<host>/path/to/image, bot will send a sendPhoto command
      uploading the image file that first was downloaded from a LAN network address.
      If the file is small enough, could be stored only in memory, but for more reliability we save it before on flash.
      N.B. This can be useful in order to send images stored in local webservers, wich is not accessible from internet.
      With images hosted on public webservers, this is not necessary because Telegram can handle links and parse it properly.

    - with command /photoweb:<url>, bot will send a sendPhoto command passing the url provided
*/

// You only need to format FFat the first time you run a test
#define FORMAT_FS false

#include <AsyncTelegram2.h>

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

BearSSL::WiFiClientSecure client;
BearSSL::Session   session;
BearSSL::X509List  certificate(telegram_cert);
  
AsyncTelegram2 myBot(client);
const char* ssid = "XXXXXXXXX";     // REPLACE mySSID WITH YOUR WIFI SSID
const char* pass = "XXXXXXXXX";     // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
const char* token = "XXXXXXXXX:XXXXXXXXXXXXXXXXXXXXXXX";   // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN


// Send picture to telegram
void sendPicture(TBMessage *msg, const char* filename) {
  Serial.printf("Sending %s picture from filesystem\n", filename);
  File file = LittleFS.open(filename, "r");  
  if (!file) {
    Serial.printf("Unable to open file for reading, aborting\n");
    myBot.sendTo(msg->sender.id, "Unable to open file for reading, aborting");
    delay(1000);
    return;
  }
  myBot.sendPhotoByFile(msg->sender.id, &file, file.size());
  file.close();
}

//Example url == "http://192.168.2.81/telegram.png"
void downloadFile(String url, String fileName) {
  HTTPClient http;
  WiFiClient client;
  Serial.println(url);
  File file = LittleFS.open("/" + fileName, "w");
  if (file) {
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        http.writeToStream(&file);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    file.close();
  }
  http.end();
}


void listDir(const char * dirname) {
  Serial.printf("Listing directory: %s\n", dirname);
  Dir root = LittleFS.openDir(dirname);
  while (root.next()) {
    File file = root.openFile("r");
    Serial.print("  FILE: ");
    Serial.print(root.fileName());
    Serial.print("  SIZE: ");
    Serial.println(file.size());
    file.close();
  }
}



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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

  if (FORMAT_FS) {
    Serial.println("LittleFS formatted");
    LittleFS.format();
  }

  Serial.println("\nMount LittleFS");
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  listDir("/");

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
}



void loop() {

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library
  // N.B. sendPhoto take a lot of time (LED will not blink correctly on ESP8266 platform)
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    MessageType msgType = msg.messageType;
    
    if (msgType == MessageText) {
      
      // Received a text message
      String msgText = msg.text;
      Serial.print("\nText message received: ");
      Serial.println(msgText);

      if (msgText.equalsIgnoreCase("/photofs1")) {        
        sendPicture(&msg, "telegram-bot1.jpg");
      }

      else if (msgText.equalsIgnoreCase("/photofs2")) {        
        sendPicture(&msg, "telegram-bot2.jpg");
      }

      else if (msgText.indexOf("/photohost>") > -1 ) {
        String url = msgText.substring(msgText.indexOf("/photohost>") + sizeof("/photohost"));
        String fileName = url.substring(url.lastIndexOf('/') + 1);
        downloadFile(url, fileName);
        listDir("/");
        Serial.println("\nSending Photo from LAN: ");
        Serial.println(url);     
        sendPicture(&msg, fileName.c_str());
        LittleFS.remove("/" + fileName);
        listDir("/");
      }

      else if (msgText.indexOf("/photoweb>") > -1 ) {
        String url = msgText.substring(msgText.indexOf("/photoweb>") + sizeof("/photoweb"));
        Serial.println("\nSending Photo from web: ");
        Serial.println(url);
        myBot.sendPhotoByUrl(msg, url, url);
      }

      else {
        String replyMsg = "Welcome to the Async Telegram bot.\n\n";
        replyMsg += "/photofs1 or /photofs2 will send an example photo from fylesystem\n";
        replyMsg += "/photohost><host>/path/to/image will send a photo from your LAN\n";
        replyMsg += "/photoweb><url> will send a photo from internet\n";
        myBot.sendMessage(msg, replyMsg);
      }

    }
  }
}