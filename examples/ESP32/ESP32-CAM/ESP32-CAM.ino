/*
 Name:          echoBot.ino
 Created:     20/06/2020
 Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
 Description: an example that show how is possible send an image captured from a ESP32-CAM board
*/

//                                             WARNING!!!
// Make sure that you have selected ESP32 Wrover Module, or another board which has PSRAM enabled


// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE

#include "camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix

// Define where store images (on board SD card reader or internal flash memory)
#include <FS.h>
//#define USE_MMC true
#ifdef USE_MMC
    #include <SD_MMC.h>           // Use onboard SD Card reader
    fs::FS &filesystem = SD_MMC;
#else
    #include <FFat.h>              // Use internal flash memory
    fs::FS &filesystem = FFat;     // Is necessary select the proper partition scheme (ex. "Default 4MB with ffta..")
#endif

// You only need to format FFat when error on mount (don't work with MMC SD card)
#define FORMAT_FS_IF_FAILED true
#define FILENAME_SIZE 25
#define DELETE_IMAGE true

#include <WiFi.h>
#include <WiFiClient.h>
#include <SSLClient.h>
#include <AsyncTelegram2.h>

const char* ssid  =  "PuccosNET";     // SSID WiFi network
const char* pass  =  "Tole76tnt";     // Password  WiFi network
const char* token =  "1693205624:AAFuMQ1E2smMNQfMcPikuokzwxgpvNBzJwg";

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

WiFiClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0);

AsyncTelegram2 myBot(client);

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

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
void listDir(const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = filesystem.open(dirname);
    if(!root){
        Serial.println("- failed to open directory\n");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory\n");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.printf("  DIR : %s\n", file.name());
            if(levels)
                listDir(file.name(), levels -1);
        }
        else
            Serial.printf("  FILE: %s\tSIZE: %d\n", file.name(), file.size());
        file = root.openNextFile();
    }
}


camera_fb_t * fb = NULL;

// Send a picture taken from CAM to Telegram
bool sendPicture( TBMessage *msg, framesize_t frameSize, int jpeg_quality){
    esp_camera_deinit();
    cameraSetup(frameSize, jpeg_quality);

    // Path where new picture will be saved "/YYYYMMDD_HHMMSS.jpg"
    char picturePath[FILENAME_SIZE];
    getLocalTime(&timeinfo);
    snprintf(picturePath, FILENAME_SIZE, "/%02d%02d%02d_%02d%02d%02d.jpg", timeinfo.tm_year +1900,
             timeinfo.tm_mon +1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    File file = filesystem.open(picturePath, "w");
    if (!file) {
        Serial.println("Failed to open file in writing mode");
        return false;
    }
    Serial.println("Capture Requested");

    if (autoLamp && (lampVal != -1)) setLamp(100);
    // Take Picture with Camera;
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        if (autoLamp && (lampVal != -1)) setLamp(0);
        return false;
    }
    if (autoLamp && (lampVal != -1)) setLamp(0);

    // Save picture to filesystem
#ifdef USE_MMC
    uint64_t freeBytes =  SD_MMC.totalBytes() - SD_MMC.usedBytes();
#else
    uint64_t freeBytes =  FFat.freeBytes();
#endif

    if (freeBytes > fb->len ) {
        file.write(fb->buf, fb->len); // payload (image), payload length
        file.close();
        Serial.printf("Saved file to path: %s - %zu bytes\n", picturePath, fb->len);
    }
    else
        Serial.println("Not enough space avalaible");
    // free(out_buf);
    // esp_camera_fb_return(fb);

    // Open again in reading mode and send stream to AyncTelegram
    file = filesystem.open(picturePath, "r");
    myBot.sendPhotoByFile(msg->sender.id, &file, file.size());
    file.close();
    //If you don't need to keep image, delete from filesystem
    #if DELETE_IMAGE
        filesystem.remove(picturePath);
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

        if (msgType == MessageText){
            // Received a text message
            if (msg.text.equalsIgnoreCase("/takePhoto")) {
                Serial.println("\nSending Photo from CAM");
                sendPicture(&msg, FRAMESIZE_UXGA, 25);
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
    vTaskDelay(10);
  }
  // Delete this task on exit (should never occurs)
  vTaskDelete(NULL);
}

void setup() {
    //disable brownout detector
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();

    // Initialise and set the lamp
    if (lampVal != -1) {
        ledcSetup(lampChannel, pwmfreq, pwmresolution);  // configure LED PWM channel
        if (autoLamp) setLamp(0);                        // set default value
        else setLamp(lampVal);
        ledcAttachPin(LAMP_PIN, lampChannel);            // attach the GPIO pin to the channel
    } else {
        Serial.println("No lamp, or lamp disabled in config");
    }
    cameraSetup(FRAMESIZE_SXGA, 15);

    // Init WiFi connections
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nWiFi connected: ");
    Serial.print(WiFi.localIP());

    // Sync time with NTP. Blocking, but with timeout (0 == no timeout)
    configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");

    // Init filesystem
#ifdef USE_MMC
    if(!SD_MMC.begin( "/sd", false))
        Serial.println("SD Card Mount Failed");
    if(SD_MMC.cardType() == CARD_NONE)
        Serial.println("No SD Card attached");
    Serial.printf("\nTotal space: %10llu\n", SD_MMC.totalBytes());
    Serial.printf("Free space: %10llu\n", SD_MMC.totalBytes() - SD_MMC.usedBytes());
#else
    // Init filesystem (format if necessary)
    if(!FFat.begin(FORMAT_FS_IF_FAILED)){
        Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
        FFat.format();
    }

    FFat.format(true, (char*)"ffat");

    Serial.printf("\nTotal space: %10d\n", FFat.totalBytes());
    Serial.printf("Free space: %10d\n", FFat.freeBytes());
#endif

    listDir("/", 0);

    // Set the Telegram bot properies
    myBot.setUpdateTime(1000);
    myBot.setTelegramToken(token);

    // Check if all things are ok
    Serial.print("\nTest Telegram connection... ");
    myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

    // Start telegram message checking in a separate task on core 0 (the loop() function run on core 1)
    xTaskCreate(
        checkTelegram,    // Function to implement the task
        "checkTelegram",  // Name of the task
        8192,             // Stack size in words
        NULL,             // Task input parameter
        1,                // Priority of the task
        NULL             // Task handle.
    );

}

void loop() {
    printHeapStats();
}
