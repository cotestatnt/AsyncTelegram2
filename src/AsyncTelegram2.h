#ifndef ASYNCTELEGRAMV2
#define ASYNCTELEGRAMV2

// for using int_64 data
#define ARDUINOJSON_USE_LONG_LONG 1
#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>

#if defined(ESP32) || defined(ESP8266)
#define FS_SUPPORT true
#include <FS.h>
#include <WiFiClientSecure.h>
#else
#define FS_SUPPORT false
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#include "Client.h"
#include "time.h"

#ifndef DEBUG_ENABLE
#define DEBUG_ENABLE 0
#endif

#if defined(ESP32) || defined(ESP8266)
#define FS_SUPPORT true
#include <FS.h>
#include <WiFiClientSecure.h>
#else
#define FS_SUPPORT false
#endif


// int 32 bit long, (eg. ESP32 platform)
#if INT_MAX == 2147483647
    #define INT32 "d"
#else
   #define INT32 "ld"
#endif

/*
    This affect only inline keyboard with at least one callback function defined.
    If you need more than MAX_INLINEKYB_CB distinct keybords with
    callback functions associated to buttons increase this value
*/
#define MAX_INLINEKYB_CB 30

#define SERVER_TIMEOUT 10000
#define MIN_UPDATE_TIME 500

#define BLOCK_SIZE 1436 // 2872   // 2 * TCP_MSS

#include "DataStructures.h"
#include "InlineKeyboard.h"
#include "ReplyKeyboard.h"
#include "serial_log.h"

#define TELEGRAM_HOST "api.telegram.org"
#define TELEGRAM_IP "149.154.167.220"
#define TELEGRAM_PORT 443

static const char telegram_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx
EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT
EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp
ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz
NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH
EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE
AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw
DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD
E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH
/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy
DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh
GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR
tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA
AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE
FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX
WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu
9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr
gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo
2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO
LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI
4uJEvlz36hz1
-----END CERTIFICATE-----
)EOF";


class AsyncTelegram2
{

    // using SentCallback = std::function<void(bool sent)>;
    typedef void(*SentCallback)(bool sent);

public:
    // default constructor
    AsyncTelegram2(Client &client, uint32_t bufferSize = BUFFER_BIG);
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
    inline void setTelegramToken(const char *token) { m_token = (char *)token; }

    // set the interval in milliseconds for polling
    // in order to Avoid query Telegram server to much often (ms)
    // params:
    //    pollingTime: interval time in milliseconds
    void setUpdateTime(uint32_t pollingTime) { m_minUpdateTime = pollingTime; }

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
    //   wait:    true if method must be blocking
    bool sendMessage(const TBMessage &msg, const char *message, char *keyboard = nullptr, bool wait = false);

    // sendMessage function overloads
    inline bool sendMessage(const TBMessage &msg, const String &message, String keyboard = "")
    {
        return sendMessage(msg, message.c_str(), (char*)keyboard.c_str());
    }

    inline bool sendMessage(const TBMessage &msg, const char *message, InlineKeyboard &keyboard)
    {
        return sendMessage(msg, message, (char*) keyboard.getJSON().c_str());
    }

    inline bool sendMessage(const TBMessage &msg, const char *message, ReplyKeyboard &keyboard)
    {
        return sendMessage(msg, message, (char*) keyboard.getJSON().c_str());
    }

    // Forward a specific message to user or chat
    bool forwardMessage(const TBMessage &msg, const int64_t to_chatid);

    // Send message to a channel. This bot must be in the admin group
    bool sendToChannel(const char *channel, const char *message, bool silent = false);

    inline bool sendToChannel(const String &channel, const String &message, bool silent)
    {
        return sendToChannel(channel.c_str(), message.c_str(), silent);
    }

    // Send message to a specific user. In order to work properly two conditions is needed:
    //  - You have to find the userid (for example using the bot @JsonBumpBot  https://t.me/JsonDumpBot)
    //  - User has to start your bot in it's own client. For example send a message with @<your bot name>
    inline bool sendTo(const int64_t userid, const char *message, const char *keyboard = nullptr)
    {
        TBMessage msg;
        msg.chatId = userid;
        return sendMessage(msg, message, keyboard);
    }

    inline bool sendTo(const int64_t userid, const String &message, String keyboard = "")
    {
        return sendTo(userid, message.c_str(), keyboard.c_str());
    }

    // Send a document passing a stream object
    enum DocumentType
    {
        ZIP,
        PDF,
        PHOTO,
        ANIMATION,
        AUDIO,
        VOICE,
        VIDEO,
        CSV,
        JSON
    };
    bool sendDocument(int64_t chat_id, Stream &stream, size_t size,
                        DocumentType doc, const char *filename, const char *caption = nullptr);

    inline bool sendDocument(const TBMessage &msg, Stream &stream, size_t size,
                                DocumentType doc, const char *filename, const char *caption = nullptr)
    {
        return sendDocument(msg.chatId, stream, size, doc, filename, caption);
    }

    // Send a picture passing the url
    bool sendPhotoByUrl(const int64_t &chat_id, const char *url, const char *caption);

    // Send an animation/gif passing the url
    bool sendAnimationByUrl(const int64_t &chat_id, const char *url, const char *caption);

    inline bool sendPhoto(const int64_t &chat_id, const char *url, const char *caption)
    {
        return sendPhotoByUrl(chat_id, url, caption);
    }

    inline bool sendPhoto(const int64_t &chat_id, const String &url, const String &caption)
    {
        return sendPhotoByUrl(chat_id, url.c_str(), caption.c_str());
    }

    inline bool sendPhoto(const TBMessage &msg, const String &url, const String &caption)
    {
        return sendPhotoByUrl(msg.chatId, url.c_str(), caption.c_str());
    }

    // Send a picture passing a stream object
    inline bool sendPhoto(int64_t chat_id, Stream &stream, size_t size, const char *filename = nullptr, const char *caption = nullptr)
    {
        return sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", stream, size, filename, caption);
    }

    inline bool sendPhoto(const TBMessage &msg, Stream &stream, size_t size, const char *filename, const char *caption = nullptr)
    {
        return sendStream(msg.chatId, "sendPhoto", "image/jpeg", "photo", stream, size, filename, caption);
    }

#if FS_SUPPORT == true // #support for <FS.h> is needed
    // Send a picture passing a file and relative filesystem
    inline bool sendPhoto(int64_t chat_id, const char *filename, fs::FS &fs, const char *caption = nullptr)
    {
        File file = fs.open(filename, "r");
        bool res = sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", file, file.size(), file.name(), caption);
        file.close();
        return res;
    }
    inline bool sendPhoto(const TBMessage &msg, const char *filename, fs::FS &fs, const char *caption = nullptr)
    {
        File file = fs.open(filename, "r");
        bool res = sendStream(msg.chatId, "sendPhoto", "image/jpeg", "photo", file, file.size(), file.name(), caption);
        file.close();
        return res;
    }
#endif

    // Send a picture passing a raw buffer
    inline bool sendPhoto(int64_t chat_id, uint8_t *data, size_t size, const char *caption = nullptr)
    {
        return sendBuffer(chat_id, "sendPhoto", "image/jpeg", "photo", data, size, caption);
    }

    inline bool sendPhoto(const TBMessage &msg, uint8_t *data, size_t size, const char *caption = nullptr)
    {
        return sendBuffer(msg.chatId, "sendPhoto", "image/jpeg", "photo", data, size, caption);
    }

    /////////////////////////////// Backward compatibility  ///////////////////////////////////////

    inline bool sendPhotoByUrl(const int64_t &chat_id, const String &url, const String &caption)
    {
        return sendPhotoByUrl(chat_id, url.c_str(), caption.c_str());
    }

    inline bool sendPhotoByUrl(const TBMessage &msg, const String &url, const String &caption)
    {
        return sendPhotoByUrl(msg.chatId, url.c_str(), caption.c_str());
    }

    inline bool sendPhotoByFile(int64_t chat_id, Stream *stream, size_t size)
    {
        return sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", *stream, size, nullptr, nullptr);
    }

#if FS_SUPPORT == true // #support for <FS.h> is needed
    inline bool sendPhotoByFile(int64_t chat_id, const char *filename, fs::FS &fs)
    {
        File file = fs.open(filename, "r");
        Serial.println(file.size());
        bool res = sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", file, file.size(), file.name(), nullptr);
        file.close();
        return res;
    }
#endif
    /////////////////////////////////////////////////////////////////////////////////////////////

    // terminate a query started by pressing an inlineKeyboard button. The steps are:
    // 1) send a message with an inline keyboard
    // 2) wait for a <message> (getNewMessage) of type MessageQuery
    // 3) handle the query and then call endQuery with <message>.callbackQueryID
    // params
    //   msg  : the TBMessage telegram recipient with unique query ID (retrieved with getNewMessage method)
    //   message  : an optional message
    //   alertMode: false -> a simply popup message
    //              true --> an alert message with ok button
    bool endQuery(const TBMessage &msg, const char *message, bool alertMode = false);

    // remove an active reply keyboard for a selected user, sending a message
    // params:
    //   msg      : the TBMessage telegram recipient with the telegram user ID
    //   message  : the message to be show to the selected user ID
    //   selective: enable selective mode (hide the keyboard for specific users only)
    //              Targets: 1) users that are @mentioned in the text of the Message object;
    //                       2) if the bot's message is a reply (has reply_to_message_id), sender of the original message
    // return:
    //   true if no error occurred
    bool removeReplyKeyboard(const TBMessage &msg, const char *message, bool selective = false);

    // Get the current bot name
    // return:
    //   the bot name
    inline const char *getBotName()
    {
        return m_botusername.c_str();
    }

    // Check for no new pending message
    // (to be sure all messages was parsed, before doing something)
    // Example: OTA sketch
    // return:
    //   true if no message
    bool noNewMessage();

    // If bot is a member of a group, return the id of group (negative number)
    // In order to be sure library is able to catch the id,
    // add bot to group while it is running, so the joining message can be parsed
    inline int64_t getGroupId(const TBMessage &msg)
    {
        if (msg.chatId < 0)
            return msg.chatId;
        return 0;
    }

    // keep track of defined inline keybaord in order to call cb function
    // params: pointer to inline keyboard
    inline void addInlineKeyboard(InlineKeyboard *keyb)
    {
        m_keyboards[m_keyboardCount++] = keyb;
    }

    // set custom commands for bot
    // params
    //   command: Text of the command, 1-32 characters. Can contain only lowercase English letters, digits and underscores.
    //   description: Description of the command, 3-256 characters.
    // return:
    //   true if success
    bool setMyCommands(const String &cmd, const String &desc);

    // get list of custom commands defined for bot
    // params:
    //   A string that will contains a JSON-serialized list of bot commands
    void getMyCommands(String &cmdList);

    // clear list of custom commands defined for bot
    // return:
    //   true if success
    bool deleteMyCommands();

    // Edit a previous sent message
    // params:
    //    chat_id: the iD of chat
    //    message_id: the message ID to be edited
    //    txt: the new text
    //    keyboard: the new inline keyboard (if present)
    // return:
    //    true if success
    bool editMessage(int64_t chat_id, int32_t message_id, const String &txt, const String &keyboard);

    inline bool editMessage(const TBMessage &msg, const String &txt, const String &keyboard)
    {
        return editMessage(msg.chatId, msg.messageID, txt, keyboard);
    }

    inline bool editMessage(int64_t chat_id, int32_t message_id, const String &txt, InlineKeyboard &keyboard)
    {
        return editMessage(chat_id, message_id, txt, keyboard.getJSON());
    }

    inline bool editMessage(const TBMessage &msg, const String &txt, InlineKeyboard &keyboard)
    {
        return editMessage(msg.chatId, msg.messageID, txt, keyboard.getJSON());
    }

    // check if connection with server is active
    // returns
    //   true on connected
    bool checkConnection();

    // This callback function will be executed once the message was delivered succesfully
    inline void addSentCallback(SentCallback sentcb, uint32_t timeout = 1000)
    {
        if (sentcb != nullptr)
        {
            m_sentCallback = sentcb;
            m_sentTimeout = timeout;
        }
    }

    // Set the default text formatting option (https://core.telegram.org/bots/api#formatting-options)
    // params:
    //    format: the type of formatting text of sent messages. No formatting, HTML style (default), MarkdownV2 style
    // return:
    //    void
    enum FormatStyle {
        NONE,
        HTML,
        MARKDOWN
    };
    inline void setFormattingStyle(uint8_t format) {
        m_formatType = format;
    }

	inline void setJsonBufferSize(uint32_t jsonBufferSize){
        m_JsonBufferSize = jsonBufferSize;
    }

private:
    Client *telegramClient;
    const char *m_token;
    String m_rxbuffer;
    String m_botusername; // Store only botname, instead TBUser struct

    int32_t m_lastUpdateId = 0;
    uint32_t m_lastUpdateTime;
    uint32_t m_minUpdateTime = MIN_UPDATE_TIME;

    uint32_t m_lastmsg_timestamp;
    bool m_waitingReply;

    InlineKeyboard *m_keyboards[10];
    uint8_t m_keyboardCount = 0;

    void setformData(int64_t chat_id, const char *cmd, const char *type, const char *propName, size_t size,
        String &formData, String &request, const char *filename, const char *caption);
    bool sendStream(int64_t chat_id, const char *command, const char *contentType, const char *binaryPropertyName,
        Stream &stream, size_t size, const char *filename, const char *caption);
    bool sendBuffer(int64_t chat_id, const char *cmd, const char *type, const char *propName, uint8_t *data, size_t size, const char *caption);

    SentCallback m_sentCallback = nullptr;
    bool m_waitSent = false;
    uint32_t m_sentTimeout;
    uint32_t m_lastSentTime;
    uint32_t m_lastSentMsgId;

    uint32_t testReconnectTime;

    uint8_t m_formatType = HTML;
    uint32_t m_JsonBufferSize = BUFFER_BIG;

protected:
    // send commands to the telegram server. For info about commands, check the telegram api https://core.telegram.org/bots/api
    // params
    //   command   : the command to send, i.e. getMe
    //   parameters: optional parameters
    // returns
    //   an empty string if error
    //   a string containing the Telegram JSON response

    bool sendCommand(const char *command, const char *payload, bool blocking = false);

    // query server for new incoming messages
    // returns
    //   http response payload if no error occurred
    // bool getUpdates(JsonDocument &doc);

    bool getUpdates();

    // get some information about the bot
    // params
    //   user: the data structure that will contains the data retreived
    // returns
    //   true if no error occurred
    bool getMe();
};

#endif
