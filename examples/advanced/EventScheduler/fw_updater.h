#include "AsyncTelegram2.h"

extern const char* firmware_version;
extern const char* update_password;

#ifdef ESP8266
    #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
    #include <HTTPClient.h>
    #include <HTTPUpdate.h>
#endif


bool updateHandler(TBMessage* msg, AsyncTelegram2* myBot) {
    bool parsed = false;
    static String fw_path;
    switch (msg->messageType) {
        // Type of message "document"
        case MessageDocument :
            // Store in memory link to the firmware file
            fw_path = msg->document.file_path;
            if (msg->document.file_exists) {
                // Check file extension of received document (firmware must be .bin)
                if( msg->document.file_path.endsWith(".bin")) {
                    char report [128];
                    snprintf(report, 128, "Start firmware update\nFile name: %s\nFile size: %d",
                        msg->document.file_name, msg->document.file_size);
                    // Inform user and query for flash confirmation with password
                    myBot->sendMessage(*msg, report, "");
                    msg->force_reply = true;
                    myBot->sendMessage(*msg, "Please insert password", "");   // Force reply == true
                }
            }
            else {
                myBot->sendMessage(*msg, "File is unavailable. Maybe size limit 20MB was reached or file deleted");
            }
            parsed = true;
            break;

        // This is a reply message, maybe the user reply with right password?
        case MessageReply:
            // User has confirmed flash start with right password
            if (msg->text.equalsIgnoreCase(update_password)) {
                myBot->sendMessage(*msg, "Start flashing... please wait (~30/60s)");
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
                // Seems work properly only with WiFiClientSecure,
                // so in case we are using SSLClient stop it and create new local client
                client.stop();
                WiFiClientSecure updateClient;
                updateClient.setTimeout(12); // Increase timeout
                updateClient.setInsecure();
                t_httpUpdate_return ret = httpUpdate.update(updateClient, fw_path, firmware_version);
                updateClient.stop();
                #endif

                fw_path = "";
                switch (ret) {
                    default:
                    #ifdef ESP8266
                        char report [64];
                        snprintf(report, 64, "HTTP_UPDATE_FAILED Error (%d): %s",
                                ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                        myBot->sendMessage(*msg, report);
                    #elif defined(ESP32)
                        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                    #endif
                        break;

                    case HTTP_UPDATE_NO_UPDATES:
                        myBot->sendMessage(*msg, "HTTP_UPDATE_NO_UPDATES");
                        break;

                    case HTTP_UPDATE_OK:
                        myBot->sendMessage(*msg, "UPDATE OK.\nRestarting...");
                        // Wait until bot synced with telegram to prevent cyclic reboot
                        while (!myBot->noNewMessage()) {
                            Serial.print(".");
                            delay(50);
                        }
                        delay(1000);
                        ESP.restart();
                        break;
                }
            }
            // Wrong password
            else{
                myBot->sendMessage(*msg, "You have entered wrong password");
                fw_path = "";
            }
            parsed = true;
            break;

        default:
            break;
    }

    return parsed;
}