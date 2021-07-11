#include "AsyncTelegram2.h"

extern const char* firmware_version;
extern const char* update_password;
extern void doRestart();

#ifdef ESP8266
    #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
    #include <HTTPUpdate.h>
#endif

bool updateHandler(TBMessage &theMsg, AsyncTelegram2 &theBot) {
    bool parsed = false;
    static String fw_path;
    switch (theMsg.messageType) {
        // Type of message "document"
        case MessageDocument: {
            // Store in memory link to the firmware file
            fw_path = theMsg.document.file_path;
            if (theMsg.document.file_exists) {
                // Check file extension of received document (firmware must be .bin)
                if( theMsg.document.file_path.endsWith(".bin")) {
                    char report [128];
                    snprintf(report, 128, "Start firmware update\nFile name: %s\nFile size: %d",
                        theMsg.document.file_name, theMsg.document.file_size);
                    // Inform user and query for flash confirmation with password
                    theBot.sendMessage(theMsg, report, "");

                   // Force reply don't work with web version of Telegram Client (use Telegram Desktop or mobile app)
                    theMsg.force_reply = true;
                    theBot.sendMessage(theMsg, "Please, reply to this message with the right password", "");  
                }
            }
            else {
                theBot.sendMessage(theMsg, "File is unavailable. Maybe size limit 20MB was reached or file deleted");
            }
            parsed = true;
            break;
        }

        // This is a reply message, maybe the user reply with right password?
        case MessageReply: {
            // User has confirmed flash start with right password
            if (theMsg.text.equalsIgnoreCase(update_password)) {
                theBot.sendMessage(theMsg, "Start flashing... please wait (~30/60s)");
                Serial.print("Firmware path: ");
                Serial.println(fw_path);
                if(fw_path.length() == 0)
                    break;
                
                #ifdef ESP8266              
                // onProgress handling is missing with ESP32 library
                ESPhttpUpdate.onProgress([](int cur, int total){
                    static uint32_t sendT;
                    if(millis() - sendT > 1000){
                        sendT = millis();
                        Serial.printf("Updating %d of %d bytes...\n", cur, total);
                    }
                });
                t_httpUpdate_return ret = ESPhttpUpdate.update(client, fw_path, firmware_version);
                client.stop();
                #elif defined(ESP32)
                WiFiClientSecure updateClient;
                updateClient.setInsecure();
                updateClient.setTimeout(12); // Increase timeout
                t_httpUpdate_return ret = httpUpdate.update(updateClient, fw_path, firmware_version);
                updateClient.stop();
                #endif
                
               
                fw_path = "";
                char report [128];
                switch (ret) {
                    default:
                    #ifdef ESP8266                        
                        snprintf(report, 128, "HTTP_UPDATE_FAILED Error (%d): %s",
                                ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                        theBot.sendMessage(theMsg, report);
                    #elif defined(ESP32)
                        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                    #endif
                        break;

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
            // Wrong password
            else{
                theBot.sendMessage(theMsg, "You have entered wrong password");
                fw_path = "";
            }
            parsed = true;
            break;
        }

        default:
            break;
    }

    return parsed;
}
