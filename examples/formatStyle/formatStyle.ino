
/*
  https://github.com/cotestatnt/AsyncTelegram2
  Name:         formatStyle.ino
  Created:      07/09/2022
  Author:       Tolentino Cotesta <cotestatnt@yahoo.com>
  Description:  an example to show how send farmatted messages
*/
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <AsyncTelegram2.h>

// Timezone definition
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

/*
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
*/
#define USE_CLIENTSSL false

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  WiFiClientSecure client;
  Session   session;
  X509List  certificate(telegram_cert);
#elif defined(ESP32)
  #if USE_CLIENTSSL
    #include <SSLClient.h>      //https://github.com/OPEnSLab-OSU/SSLClient/
    #include "tg_certificate.h"
    WiFiClient base_client;
    SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
  #else
    WiFiClientSecure client;
  #endif
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
const uint8_t LED = LED_BUILTIN;

AsyncTelegram2 myBot(client);
const char* ssid  =  "xxxxxxxx";       // SSID WiFi network
const char* pass  =  "xxxxxxxx";       // Password  WiFi network
const char* token =  "xxxxxx:xxxxxx";  // Telegram token


// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 123456789;

// Name of public channel (your bot must be in admin group)
const char* channel = "@AsyncTelegram2";

// Structure containing a calendar date and time broken down into its components.
struct tm sysTime;

static const char html_formatted[] PROGMEM = R"EOF(
<b>bold</b>, <strong>bold</strong>
<i>italic</i>, <em>italic</em>
<u>underline</u>, <ins>underline</ins>
<s>strikethrough</s>, <strike>strikethrough</strike>, <del>strikethrough</del>
<span class="tg-spoiler">spoiler</span>, <tg-spoiler>spoiler</tg-spoiler>
<b>bold <i>italic bold <s>italic bold strikethrough
<span class="tg-spoiler">italic bold strikethrough spoiler</span></s> <u>underline italic bold</u></i> bold</b>
<a href="http://www.example.com/">inline URL</a>
<a href="tg://user?id=123456789">inline mention of a user</a>
<code>inline fixed-width code</code>
<pre>pre-formatted fixed-width code block</pre>
<pre><code class="language-python">pre-formatted fixed-width code block written in the Python programming language</code></pre>
)EOF";

static const char markdown_formatted[] PROGMEM = R"EOF(
*bold \*text*
_italic \*text_
__underline__
~strikethrough~
||spoiler||
*bold _italic bold ~italic bold strikethrough
||italic bold strikethrough spoiler||~ __underline italic bold___ bold*
[inline URL](http://www.example.com/)
[inline mention of a user](tg://user?id=123456789)
`inline fixed-width code`
```
pre-formatted fixed-width code block
```
```python
pre-formatted fixed-width code block written in the Python programming language
```
)EOF";


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the Serial
  Serial.begin(115200);

  rst_info *resetInfo;
  resetInfo = ESP.getResetInfoPtr();
  Serial.print("Reset reason: ");
  Serial.println(resetInfo->reason);

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

  // https://core.telegram.org/bots/api#formatting-options
  myBot.setFormattingStyle(AsyncTelegram2::FormatStyle::HTML /* MARKDOWN */);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  char welcome_msg[128];
  snprintf(welcome_msg, sizeof(welcome_msg),
          "BOT @%s online.\n/help for command list.\nLast reset reason: %d",
          myBot.getBotName(), resetInfo->reason);

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

      // Send a message using HTML style
      if (msgText.equalsIgnoreCase("/html")) {
        myBot.setFormattingStyle(AsyncTelegram2::FormatStyle::HTML);
        String message = html_formatted;
        myBot.sendMessage(msg, message);
        myBot.sendToChannel(channel, message.c_str());
      }

      // Send a message using MarkdownV2 style
      else if (msgText.equalsIgnoreCase("/markdown")) {
        myBot.setFormattingStyle(AsyncTelegram2::FormatStyle::MARKDOWN);
        String message = markdown_formatted;
        myBot.sendMessage(msg, message);
        myBot.sendToChannel(channel, message.c_str(), true);
      }

      else {
        String replyMsg = "Welcome to the Async Telegram bot.\n\n";
        replyMsg += "Try command /html for a message formatted with HTML style.\n";
        replyMsg += "Try command /markdown for a message formatted with MarkdownV2 style.\n";
        myBot.sendMessage(msg, replyMsg);
      }

    }
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
    uint16_t max;
    ESP.getHeapStats(&free, &max, nullptr);
    Serial.printf("%02d:%02d:%02d - Total free: %5d - Max block: %5d\n",
      sysTime.tm_hour, sysTime.tm_min, sysTime.tm_sec, free, max);
#endif
  }
}