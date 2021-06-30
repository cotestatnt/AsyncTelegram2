#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <AsyncTelegram2.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix

#define USE_SSLCLIENT 0
#if USE_SSLCLIENT
  #include <SSLClient.h>
  #include "tg_certificate.h"
  WiFiClient base_client;
  SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0);
#else
  #include <WiFiClientSecure.h>
  WiFiClientSecure client;
#endif

const char* ssid = "PuccosNET";  // SSID WiFi network
const char* pass = "Tole76tnt";  // Password  WiFi network
const char* token = "488075445:AAGhC7lD2_lLWhRlkoQcnrranavca8rPrfM";

// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

AsyncTelegram2 myBot(client);

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define LAMP_PIN 4

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,  
  .xclk_freq_hz = 10000000,        //XCLK 20MHz or 10MHz
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,  //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_UXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
  .jpeg_quality = 12,              //0-63 lower number means higher quality
  .fb_count = 1                    //if more than one, i2s runs in continuous mode. Use only with JPEG
};


int lampChannel = 7;           // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
const int pwmMax = pow(2,pwmresolution)-1;

// Lamp Control
void setLamp(int newVal) {
    if (newVal != -1) {
        // Apply a logarithmic function to the scale.
        int brightness = round((pow(2,(1+(newVal*0.02)))-2)/6*pwmMax);
        ledcWrite(lampChannel, brightness);
        Serial.print("Lamp: ");
        Serial.print(newVal);
        Serial.print("%, pwm = ");
        Serial.println(brightness);
    }
}

static esp_err_t init_camera() {
  //initialize the camera
  Serial.print("Camera init... ");
  esp_err_t err = esp_camera_init(&camera_config);

  if (err != ESP_OK) {
    delay(100);  // need a delay here or the next serial o/p gets missed
    Serial.printf("\n\nCRITICAL FAILURE: Camera sensor failed to initialise.\n\n");
    Serial.printf("A full (hard, power off/on) reboot will probably be needed to recover from this.\n");
    return err;
  } else {
    Serial.println("succeeded");

    // Get a reference to the sensor
    sensor_t* s = esp_camera_sensor_get();

    // Dump camera module, warn for unsupported modules.
    switch (s->id.PID) {
      case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
      case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
      default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
    }
  }
  return ESP_OK;
}


// Save a picture taken from CAM to filesystem
bool savePicture(char* filename) {

  // Take Picture with Camera;
  setLamp(100);
  camera_fb_t *fb = esp_camera_fb_get();
  setLamp(0);
  if (!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return false;
  }
  // Save frame to file
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return false;
  }
  file.write(fb->buf, fb->len);  
  // Check if file was written with success
  if (file.size() == 0) {
    file.write(fb->buf, fb->len);   // Previuos write fails, try again
    Serial.print("*");
  }
  Serial.printf("Saved file to path: %s - %zu bytes\n", filename, file.size());
  file.close();
  
  esp_camera_fb_return(fb);
  return true;
}

bool sendPicture(TBMessage& msg, const char* filename) {
  
  File file = SPIFFS.open(filename, "r");
  if (file.size() > 0) {
    Serial.printf("Sending file %s - %zu bytes\n", filename, file.size());
    myBot.sendPhotoByFile(msg.sender.id, &file, file.size());
    file.close();
    SPIFFS.remove(filename);
    return true;
  } 
  else {
    Serial.println("File has not valid size");
    file.close();
    SPIFFS.remove("/frame.jpg");
    return false;
  }
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

  // Init filesystem (format if necessary)
  if (!SPIFFS.begin(true)) {
    Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
    SPIFFS.format();
    delay(1000);
    ESP.restart();
  }

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
  #if USE_CLIENTSSL == false
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
  int64_t userid = 1234567890;  
  myBot.sendTo(userid, welcome_msg);

  // Init the camera module (accordind the camera_config_t defined)
  init_camera();
}

void loop() {
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
        char* filename = "/frame.jpg";
        if (savePicture(filename)) {
          if (sendPicture(msg, filename))
            Serial.println("Picture sent successfull");
        }
      } 
      else {
        Serial.print("\nText message received: ");
        Serial.println(msg.text);
        String replyStr = "Message received:\n";
        replyStr += msg.text;
        replyStr += "\nTry with /takePhoto";
        myBot.sendMessage(msg, replyStr);
      }
    }
  }
}
