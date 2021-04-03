/*
  Created:     24/03/2020
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Advance example with OTA support and "event scheduler".
  With this example a weekly scheduler has been implemented that allows
  managing an indefinite number of freely programmable events.
  For example, a possible application could be a chronothermostat, or a "smart" power socket.
*/

/*
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
  With SSLClient, be sure "certificates.h" file is present in sketch folder
*/
#define USE_CLIENTSSL true

// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

#include <AsyncTelegram2.h>
#include <FS.h>
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
  #if USE_CLIENTSSL
    #include <WiFiClient.h>
    #include <SSLClient.h>
    #include "certificates.h"
    WiFiClient base_client;
    SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
  #else
    #include <WiFiClientSecure.h>
    WiFiClientSecure client;
  #endif
#endif

AsyncTelegram2 myBot(client);
const char* ssid  =  "xxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";  // Telegram token

ReplyKeyboard weekdaysKbd;
InlineKeyboard scheduleKbd;
ReplyKeyboard myKbd;

const char* update_password = "update";
const char* firmware_version = __TIME__;

// Split scheduler functions in a separate .h file
#include "ev_scheduler.h"

// Split firmware update functions in a separate .h file
#include "fw_updater.h"

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

  Serial.print("\n\nStart connection to WiFi...");
  // connects to access point
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

  if (!FILESYSTEM.begin()) {
    Serial.println("Unable to mount filesystem, check selected partition scheme");
  }
  loadConfigFile();

#ifdef ESP8266
  // We have to handle reboot manually after sync with TG server
  ESPhttpUpdate.rebootOnUpdate(false);
  // Sync time with NTP (don't skip this step, secure connection needs synced clock)
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  // Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(TCP_MSS, TCP_MSS);
#elif defined(ESP32)
  httpUpdate.rebootOnUpdate(false);
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #if USE_CLIENTSSL == false
    client.setCACert(telegram_cert);
  #endif
#endif

  // Now is possible start Telegram connection
  myBot.setUpdateTime(2000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  getSchedulerKeyboard(&scheduleKbd);
  getWeekdayKeyboard(&weekdaysKbd);
  getCommandKeyboard(&myKbd);

  const char* botName = myBot.getBotName();
  Serial.printf("Nome del bot: @%s", botName);

  char welcome_msg[128];
  snprintf(welcome_msg, 128, PSTR("BOT @%s online\n/help all commands avalaible."), botName);

  int32_t chat_id = 436865110; // You can discover your own chat id, with "Json Dump Bot"
  myBot.sendTo(chat_id, welcome_msg);
}



void loop() {
  if(do_restart)
    doRestart();

  printHeapStats();

  // get updated timedate and check if there is any active event
  time_t epoch = time(nullptr);
  tm now = *gmtime(&epoch);
  uint32_t startOfDay = mktime(&now) - now.tm_hour*3600UL  -  now.tm_min*60UL - now.tm_sec;

  for(uint8_t i=0; i< lastEvent; i++){
    // For each event stored we evaluate whether it is necessary to activate
    if (startOfDay + events[i].start == (uint32_t) epoch
        && bitRead(events[i].days, now.tm_wday) )
        //&& events[i].setpoint > actual_temperature
    {
      events[i].active = true;  // Now we can use this as trigger for any need
      delay(500);
      Serial.printf("\nEvent n. %d actvivated", i+1);
    }

    if (startOfDay + events[i].stop == (uint32_t) epoch) {
      events[i].active = false;
      delay(500);
      Serial.printf("\nEvent n. %d deactvivated", i+1);
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
    // Is this message intended for firmware update handler?
    bool msgParsed =  updateHandler(&msg, &myBot);

    // maybe is for events scheduler handler?
    if(! msgParsed)
      msgParsed = schedulerHandler(&msg, &myBot);

    // none of the above, process the message received for remaining cases
    if (! msgParsed){
      String txtMessage = msg.text;
      Serial.print("\nMessagge: ");
      Serial.println(txtMessage);

      // Send back to user current time
      if (txtMessage.equalsIgnoreCase("/time")) {
          time_t epoch = time(nullptr);
          now = *localtime(&epoch);
          char buffer [40];
          strftime (buffer, 40,"%c", &now);
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
          myBot.sendMessage(msg,"To update firmware, upload compiled file ########.bin \n"
                                "System:\n/restart for restart MCU\n"
                                "/version for print the current firmware version\n"
                                "/time per conoscere la mia ora locale\n\nScheduler:\n"
                                "/clear delete all scheduled events \n\n"
                                "/list print all events in memory\n"
                                "/edit to edit a specific event\n"
                                "/add to add new event to list", myKbd);
      }
      // None of the above
      else {
          myBot.sendMessage(msg, "Command not supported. Type /help for complete list");
      }
    }

  }
}


bool loadConfigFile(){
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

  for(uint8_t i=0; i<MAX_EVENTS; i++){
    String evt_name = "event";
    evt_name += i + 1;
    if(doc[evt_name]) {
      events[i].start = doc[evt_name]["start"];
      events[i].stop = doc[evt_name]["stop"];
      events[i].days = doc[evt_name]["days"];
      events[i].setpoint = doc[evt_name]["setpoint"];
      lastEvent++;
    }
  }

  file.close();
  return true;
}


void saveConfigFile(){
  // Open file for writing
  File file = FILESYSTEM.open(filename, "w");
  if (file)
    Serial.println(F("Configuration file created"));
  else
    return;

  // Serialize JSON to file
  StaticJsonDocument<1024> doc;
  JsonObject root = doc.to<JsonObject>();

  for(uint8_t i=0; i<lastEvent; i++){
    String evt_name = "event";
    evt_name += i+1;
    JsonObject evt = root.createNestedObject(evt_name);
    evt["start"] = events[i].start;
    evt["stop"] = events[i].stop;
    evt["days"] = events[i].days;
    evt["setpoint"] = events[i].setpoint;
  }

  serializeJsonPretty(root, Serial);
  if (serializeJson(root, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

void doRestart(){
  do_restart = false;
  // Wait until bot synced with telegram to prevent cyclic reboot
  while (! myBot.noNewMessage()) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Restart in 5 seconds...");
  delay(5000);
  ESP.restart();
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
