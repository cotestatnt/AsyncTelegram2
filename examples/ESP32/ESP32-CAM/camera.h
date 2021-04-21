
/*
 *   Pin definitions for some common ESP-CAM modules
 *
 *   Select the module to use in myconfig.h
 *   Defaults to AI-THINKER CAM module
 *
 */

#include <esp_timer.h>
#include <esp_camera.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#include <Arduino.h>

#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_WROVER_KIT)
  //
  // ESP WROVER
  // https://dl.espressif.com/dl/schematics/ESP-WROVER-KIT_SCH-2.pdf
  //
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22
  #define LED_PIN           2 // A status led on the RGB; could also use pin 0 or 4
  #define LED_ON         HIGH //
  #define LED_OFF         LOW //
  // #define LAMP_PIN          x // No LED FloodLamp.

#elif defined(CAMERA_MODEL_ESP_EYE)
  //
  // ESP-EYE
  // https://twitter.com/esp32net/status/1085488403460882437
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM     4
  #define SIOD_GPIO_NUM    18
  #define SIOC_GPIO_NUM    23
  #define Y9_GPIO_NUM      36
  #define Y8_GPIO_NUM      37
  #define Y7_GPIO_NUM      38
  #define Y6_GPIO_NUM      39
  #define Y5_GPIO_NUM      35
  #define Y4_GPIO_NUM      14
  #define Y3_GPIO_NUM      13
  #define Y2_GPIO_NUM      34
  #define VSYNC_GPIO_NUM    5
  #define HREF_GPIO_NUM    27
  #define PCLK_GPIO_NUM    25
  #define LED_PIN          21 // Status led
  #define LED_ON         HIGH //
  #define LED_OFF         LOW //
  // #define LAMP_PIN          x // No LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  //
  // ESP32 M5STACK
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_V2_PSRAM)
  //
  // ESP32 M5STACK V2
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_WIDE)
  //
  // ESP32 M5STACK WIDE
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     22
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  //
  // Common M5 Stack without PSRAM
  //
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // Note NO PSRAM,; so maximum working resolution is XGA 1024×768
  // M5 Stack status/illumination LED details unknown/unclear
  // #define LED_PIN            x // Status led
  // #define LED_ON          HIGH //
  // #define LED_OFF          LOW //
  // #define LAMP_PIN          x  // LED FloodLamp.

#elif defined(CAMERA_MODEL_AI_THINKER)
  //
  // AI Thinker
  // https://github.com/SeeedDocument/forum_doc/raw/master/reg/ESP32_CAM_V1.6.pdf
  //
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
  #define LED_PIN           33 // Status led
  #define LED_ON           LOW // - Pin is inverted.
  #define LED_OFF         HIGH //
  #define LAMP_PIN           4 // LED FloodLamp.

#elif defined(CAMERA_MODEL_TTGO_T_JOURNAL)
  //
  // LilyGO TTGO T-Journal ESP32; with OLED! but not used here.. :-(
  #define PWDN_GPIO_NUM      0
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21
  // TTGO T Journal status/illumination LED details unknown/unclear
  // #define LED_PIN           33 // Status led
  // #define LED_ON           LOW // - Pin is inverted.
  // #define LED_OFF         HIGH //
  // #define LAMP_PIN           4 // LED FloodLamp.

#else
  // Well.
  // that went badly...
  #error "Camera model not selected, did you forget to uncomment it in myconfig?"
#endif


bool autoLamp = true;          // Automatic lamp (auto on while camera running)
int lampChannel = 7;           // a free PWM channel (some channels used by camera)
const int pwmfreq = 50000;     // 50K pwm frequency
const int pwmresolution = 9;   // duty cycle bit range
const int pwmMax = pow(2,pwmresolution)-1;

// Illumination LAMP/LED
#if defined(LAMP_DISABLE)
    int lampVal = -1; // lamp is disabled in config
#elif defined(LAMP_PIN)
    int lampVal = 0; //default to off
#else
    int lampVal = -1; // no lamp pin assigned
#endif

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

void flashLED(int flashtime) {
#ifdef LED_PIN                    // If we have it; flash it.
    digitalWrite(LED_PIN, LED_ON);  // On at full power.
    delay(flashtime);               // delay
    digitalWrite(LED_PIN, LED_OFF); // turn Off
#else
    return;                         // No notifcation LED, do nothing, no delay
#endif
}

void cameraSetup(framesize_t frameSize, int jpeg_quality){

  // Create camera config structure; and populate with hardware and other defaults
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = frameSize;
  config.jpeg_quality = jpeg_quality;
  config.fb_count = 1;

  #if defined(CAMERA_MODEL_ESP_EYE)
      pinMode(13, INPUT_PULLUP);
      pinMode(14, INPUT_PULLUP);
  #endif

  // camera init
  Serial.print("Camera init... ");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
      delay(100);  // need a delay here or the next serial o/p gets missed
      Serial.printf("\n\nCRITICAL FAILURE: Camera sensor failed to initialise.\n\n");
      Serial.printf("A full (hard, power off/on) reboot will probably be needed to recover from this.\n");
      Serial.printf("Meanwhile; this unit will reboot in 1 minute since these errors sometime clear automatically\n");
      // Reset the I2C bus.. may help when rebooting.
      periph_module_disable(PERIPH_I2C0_MODULE); // try to shut I2C down properly in case that is the problem
      periph_module_disable(PERIPH_I2C1_MODULE);
      periph_module_reset(PERIPH_I2C0_MODULE);
      periph_module_reset(PERIPH_I2C1_MODULE);
      // Start a 60 second watchdog timer
      esp_task_wdt_init(60, true);
      esp_task_wdt_add(NULL);
  } else {
      Serial.println("succeeded");

      // Get a reference to the sensor
      sensor_t * s = esp_camera_sensor_get();

      // Dump camera module, warn for unsupported modules.
      switch (s->id.PID) {
          case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
          case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
          case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
          case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
          default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
      }

      // OV3660 initial sensors are flipped vertically and colors are a bit saturated
      if (s->id.PID == OV3660_PID) {
          s->set_vflip(s, 1);  //flip it back
          s->set_brightness(s, 1);  //up the blightness just a bit
          s->set_saturation(s, -2);  //lower the saturation
      }

      // M5 Stack Wide has special needs
      #if defined(CAMERA_MODEL_M5STACK_WIDE)
          s->set_vflip(s, 1);
          s->set_hmirror(s, 1);
      #endif

      /*
      * Add any other defaults you want to apply at startup here:
      * uncomment the line and set the value as desired (see the comments)
      *
      * these are defined in the esp headers here:
      * https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h#L149
      */

      s->set_framesize(s, frameSize); // FRAMESIZE_[QQVGA|HQVGA|QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA|QXGA(ov3660)]);
      s->set_quality(s, jpeg_quality);// 10 to 63
      s->set_brightness(s, 0);      // -2 to 2
      s->set_contrast(s, 0);        // -2 to 2
      s->set_saturation(s, 0);      // -2 to 2
      s->set_special_effect(s, 2);  // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
      s->set_whitebal(s, 1);        // aka 'awb' in the UI; 0 = disable , 1 = enable
      s->set_awb_gain(s, 1);        // 0 = disable , 1 = enable
      s->set_wb_mode(s, 1);         // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
      s->set_exposure_ctrl(s, 1);   // 0 = disable , 1 = enable
      s->set_aec2(s, 0);            // 0 = disable , 1 = enable
      s->set_ae_level(s, 0);        // -2 to 2
      //s->set_aec_value(s, 300);     // 0 to 1200
      //s->set_gain_ctrl(s, 1);       // 0 = disable , 1 = enable
      //s->set_agc_gain(s, 0);        // 0 to 30
      //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
      //s->set_bpc(s, 0);             // 0 = disable , 1 = enable
      //s->set_wpc(s, 1);             // 0 = disable , 1 = enable
      s->set_raw_gma(s, 1);         // 0 = disable , 1 = enable
      //s->set_lenc(s, 1);            // 0 = disable , 1 = enable
      //s->set_hmirror(s, 0);         // 0 = disable , 1 = enable
      //s->set_vflip(s, 0);           // 0 = disable , 1 = enable
      //s->set_dcw(s, 1);             // 0 = disable , 1 = enable
      //s->set_colorbar(s, 0);        // 0 = disable , 1 = enable
  }

}
