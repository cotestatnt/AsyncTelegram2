#include <LittleFS.h>
#include <AsyncTelegram2.h>

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"
struct tm sysTime;

#include <WiFiClientSecure.h>
WiFiClientSecure client;
#ifdef ESP8266
#include <ESP8266WiFi.h>
  Session   session;
  X509List  certificate(telegram_cert);
#endif


const uint8_t LED = LED_BUILTIN;

AsyncTelegram2 myBot(client);

const char* ssid  =  "xxxxxxxxx";     // SSID WiFi network
const char* pass  =  "xxxxxxxxx";     // Password  WiFi network
const char* token =  "xxxxxx:xxxxxxxxxxxxx";  // Telegram token

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;

void messageSent(bool sent) {
  if (sent) {
    Serial.println("Last message was delivered");
  }
  else {
    Serial.println("Last message was NOT delivered");
  }
}


void sendDocument(TBMessage &msg,
                  AsyncTelegram2::DocumentType fileType,
                  const char* filename,
                  const char* caption = nullptr )
  {
  Serial.print("\nFilename: ");
  Serial.println(filename);

  File file = LittleFS.open(filename, "r");
  if (file) {
    myBot.sendDocument(msg, file, file.size(), fileType, file.name(), caption);
    myBot.sendMessage(msg, "Log file sent");
    file.close();
  }
  else {
    Serial.println("Can't open the file. Upload \"data\" folder to filesystem");
  }
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);

  // rst_info *resetInfo;
  // resetInfo = ESP.getResetInfoPtr();
  // Serial.print("Reset reason: ");
  // Serial.println(resetInfo->reason);
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  // connects to access point
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  // Init filesystem (format if necessary)
  if (!LittleFS.begin()) {
    Serial.println("\nFS Mount Failed.\nFilesystem will be formatted, please wait.");
    LittleFS.format();
    ESP.restart();
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
  client.setCACert(telegram_cert);
#endif

  // Add the callback function to bot
  myBot.addSentCallback(messageSent, 3000);

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // https://core.telegram.org/bots/api#formatting-options
  myBot.setFormattingStyle(AsyncTelegram2::FormatStyle::HTML /* MARKDOWN */);

  // Send a welcome message to user when ready
  char welcome_msg[128];
  snprintf(welcome_msg, sizeof(welcome_msg),
          "BOT @%s online.\n/help for command list.\nLast reset reason: %d",
          myBot.getBotName(), -1 /*resetInfo->reason*/);

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
    MessageType msgType = msg.messageType;

    // Received a text message
    if (msgType == MessageText){
      String msgText = msg.text;
      Serial.print("Text message received: ");
      Serial.println(msgText);

      // Send docuements stored in filesystem passing the stream
      // (File is a class derived from Stream)
      if (msgText.indexOf("/picfs") > -1) {
        Serial.println("\nSending a picture from filesystem");
        sendDocument(msg, AsyncTelegram2::DocumentType::PHOTO, "/telegram-bot.jpg", "This is the caption" );
      }

      // Send a csv file passing the stream
      else if (msgText.indexOf("/csv") > -1) {
        Serial.println("\nSending csv file from filesystem");
        sendDocument(msg, AsyncTelegram2::DocumentType::CSV, "/cities.csv" );
      }

      else if (msgText.indexOf("/zip") > -1) {
        Serial.println("\nSending a zip file from filesystem");
        sendDocument(msg, AsyncTelegram2::DocumentType::ZIP, "/cities.zip" );
      }

      else if (msgText.indexOf("/pdf") > -1) {
        Serial.println("\nSending a pdf file from filesystem");
        sendDocument(msg, AsyncTelegram2::DocumentType::PDF, "/test.pdf" );
      }

      else if (msgText.equalsIgnoreCase("/reset")) {
        myBot.sendMessage(msg, "Restarting ESP....");
        // Wait until bot synced with telegram to prevent cyclic reboot
        while (!myBot.noNewMessage()) {
          Serial.print(".");
          delay(50);
        }
        ESP.restart();
      }

      else {
        String replyMsg = "Welcome to the AsyncTelegram2 bot.\n\n";
        replyMsg += "/csv will send an example csv file from fylesystem\n";
        replyMsg += "/zip will send an example zipped file from fylesystem\n";
        replyMsg += "/pdf will send a test pdf file from fylesystem\n";
        replyMsg += "/picfs will send an example picture from fylesystem\n";
        myBot.sendMessage(msg, replyMsg);
      }

    }
  }
}


// List all files saved in the selected filesystem
void listDir(const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  File root = LittleFS.open(dirname, "r");
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
  static uint32_t infoTime;
  if (millis() - infoTime > 10000) {
    infoTime = millis();
    time_t now = time(nullptr);
    sysTime = *localtime(&now);
#ifdef ESP32
    //heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    Serial.printf("%02d:%02d:%02d - Total free: %6d - Max block: %6d\n",
      sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec,
      heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0) );
#elif defined(ESP8266)
    uint32_t free;
    uint32_t max;
    uint8_t frag;
    ESP.getHeapStats(&free, &max, &frag);
    Serial.printf("%02d:%02d:%02d - Total free: %5lu - Max block: %5lu - Frag: %5d\n",
      sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec, free, max, frag);
#endif
  }
}