#ifndef ASYNCTELEGRAMV2
#define ASYNCTELEGRAMV2

// for using int_64 data
#define ARDUINOJSON_USE_LONG_LONG 	1
#define ARDUINOJSON_DECODE_UNICODE  1
#include <ArduinoJson.h>
#include "Client.h"
#include "time.h"

#define DEBUG_ENABLE        0
#ifndef DEBUG_ENABLE
    #define DEBUG_ENABLE    0
#endif

/*
    This affect only inline keyboard with at least one callback function defined.
    If you need more than MAX_INLINEKYB_CB distinct keybords with
    callback functions associated to buttons increase this value
*/
#define MAX_INLINEKYB_CB    10

#define SERVER_TIMEOUT      10000
#define MIN_UPDATE_TIME     500

#if defined(ESP8266)
#define BLOCK_SIZE          2048
#else
#define BLOCK_SIZE          4096
#endif

#include "DataStructures.h"
#include "InlineKeyboard.h"
#include "ReplyKeyboard.h"
#include "serial_log.h"

#define TELEGRAM_HOST  "api.telegram.org"
#define TELEGRAM_IP    "149.154.167.220"
#define TELEGRAM_PORT   443

/* This is used with ESP8266 platform only */
static const char telegram_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIE0DCCA7igAwIBAgIBBzANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx
EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT
EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp
ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTExMDUwMzA3MDAwMFoXDTMxMDUwMzA3
MDAwMFowgbQxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH
EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjEtMCsGA1UE
CxMkaHR0cDovL2NlcnRzLmdvZGFkZHkuY29tL3JlcG9zaXRvcnkvMTMwMQYDVQQD
EypHbyBEYWRkeSBTZWN1cmUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwggEi
MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC54MsQ1K92vdSTYuswZLiBCGzD
BNliF44v/z5lz4/OYuY8UhzaFkVLVat4a2ODYpDOD2lsmcgaFItMzEUz6ojcnqOv
K/6AYZ15V8TPLvQ/MDxdR/yaFrzDN5ZBUY4RS1T4KL7QjL7wMDge87Am+GZHY23e
cSZHjzhHU9FGHbTj3ADqRay9vHHZqm8A29vNMDp5T19MR/gd71vCxJ1gO7GyQ5HY
pDNO6rPWJ0+tJYqlxvTV0KaudAVkV4i1RFXULSo6Pvi4vekyCgKUZMQWOlDxSq7n
eTOvDCAHf+jfBDnCaQJsY1L6d8EbyHSHyLmTGFBUNUtpTrw700kuH9zB0lL7AgMB
AAGjggEaMIIBFjAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjAdBgNV
HQ4EFgQUQMK9J47MNIMwojPX+2yz8LQsgM4wHwYDVR0jBBgwFoAUOpqFBxBnKLbv
9r0FQW4gwZTaD94wNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8v
b2NzcC5nb2RhZGR5LmNvbS8wNQYDVR0fBC4wLDAqoCigJoYkaHR0cDovL2NybC5n
b2RhZGR5LmNvbS9nZHJvb3QtZzIuY3JsMEYGA1UdIAQ/MD0wOwYEVR0gADAzMDEG
CCsGAQUFBwIBFiVodHRwczovL2NlcnRzLmdvZGFkZHkuY29tL3JlcG9zaXRvcnkv
MA0GCSqGSIb3DQEBCwUAA4IBAQAIfmyTEMg4uJapkEv/oV9PBO9sPpyIBslQj6Zz
91cxG7685C/b+LrTW+C05+Z5Yg4MotdqY3MxtfWoSKQ7CC2iXZDXtHwlTxFWMMS2
RJ17LJ3lXubvDGGqv+QqG+6EnriDfcFDzkSnE3ANkR/0yBOtg2DZ2HKocyQetawi
DsoXiWJYRBuriSUBAA/NxBti21G00w9RKpv0vHP8ds42pM3Z2Czqrpv1KrKQ0U11
GIo/ikGQI31bS/6kA1ibRrLDYGCD+H1QQc7CoZDDu+8CL9IVVO5EFdkKrqeKM+2x
LXY2JtwE65/3YR8V3Idv7kaWKK2hJn0KCacuBKONvPi8BDAB
-----END CERTIFICATE-----
)EOF";



class AsyncTelegram2
{

public:
    // default constructor
    AsyncTelegram2(Client &client);
    // default destructor
    ~AsyncTelegram2();

    // test the connection between ESP8266 and the telegram server
    // returns
    //    true if no error occurred
    bool begin(void);

    // reset the connection between ESP8266 and the telegram server (ex. when connection was lost)
    // returns
    //    true if no error occurred
    bool reset(void);

    // set the telegram token
    // params
    //   token: the telegram token
    inline void setTelegramToken(const char* token) { m_token = (char*) token; }

    // set the interval in milliseconds for polling
    // in order to Avoid query Telegram server to much often (ms)
    // params:
    //    pollingTime: interval time in milliseconds
    void setUpdateTime(uint32_t pollingTime) { m_minUpdateTime = pollingTime;}

    // Get file link and size by unique document ID
    // params
    //   doc   : document structure
    // returns
    //   true if no error
    bool getFile(TBDocument &doc);

    // get the first unread message from the queue (text and query from inline keyboard).
    // This is a destructive operation: once read, the message will be marked as read
    // so a new getMessage will read the next message (if any).
    // params
    //   message: the data structure that will contains the data retrieved
    // returns
    //   MessageNoData: an error has occurred
    //   MessageText  : the received message is a text
    //   MessageQuery : the received message is a query (from inline keyboards)
    MessageType getNewMessage(TBMessage &message);

    // send a message to the specified telegram user ID
    // params
    //   msg      : the TBMessage telegram recipient with user ID
    //   message : the message to send
    //   keyboard: the inline/reply keyboard (optional)
    //             (in json format or using the inlineKeyboard/ReplyKeyboard class helper)
    void sendMessage(const TBMessage &msg, const char* message, String keyboard = "");

    // sendMessage function overloads
    inline void sendMessage(const TBMessage &msg, const String &message, String keyboard = "")
    {
        return sendMessage(msg, message.c_str(), keyboard);
    }

    inline void sendMessage(const TBMessage &msg, const char* message, InlineKeyboard &keyboard)
    {
        return sendMessage(msg, message, keyboard.getJSON());
    }

    inline void sendMessage(const TBMessage &msg, const char* message, ReplyKeyboard &keyboard) {
        return sendMessage(msg, message, keyboard.getJSON());
    }


    // Send message to a specific user. In order to work properly two conditions is needed:
    //  - You have to find the userid (for example using the bot @JsonBumpBot  https://t.me/JsonDumpBot)
    //  - User has to start your bot in it's own client. For example send a message with @<your bot name>
    void sendTo(const int32_t userid, const char* message, String keyboard = "") {
        TBMessage msg;
        msg.chatId = userid;
        return sendMessage(msg, message, keyboard);
    }

    // sendTo function overloads
    inline void sendTo(const int32_t userid, const String &message, String keyboard = "") {
        sendTo(userid, message, keyboard );
    }

    // Send message to a channel. This bot must be in the admin group
    void sendToChannel(const char*  &channel, const String &message, bool silent) ;

    // Send a picture passing the url
    void sendPhotoByUrl(const uint32_t& chat_id,  const String& url, const String& caption);

    inline void sendPhotoByUrl(const TBMessage &msg,  const String& url, const String& caption){
        sendPhotoByUrl(msg.sender.id, url, caption);
    }

    // Send a picture stored in local memory
    inline bool sendPhotoByFile(uint32_t chat_id, Stream* stream, size_t size) {
        return sendDocument(chat_id, "sendPhoto", "image/jpeg", "photo", stream, size);
    }

    // terminate a query started by pressing an inlineKeyboard button. The steps are:
    // 1) send a message with an inline keyboard
    // 2) wait for a <message> (getNewMessage) of type MessageQuery
    // 3) handle the query and then call endQuery with <message>.callbackQueryID
    // params
    //   msg  : the TBMessage telegram recipient with unique query ID (retrieved with getNewMessage method)
    //   message  : an optional message
    //   alertMode: false -> a simply popup message
    //              true --> an alert message with ok button
    void endQuery(const TBMessage &msg, const char* message, bool alertMode = false);

    // remove an active reply keyboard for a selected user, sending a message
    // params:
    //   msg      : the TBMessage telegram recipient with the telegram user ID
    //   message  : the message to be show to the selected user ID
    //   selective: enable selective mode (hide the keyboard for specific users only)
    //              Targets: 1) users that are @mentioned in the text of the Message object;
    //                       2) if the bot's message is a reply (has reply_to_message_id), sender of the original message
    // return:
    //   true if no error occurred
    void removeReplyKeyboard(const TBMessage &msg, const char* message, bool selective = false);

    // Get the current bot name
    // return:
    //   the bot name
    inline const char* getBotName() {
        return m_botusername.c_str();
    }

    // Check for no new pending message
    // (to be sure all messages was parsed, before doing something)
    // Example: OTA sketch
    // return:
    //   true if no message
    bool noNewMessage();

    // keep track of defined inline keybaord in order to call cb function
    // params: pointer to inline keyboard
    inline void addInlineKeyboard(InlineKeyboard* keyb)
    {
        m_keyboards[m_keyboardCount++] = keyb;
    }

private:
    Client*         telegramClient;
    const char*     m_token;
    String          m_botusername;      // Store only botname, instead TBUser struct

    int32_t         m_lastUpdateId = 0;
    uint32_t        m_lastUpdateTime;
    uint32_t        m_minUpdateTime = 2000;

    uint32_t        m_lastmsg_timestamp;
    bool            m_waitingReply;

    InlineKeyboard* m_keyboards[10];
    uint8_t         m_keyboardCount = 0;

    // send commands to the telegram server. For info about commands, check the telegram api https://core.telegram.org/bots/api
    // params
    //   command   : the command to send, i.e. getMe
    //   parameters: optional parameters
    // returns
    //   an empty string if error
    //   a string containing the Telegram JSON response
    bool sendCommand(const char* const &command, JsonDocument &doc, bool blocking = false);

        // query server for new incoming messages
    // returns
    //   http response payload if no error occurred
    bool getUpdates(JsonDocument &doc);

    // get some information about the bot
    // params
    //   user: the data structure that will contains the data retreived
    // returns
    //   true if no error occurred
    bool getMe();

    //  example: sendDocument("sendPhoto", chat_id, "image/jpeg", "photo", File );
    bool sendDocument( uint32_t chat_id, const char* command, const char* contentType, const char* binaryPropertyName, Stream* stream, size_t size);

    // check if connection with server is active
    // returns
    //   true on connected
    bool checkConnection();

};

#endif
