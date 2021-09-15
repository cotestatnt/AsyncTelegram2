/*
  Created:     01/09/2021
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Advance example with remote programmable multiple timers.
  With this example a weekly scheduler has been implemented that allows
  managing an indefinite number of freely programmable timers.
  For example, a possible application could be a chronothermostat, or a "smart" power socket.
*/

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
#include <time.h>
#include <FS.h>
#include <AsyncTelegram2.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  #include <LittleFS.h>
  #define FILESYSTEM LittleFS
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
#elif defined(ESP32)
  #include <WiFi.h>
  #include <SPIFFS.h>
  #define FILESYSTEM SPIFFS
  #include <WiFiClientSecure.h>
  WiFiClientSecure client;
#endif

AsyncTelegram2 myBot(client);
const char* ssid  =  "xxxxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 1234567890;

const char* firmware_version = __TIME__;

// Split scheduler functions in a separate .h file
#include "scheduler.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Scheduled events will be saved in flash memory
bool saveConfig = false;
const char *filename = "/config.txt";
bool do_restart = false;

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  Serial.print(F("\n\nStart connection to WiFi..."));
  // connects to access point
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  if (!FILESYSTEM.begin()) {
    Serial.println(F("Unable to mount filesystem, check selected partition scheme"));
    FILESYSTEM.format();
    delay(1000);
    ESP.restart();
  }
  loadConfigFile();

#ifdef ESP8266
  // Sync time with NTP (don't skip this step, secure connection needs synced clock)
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  // Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  client.setCACert(telegram_cert);
#endif

  // Now is possible start Telegram connection
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print(F("\nTest Telegram connection... "));
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  myBot.sendTo(userid, welcome_msg);
}



void loop() {
  if (do_restart)
    doRestart();

  printHeapStats();

  // get updated timedate and check if there is any active event
  time_t epoch = time(nullptr);
  tm now = *gmtime(&epoch);
  uint32_t startOfDay = mktime(&now) - now.tm_hour * 3600UL  -  now.tm_min * 60UL - now.tm_sec;

  for (uint8_t i = 0; i < lastEvent; i++) {
    // For each event stored we evaluate whether it is necessary to activate
    if (startOfDay + events[i].start == (uint32_t) epoch
        && bitRead(events[i].days, now.tm_wday) )
    {
      events[i].active = true;  // Now we can use this as trigger for any need
      delay(500);
      Serial.printf("\nEvent n. %d actvivated\n", i + 1);
    }

    if (startOfDay + events[i].stop == (uint32_t) epoch) {
      events[i].active = false;
      delay(500);
      Serial.printf("\nEvent n. %d deactvivated\n", i + 1);
    }
  }

  // LED blinking
  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // check if there is a new incoming message and parse the content
  TBMessage msg;
  if (myBot.getNewMessage(msg)) {

    // Is this message intended for scheduler handler?
    bool msgParsed =  schedulerHandler(msg, myBot);

    // nope, process the message received for remaining cases
    if (! msgParsed) {
      String txtMessage = msg.text;
      Serial.print(F("\nMessagge: "));
      Serial.println(txtMessage);

      // Send back to user current time
      if (txtMessage.equalsIgnoreCase("/time")) {
        time_t epoch = time(nullptr);
        now = *localtime(&epoch);
        char buffer [40];
        strftime (buffer, 40, "%c", &now);
        myBot.sendMessage(msg, buffer);
      }

      // Inform the user about actual firmware version
      else if (txtMessage.equalsIgnoreCase("/version")) {
        char fw_info[30];
        snprintf(fw_info, 30, "Firmware version: %s", firmware_version);
        myBot.sendMessage(msg, fw_info);
      }

      // User request a restart
      else if (txtMessage.equalsIgnoreCase("/restart")) {
        myBot.sendMessage(msg, "Going to restart MCU...");
        do_restart = true;
      }

      // User request a list of supported commands
      else if (txtMessage.equalsIgnoreCase("/help")) {
        myBot.sendMessage(msg,
            "System:\n/restart for restart MCU\n"
            "/version for print the current firmware version\n"
            "/time for local system timedate\n\nScheduler:\n"
            "/clear delete all scheduled events \n\n"
            "/list print all events in memory\n"
            "/edit to edit a specific event\n"
            "/add to add new event to list");
      }

      // Not a supported command
      else {
        myBot.sendMessage(msg, "Command not supported. Type /help for complete list");
      }
    }

  }
}


bool loadConfigFile() {
  // Open file for reading
  File file = FILESYSTEM.open(filename, "r");
  if (!file) {
    Serial.println(F("Fail to open configuration file"));
    return false;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Errore"));

  serializeJsonPretty(doc, Serial);
  Serial.println();
  for (uint8_t i = 0; i < MAX_EVENTS; i++) {
    String evt_name = "event";
    evt_name += i + 1;
    if (doc[evt_name]) {
      events[i].start = doc[evt_name]["start"];
      events[i].stop = doc[evt_name]["stop"];
      events[i].days = doc[evt_name]["days"];
      lastEvent++;
    }
  }

  file.close();
  return true;
}


void saveConfigFile() {
  // Open file for writing
  File file = FILESYSTEM.open(filename, "w");
  if (file)
    Serial.println(F("Configuration file created"));
  else
    return;

  // Serialize JSON to file
  StaticJsonDocument<1024> doc;
  JsonObject root = doc.to<JsonObject>();

  for (uint8_t i = 0; i < lastEvent; i++) {
    String evt_name = "event";
    evt_name += i + 1;
    JsonObject evt = root.createNestedObject(evt_name);
    evt["start"] = events[i].start;
    evt["stop"] = events[i].stop;
    evt["days"] = events[i].days;
  }

  serializeJsonPretty(root, Serial);
  Serial.println();
  if (serializeJson(root, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

void doRestart() {
  do_restart = false;
  // Wait until bot synced with telegram to prevent cyclic reboot
  while (! myBot.noNewMessage()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println(F("Restart in 5 seconds..."));
  delay(5000);
  ESP.restart();
}

void printHeapStats() {
  time_t now = time(nullptr);
  struct tm tInfo = *localtime(&now);
  static uint32_t infoTime;
  if (millis() - infoTime > 5000) {
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
