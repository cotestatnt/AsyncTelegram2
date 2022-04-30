#include "AsyncTelegram2.h"

extern const char* firmware_version;
extern const char* update_password;
extern void doRestart();

#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#define UPDATER ESPhttpUpdate
#elif defined(ESP32)
#include <HTTPUpdate.h>
#define UPDATER httpUpdate
#endif


bool updateHandler(TBMessage &theMsg, AsyncTelegram2 &theBot) {

  bool msgParsed = false;             // A variable to inform caller that msg is for this function
  static String fw_path;              // The remote firmware path (passed from Telegram server)
  static bool updateRequest = false;  // A variable to handle only MessageReply related to firmware update

  // Check message type
  switch (theMsg.messageType) {

    // Here will be evalueted only MessageDocument and MessageReply type
    default:
      break;

    // The massage received if "document" type (binary file)
    case MessageDocument: {

        // Store in memory link to the firmware file
        fw_path = theMsg.document.file_path;
        if (theMsg.document.file_exists) {

          // Check file extension of received document (firmware must be .bin)
          if ( theMsg.document.file_path.endsWith(".bin")) {
            char report [128];
            snprintf(report, 128, "Start firmware update\nFile name: %s\nFile size: %d",
                     theMsg.document.file_name.c_str(), theMsg.document.file_size);

            // Inform user and query for flash confirmation with password
            theBot.sendMessage(theMsg, report, "");

            // Force reply don't work with web version of Telegram Client (use Telegram Desktop or mobile app)
            theMsg.force_reply = true;
            theBot.sendMessage(theMsg, "Please, reply to this message with the right password", "");
            updateRequest = true;
          }
        }
        else {
          theBot.sendMessage(theMsg, "File is unavailable. Maybe size limit 20MB was reached or file deleted");
        }
        msgParsed = true;
        break;
      }

    // This is a reply message, maybe the user reply with password?
    case MessageReply: {

        // User has confirmed flash start
        if (updateRequest) {

          // Wrong password
          if (!theMsg.text.equalsIgnoreCase(update_password)) {
            theBot.sendMessage(theMsg, "You have entered wrong password");
            fw_path = "";
            msgParsed = true;
            break;
          }

          // Check if previous message has a valid firmware remote path
          if (fw_path.length() != 0) {
            updateRequest = false;
            theBot.sendMessage(theMsg, "Start flashing... please wait (~30/60s)");
            Serial.printf("Firmware path: %s\n", fw_path.c_str());
          }
          else
            break;

          // onProgress handling is missing with ESP32 library
#ifdef ESP8266
          UPDATER.onProgress([](int cur, int total) {
            static uint32_t sendT;
            if (millis() - sendT > 1000) {
              sendT = millis();
              Serial.printf("Updating %d of %d bytes...\n", cur, total);
            }
          });
#endif

          // Start remote firmware update
          WiFiClientSecure updateClient;
          updateClient.setInsecure();         

          // We have to handle reboot manually after sync with TG server
          UPDATER.rebootOnUpdate(false);
          t_httpUpdate_return ret = UPDATER.update(updateClient, fw_path, firmware_version);
          updateClient.stop();
          fw_path = "";

          // Evaluate the result
          switch (ret) {
            default: {
                char report [128];
                snprintf(report, 128,
                         "HTTP_UPDATE_FAILED Error (%d): %s",
                         UPDATER.getLastError(),
                         UPDATER.getLastErrorString().c_str());
                theBot.sendMessage(theMsg, report);
                break;
              }

            case HTTP_UPDATE_NO_UPDATES:
              theBot.sendMessage(theMsg, "HTTP_UPDATE_NO_UPDATES");
              break;

            case HTTP_UPDATE_OK:
              theBot.begin();
              String infoStr = "UPDATE OK.\nRestarting in few seconds...";
              theBot.sendMessage(theMsg, infoStr);
              delay(100);
              // Wait until bot synced with telegram to prevent cyclic reboot
              doRestart();
              break;
          }
        }
      }
  }

  return msgParsed;
}
