#include <WiFi.h>
#include <FS.h>
#include <AsyncTelegram2.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix

// Local include files
#include "camera_pins.h"

#define USE_SSLCLIENT true
#if USE_SSLCLIENT
#include <SSLClient.h>
#include "tg_certificate.h"
WiFiClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0);
#else
#include <WiFiClientSecure.h>
WiFiClientSecure client;
#endif

#define FILENAME_SIZE 25
#define FORMAT_FS_IF_FAILED true
#define DELETE_IMAGE true   // Set fault only if you have an SD or a very big flash memory.
//#define USE_MMC true      // Define where store images (on board SD card reader or internal flash memory)

#ifdef USE_MMC
#include <SD_MMC.h>           // Use onboard SD Card reader
#define FILESYSTEM SD_MMC
#else
#include <FFat.h>              // Use internal flash memory
#define FILESYSTEM FFat        // Be sure to select the correct filesystem in IDE option 
#endif

const char* ssid = "xxxxxxxxxxxx";  // SSID WiFi network
const char* pass = "xxxxxxxxxxxx";  // Password  WiFi network
const char* token = "xxxxxxxx:xxxxxxxxxxx-xxxxxxxxxxxxxxxxxxx";

AsyncTelegram2 myBot(client);

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

// Struct for saving time datas (needed for time-naming the image files)
struct tm timeinfo;

// Just to check if everithing work as expected
void printHeapStats() {
  time_t now = time(nullptr);
  struct tm tInfo = *localtime(&now);
  static uint32_t infoTime;
  if (millis() - infoTime > 10000) {
    infoTime = millis();
    Serial.printf("%02d:%02d:%02d - Total free: %6d - Max block: %6d\n",
                  tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0));
  }
}

// List all files saved in the selected filesystem
void listDir(const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = FILESYSTEM.open(dirname);
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

// Lamp Control
void setLamp(int newVal) {
  if (newVal != -1) {
    // Apply a logarithmic function to the scale.
    int brightness = round((pow(2, (1 + (newVal * 0.02))) - 2) / 6 * pwmMax);
    ledcWrite(lampChannel, brightness);
    Serial.print("Lamp: ");
    Serial.print(newVal);
    Serial.print("%, pwm = ");
    Serial.println(brightness);
  }
}


// Send a picture taken from CAM to Telegram
bool sendPicture(TBMessage* msg) {
  // Path where new picture will be saved "/YYYYMMDD_HHMMSS.jpg"
  char picturePath[FILENAME_SIZE];
  getLocalTime(&timeinfo);
  snprintf(picturePath, FILENAME_SIZE, "/%02d%02d%02d_%02d%02d%02d.jpg", timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  File file = FILESYSTEM.open(picturePath, "w");
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return false;
  }
  Serial.println("Capture Requested");

  // Take Picture with Camera;
  setLamp(100);
  camera_fb_t* fb = esp_camera_fb_get();
  setLamp(0);
  if (!fb) {
    Serial.println("Camera capture failed");
    file.close();
    FILESYSTEM.remove(picturePath);
    return false;
  }

  file.write(fb->buf, fb->len);  // payload (image), payload length
  file.close();
  Serial.printf("Saved file to path: %s - %zu bytes\n", picturePath, fb->len);

  // Open again in reading mode and send stream to AyncTelegram
  file = FILESYSTEM.open(picturePath, "r");
  myBot.sendPhotoByFile(msg->sender.id, &file, file.size());
  file.close();
  //If you don't need to keep image, delete from filesystem
#if DELETE_IMAGE
  FILESYSTEM.remove(picturePath);
#endif
  return true;
}


// This is the task for checking new messages from Telegram
static void checkTelegram(void * args) {
  while (true) {
    // A variable to store telegram message data
    TBMessage msg;
    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
      Serial.print("New message from chat_id: ");
      Serial.println(msg.sender.id);
      MessageType msgType = msg.messageType;

      if (msgType == MessageText) {
        // Received a text message
        if (msg.text.equalsIgnoreCase("/takePhoto")) {
          Serial.println("\nSending Photo from CAM");
          if (sendPicture(&msg))
            Serial.println("Message sent");
        }
        else {
          Serial.print("\nText message received: ");
          Serial.println(msg.text);
          String replyStr = "Message received:\n";
          replyStr += msg.text;
          replyStr +=  "\nTry with /takePhoto";
          myBot.sendMessage(msg, replyStr);
        }
      }
    }
    yield();
  }
  // Delete this task on exit (should never occurs)
  vTaskDelete(NULL);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);       // disable brownout detector

  pinMode(LAMP_PIN, OUTPUT);                       // set the lamp pin as output
  ledcSetup(lampChannel, pwmfreq, pwmresolution);  // configure LED PWM channel
  setLamp(0);                                      // set default value
  ledcAttachPin(LAMP_PIN, lampChannel);            // attach the GPIO pin to the channel

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Init filesystem
#ifdef USE_MMC
  if (!SD_MMC.begin( "/sd", false))
    Serial.println("SD Card Mount Failed");
  if (SD_MMC.cardType() == CARD_NONE)
    Serial.println("No SD Card attached");
#else
  // Init filesystem (format if necessary)
  if (!FILESYSTEM.begin(FORMAT_FS_IF_FAILED)) {
    Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
    FILESYSTEM.format();
  }
  uint32_t freeBytes =  FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes();
  Serial.printf("\nTotal space: %10d\n", FILESYSTEM.totalBytes());
  Serial.printf("Free space: %10d\n", freeBytes);
#endif

  listDir("/", 0);

  // Start WiFi connection
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
#if USE_SSLCLIENT == false
  client.setCACert(telegram_cert);
#endif

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  char welcome_msg[64];
  snprintf(welcome_msg, 64, "BOT @%s online.\nTry with /takePhoto command.", myBot.getBotName());

  // Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
  // https://t.me/JsonDumpBot  or  https://t.me/getidsbot
  int32_t userid = 436865110;
  myBot.sendTo(userid, welcome_msg);

  // Start telegram message checking in a separate task on core 0 (the loop() function run on core 1)
  xTaskCreate(
    checkTelegram,    // Function to implement the task
    "checkTelegram",  // Name of the task
    8192,             // Stack size in words
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL             // Task handle.
  );
  
  // Init the camera module (accordind the camera_config_t defined)
  init_camera();
}

void loop() {
  printHeapStats();
}
