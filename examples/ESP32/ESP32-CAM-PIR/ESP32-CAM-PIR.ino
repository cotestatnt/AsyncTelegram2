#include <WiFi.h>
#include <FS.h>
#include <AsyncTelegram2.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix

// Local include files
#include "camera_pins.h"

#define USE_MMC true          // Define where store images (on board SD card reader or internal flash memory)
#if USE_MMC
#include <SD_MMC.h>           // Use onboard SD Card reader
#define FILESYSTEM SD_MMC
#else
#include <FFat.h>              // Use internal flash memory
#define FILESYSTEM FFat        // Be sure to select the correct filesystem in IDE option
#endif

#include <WiFiClientSecure.h>
WiFiClientSecure client;

#define PIR_PIN   GPIO_NUM_13

const char* ssid = "xxxxxxxxx";                                        // SSID WiFi network
const char* pass = "xxxxxxxxx";                                        // Password  WiFi network
const char* token = "xxxxxxxxxx:xxxxxxxxxxxxxxx-xxxxxxxxxxxxxxxxxxx";  // Telegram token
// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;


#define PIR_PIN       GPIO_NUM_13
#define NUM_PHOTO     3               // Total number of photos to send on motion detection
#define DELAY_PHOTO   2000            // Waiting time between one photo and the next 

int currentPict = 3;
AsyncTelegram2 myBot(client);

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

// Struct for saving time datas (needed for time-naming the image files)
struct tm tInfo;

// Functions prototype
void listDir(const char *, uint8_t, bool) ;
void printHeapStats();
static void checkTelegram(void *);
void setLamp(int);
size_t sendPicture(bool, int64_t);

///////////////////////////////////  SETUP  ///////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);       // disable brownout detect
  
  // PIR Motion Sensor setup
  pinMode(PIR_PIN, INPUT_PULLUP);         
  
  // Flash LED setup
  pinMode(LAMP_PIN, OUTPUT);                       // set the lamp pin as output
  ledcSetup(lampChannel, pwmfreq, pwmresolution);  // configure LED PWM channel
  setLamp(0);                                      // set default value
  ledcAttachPin(LAMP_PIN, lampChannel);            // attach the GPIO pin to the channel

  // Start WiFi connection
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Init filesystem (format if necessary)
  if (!FILESYSTEM.begin("/sdcard", true, true)) {
    #ifndef USE_MMC
    Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");    
    FILESYSTEM.format();
    #else
    Serial.println("\nSD Mount Failed.\n");    
    #endif
  }
  uint32_t freeBytes =  FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes();
  Serial.print("\nTotal space: "); Serial.println(FILESYSTEM.totalBytes());
  Serial.print("Free space: ");  Serial.println(freeBytes);
  listDir("/", 0, false);

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  client.setCACert(telegram_cert);

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online.\nTry with /takePhoto or /savePhoto", myBot.getBotName());
  myBot.sendTo(userid, welcome_msg);

  // Start telegram message check task
  xTaskCreatePinnedToCore(
    checkTelegram,    // Function to implement the task
    "checkTelegram",  // Label name of the task
    16384,            // Stack size
    NULL,             // Task input parameter
    1,                // Priority of the task
    NULL,             // Task handle.
    1                 // Core number
  );

  // Init the camera module (accordind the camera_config_t defined)
  init_camera();
}


///////////////////////////////////  LOOP  ///////////////////////////////////////
void loop() {
  // printHeapStats();
}


//////////////////////////////////  FUNCTIONS//////////////////////////////////////

// This is the task for checking new messages from Telegram
static void checkTelegram(void * args) {
  while (true) {
    static uint32_t waitPhotoTime;
    
    // PIR motion detected 
    // could be on interrupt, but PIR sensor usually keep signal HIGH for enough time
    if(digitalRead(PIR_PIN) == LOW) {   
      currentPict = 0;
      waitPhotoTime = 0;
      char message[50];
      time_t now = time(nullptr);
      tInfo = *localtime(&now);
      strftime(message, 50, "%d/%m/%Y %H:%M:%S - Motion detected!", &tInfo);
      Serial.println(message);
      myBot.sendTo(userid, message);
    }
    
    if(millis() - waitPhotoTime > DELAY_PHOTO && currentPict < NUM_PHOTO) {
      waitPhotoTime = millis();
      size_t bytes_sent = sendPicture(true, userid);
      if (bytes_sent) {
        Serial.printf("CAM picture sent to Telegram (%d bytes)\n", bytes_sent);
        currentPict++;
      }
    }

    // A variable to store telegram message data
    TBMessage msg;
    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
      Serial.print("New message from chat_id: ");
      Serial.println(msg.chatId);
      MessageType msgType = msg.messageType;

      // Received a text message
      if (msgType == MessageText) {

        // Send a picture grabbed from camera directly to Telegram
        if (msg.text.equalsIgnoreCase("/takePhoto")) {
          uint32_t t1 = millis();
          size_t bytes_sent = sendPicture(false, msg.chatId);
          if (bytes_sent) Serial.printf("Picture sent to Telegram (%d bytes)\n", bytes_sent);
          
          Serial.printf("Total upload time (server latency time included, ~ 500 ms): %lu ms\n", millis() - t1 );
        }

        // Save a picture and then send to Telegram
        else if (msg.text.equalsIgnoreCase("/savePhoto")) {
          size_t bytes_sent = sendPicture(true, msg.chatId);
          if (bytes_sent) Serial.printf("Picture saved and sent to Telegram (%d bytes)\n", bytes_sent);
        }

        // Delete all files in folder
        else if (msg.text.equalsIgnoreCase("/deleteAll")) {
          listDir("/", 0, true);
          myBot.sendMessage(msg, "All files deleted.");
        }

        // Delete all files in folder
        else if (msg.text.equalsIgnoreCase("/listDir")) {
          listDir("/", 0, false);
          myBot.sendMessage(msg, "All files listed on Serial port.");
        }

        // Echo received message and send command list
        else {
          Serial.print("\nText message received: ");
          Serial.println(msg.text);
          String replyStr = "Message received:\n";
          replyStr += msg.text;
          replyStr +=  "\nTry with /takePhoto or /savePhoto";
          myBot.sendMessage(msg, replyStr);
        }
      }
    }
    yield();
  }
  // Delete this task on exit (should never occurs)
  vTaskDelete(NULL);
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

// Send a picture taken from CAM to a Telegram chat
size_t sendPicture(bool saveImg, int64_t chatId) {
  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take Picture with Camera and store in ram buffer fb
  setLamp(100);
  camera_fb_t* fb = esp_camera_fb_get();
  setLamp(0);
  if (!fb) {
    Serial.println("Camera capture failed");
    return 0;
  }
  size_t len = fb->len;

  // If is necessary keep the image file, save and send as stream object
  if (saveImg) {
    // Keep files on SD memory, filename is time based (YYYYMMDD_HHMMSS.jpg)
    #if USE_MMC
      char filename[30];
      time_t now = time(nullptr);
      tInfo = *localtime(&now);
      strftime(filename, 30, "/%Y%m%d_%H%M%S.jpg", &tInfo);
    #else
      // Embedded filesystem is too small, overwrite the same file
      const char* filename = "/image.jpg";
    #endif

    File file = FILESYSTEM.open(filename, "w");
    if (!file) {
      Serial.println("Failed to open file in writing mode");
      return 0;
    }
    file.write(fb->buf, fb->len);
    file.close();
    Serial.printf("Saved file to path: %s - %zu bytes\n", filename, fb->len);
    myBot.sendPhoto(chatId, filename, FILESYSTEM);
  }

  // If is NOT necessary keep the image file, send picture directly from ram buffer
  else {
    myBot.sendPhoto(chatId, fb->buf, fb->len);
  }

  // Clear buffer
  esp_camera_fb_return(fb);
  return len;
}


// Just to check if everithing work as expected
void printHeapStats() {
  time_t now = time(nullptr);
  tInfo = *localtime(&now);
  static uint32_t infoTime;
  if (millis() - infoTime > 10000) {
    infoTime = millis();
    Serial.printf("%02d:%02d:%02d - Total free: %6d - Max block: %6d\n",
                  tInfo.tm_hour, tInfo.tm_min, tInfo.tm_sec, heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0));
  }
}

// List all files saved in the selected filesystem
void listDir(const char * dirname, uint8_t levels, bool deleteAllFiles = false) {
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
    else {
      Serial.printf("  FILE: %s\tSIZE: %d", file.name(), file.size());
      if (deleteAllFiles) {
        FILESYSTEM.remove(file.name());
        Serial.print(" deleted!");
      }
      Serial.println();
    }
    file = root.openNextFile();
  }
}
