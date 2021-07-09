/*
  https://github.com/cotestatnt/AsyncTelegram2
  Name:         sendPhoto.ino
  Created:      09/07/2021
  Author:       Tolentino Cotesta <cotestatnt@yahoo.com>
  Description:  an example to show how send a picture from bot.

  Note: 
  Sending image to Telegram take some time (as longer as bigger are picture files)
    - with command /photofs, bot will send an example image stored in filesystem (or in an external SD)      
    - with command /photoweb:<url>, bot will send a sendPhoto command passing the url provided
*/
#include <FS.h>
#include <AsyncTelegram2.h>

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

/* 
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure 
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
*/ 
#define USE_CLIENTSSL false  
#define FORMAT_FS_IF_FAILED true

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <LittleFS.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
  #define FILESYSTEM LittleFS      
#elif defined(ESP32)
  // Be sure to select the correct filesystem in IDE option 
  #define USE_FFAT false
  #if USE_FFAT
    #include <FFat.h>           
    #define FILESYSTEM FFat
  #else
    #include <SPIFFS.h>           
    #define FILESYSTEM SPIFFS       
  #endif
  #include <WiFi.h>
  #include <WiFiClient.h>
  #if USE_CLIENTSSL
    #include <SSLClient.h>      //https://github.com/OPEnSLab-OSU/SSLClient/
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
const char* pass  =  "xxxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 1234567890;

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
const uint8_t LED = LED_BUILTIN;

// List all files saved in the selected filesystem
void listDir(const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = FILESYSTEM.open(dirname, "r");
  if (!root) {
    Serial.println("- failed to open directory\n");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory\n");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\n", file.name());
      if (levels)
        listDir(file.name(), levels - 1);
    }
    else
      Serial.printf("  FILE: %s\tSIZE: %d\n", file.name(), file.size());
    file = root.openNextFile();
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
    Serial.printf("%02d:%02d:%02d - Total free: %6d - Max block: %6d\n",
      tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0) );
#elif defined(ESP8266)
    uint32_t free;
    uint16_t max;
    ESP.getHeapStats(&free, &max, nullptr);
    Serial.printf("%02d:%02d:%02d - Total free: %5d - Max block: %5d\n",
      tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, free, max);
#endif
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);  
  // initialize the Serial
  Serial.begin(115200);

  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  // connects to access point
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Init filesystem (format if necessary)
  if (!FILESYSTEM.begin()) {
    Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
    #if FORMAT_FS_IF_FAILED
    FILESYSTEM.format();
    ESP.restart();
    #endif
  }
  listDir("/", 0);

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

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  char welcome_msg[64];
  snprintf(welcome_msg, 64, "BOT @%s online.\n/help for command list.", myBot.getBotName());

  // Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
  // https://t.me/JsonDumpBot  or  https://t.me/getidsbot
  myBot.sendTo(userid, welcome_msg); 

}



void loop() {

  printHeapStats();

  // In the meantime LED_BUILTIN will blink with a fixed frequency
  // to evaluate async and non-blocking working of library  
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {        
    Serial.print("New message from chat_id: ");
    Serial.println(msg.sender.id);
    MessageType msgType = msg.messageType;

    // Received a text message
    if (msgType == MessageText){
      String msgText = msg.text;      
      Serial.print("\nText message received: ");
      Serial.println(msgText);

      // Send picture stored in filesystem passing only filename and filesystem type
      if (msgText.equalsIgnoreCase("/picfs1")) {
        Serial.println("\nSending picture 1 from filesystem");          
        myBot.sendPhoto(msg.sender.id, "/telegram-bot1.jpg", FILESYSTEM);          
      }
      
      // Send picture stored in filesystem passing the stream 
      // (File is a class derived from Stream)
      else if (msgText.equalsIgnoreCase("/picfs2")) {
        Serial.println("\nSending picture 2 from filesystem");                   
        File file = FILESYSTEM.open("/telegram-bot2.jpg", "r");
        myBot.sendPhoto(msg.sender.id, file, file.size());
        file.close();               
      }

      // Send a picture passing url to online file
      else if (msgText.indexOf("/picweb") > -1) {          
        String url = msgText.substring(msgText.indexOf("/picweb ") + sizeof("/picweb "));                   
        Serial.print("\nSending picture from web: "); 
        Serial.println(url);          
        if(url.length()) 
            myBot.sendPhoto(msg, url, url);                   
      }

      else {
        String replyMsg = "Welcome to the Async Telegram bot.\n\n";
        replyMsg += "/picfs1 or /picfs2 will send an example picture from fylesystem\n";         
        replyMsg += "/picweb <b>https://telegram.org/img/t_logo.svg</b> will send a picture from internet\n";      
        msg.isHTMLenabled = true;
        myBot.sendMessage(msg, replyMsg);    
      }
      
    }
  }
}