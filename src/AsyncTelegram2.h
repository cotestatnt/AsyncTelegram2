#ifndef ASYNCTELEGRAMV2
#define ASYNCTELEGRAMV2

// for using int_64 data
#define ARDUINOJSON_USE_LONG_LONG 	1
#define ARDUINOJSON_DECODE_UNICODE  1
#include <ArduinoJson.h>

#ifndef STM32
    #define FS_SUPPORT true
    #include <FS.h>
#else
    #define FS_SUPPORT false
#endif


#include "Client.h"
#include "time.h"

#define DEBUG_ENABLE        false
#ifndef DEBUG_ENABLE
    #define DEBUG_ENABLE    false
#endif

/*
    This affect only inline keyboard with at least one callback function defined.
    If you need more than MAX_INLINEKYB_CB distinct keybords with
    callback functions associated to buttons increase this value
*/
#define MAX_INLINEKYB_CB    30

#define SERVER_TIMEOUT      10000
#define MIN_UPDATE_TIME     500

#define BLOCK_SIZE          1436    //2872   // 2 * TCP_MSS

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
MIIEfTCCA2WgAwIBAgIDG+cVMA0GCSqGSIb3DQEBCwUAMGMxCzAJBgNVBAYTAlVT
MSEwHwYDVQQKExhUaGUgR28gRGFkZHkgR3JvdXAsIEluYy4xMTAvBgNVBAsTKEdv
IERhZGR5IENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMTAx
MDcwMDAwWhcNMzEwNTMwMDcwMDAwWjCBgzELMAkGA1UEBhMCVVMxEDAOBgNVBAgT
B0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoTEUdvRGFkZHku
Y29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRpZmljYXRlIEF1
dGhvcml0eSAtIEcyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAv3Fi
CPH6WTT3G8kYo/eASVjpIoMTpsUgQwE7hPHmhUmfJ+r2hBtOoLTbcJjHMgGxBT4H
Tu70+k8vWTAi56sZVmvigAf88xZ1gDlRe+X5NbZ0TqmNghPktj+pA4P6or6KFWp/
3gvDthkUBcrqw6gElDtGfDIN8wBmIsiNaW02jBEYt9OyHGC0OPoCjM7T3UYH3go+
6118yHz7sCtTpJJiaVElBWEaRIGMLKlDliPfrDqBmg4pxRyp6V0etp6eMAo5zvGI
gPtLXcwy7IViQyU0AlYnAZG0O3AqP26x6JyIAX2f1PnbU21gnb8s51iruF9G/M7E
GwM8CetJMVxpRrPgRwIDAQABo4IBFzCCARMwDwYDVR0TAQH/BAUwAwEB/zAOBgNV
HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFDqahQcQZyi27/a9BUFuIMGU2g/eMB8GA1Ud
IwQYMBaAFNLEsNKR1EwRcbNhyz2h/t2oatTjMDQGCCsGAQUFBwEBBCgwJjAkBggr
BgEFBQcwAYYYaHR0cDovL29jc3AuZ29kYWRkeS5jb20vMDIGA1UdHwQrMCkwJ6Al
oCOGIWh0dHA6Ly9jcmwuZ29kYWRkeS5jb20vZ2Ryb290LmNybDBGBgNVHSAEPzA9
MDsGBFUdIAAwMzAxBggrBgEFBQcCARYlaHR0cHM6Ly9jZXJ0cy5nb2RhZGR5LmNv
bS9yZXBvc2l0b3J5LzANBgkqhkiG9w0BAQsFAAOCAQEAWQtTvZKGEacke+1bMc8d
H2xwxbhuvk679r6XUOEwf7ooXGKUwuN+M/f7QnaF25UcjCJYdQkMiGVnOQoWCcWg
OJekxSOTP7QYpgEGRJHjp2kntFolfzq3Ms3dhP8qOCkzpN1nsoX+oYggHFCJyNwq
9kIDN0zmiN/VryTyscPfzLXs4Jlet0lUIDyUGAzHHFIYSaRt4bNYC8nY7NmuHDKO
KHAN4v6mF56ED71XcLNa6R+ghlO773z/aQvgSMO3kwvIClTErF0UZzdsyqUvMQg3
qm5vjLyb4lddJIGvl5echK1srDdMZvNhkREg5L4wn3qkKQmw4TRfZHcYQFHfjDCm
rw==
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
    bool sendMessage(const TBMessage &msg, const char* message, const char* keyboard = nullptr);

    // sendMessage function overloads
    inline bool sendMessage(const TBMessage &msg, const String &message, String keyboard = "")
    {
        return sendMessage(msg, message.c_str(), keyboard.c_str());
    }

    inline bool sendMessage(const TBMessage &msg, const char* message, InlineKeyboard &keyboard)
    {
        return sendMessage(msg, message, keyboard.getJSON().c_str());
    }

    inline bool sendMessage(const TBMessage &msg, const char* message, ReplyKeyboard &keyboard) {
        return sendMessage(msg, message, keyboard.getJSON().c_str());
    }

    // Forward a specific message to user or chat
    bool forwardMessage(const TBMessage &msg, const int32_t to_chatid);

    // Send message to a channel. This bot must be in the admin group
    bool sendToChannel(const char* channel, const char* message, bool silent) ;

    inline bool sendToChannel(const String& channel, const String& message, bool silent) {
        return sendToChannel(channel.c_str(), message.c_str(), silent) ;
    }

    // Send message to a specific user. In order to work properly two conditions is needed:
    //  - You have to find the userid (for example using the bot @JsonBumpBot  https://t.me/JsonDumpBot)
    //  - User has to start your bot in it's own client. For example send a message with @<your bot name>
    inline bool sendTo(const int64_t userid, const char* message, const char*  keyboard = nullptr) {
        TBMessage msg;
        msg.chatId = userid;
        return sendMessage(msg, message, keyboard);
    }

    inline bool sendTo(const int64_t userid, const String &message, String keyboard = "") {
        return sendTo(userid, message.c_str(), keyboard.c_str() );
    }


    // Send a picture passing the url
    bool sendPhotoByUrl(const int64_t& chat_id,  const char* url, const char* caption);

    inline bool sendPhoto(const int64_t& chat_id,  const char* url, const char* caption){
        return sendPhotoByUrl(chat_id, url, caption);
    }

    inline bool sendPhoto(const int64_t& chat_id,  const String& url, const String& caption){
        return sendPhotoByUrl(chat_id, url.c_str(), caption.c_str());
    }

    inline bool sendPhoto(const TBMessage &msg,  const String& url, const String& caption){
        return sendPhotoByUrl(msg.sender.id, url.c_str(), caption.c_str());
    }

    // Send a picture passing a stream object
    inline bool sendPhoto(int64_t chat_id, Stream &stream, size_t size) {
        return sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", stream, size);
    }

    inline bool sendPhoto(const TBMessage &msg, Stream &stream, size_t size) {
        return sendStream(msg.sender.id, "sendPhoto", "image/jpeg", "photo", stream, size);
    }

    #if FS_SUPPORT == true  // #support for <FS.h> is needed
    // Send a picture passing a file and relative filesystem
    inline bool sendPhoto(int64_t chat_id, const char* filename, fs::FS &fs) {
        File file = fs.open(filename, "r");
        bool res = sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", file, file.size());
        file.close();
        return res;
    }
    inline bool sendPhoto(const TBMessage &msg, const char* filename, fs::FS &fs) {
        File file = fs.open(filename, "r");
        bool res = sendStream(msg.sender.id, "sendPhoto", "image/jpeg", "photo", file, file.size());
        file.close();
        return res;
    }
    #endif

    // Send a picture passing a raw buffer
    inline bool sendPhoto(int64_t chat_id, uint8_t *data, size_t size) {
        return sendBuffer(chat_id, "sendPhoto", "image/jpeg", "photo", data, size);
    }

    inline bool sendPhoto(const TBMessage &msg, uint8_t *data, size_t size) {
        return sendBuffer(msg.sender.id, "sendPhoto", "image/jpeg", "photo", data, size);
    }


    /////////////////////////////// Backward compatibility  ///////////////////////////////////////

    inline bool sendPhotoByUrl(const int64_t& chat_id,  const String& url, const String& caption){
        return sendPhotoByUrl(chat_id, url.c_str(), caption.c_str());
    }

    inline bool sendPhotoByUrl(const TBMessage &msg,  const String& url, const String& caption){
        return sendPhotoByUrl(msg.sender.id, url.c_str(), caption.c_str());
    }

    inline bool sendPhotoByFile(int64_t chat_id, Stream *stream, size_t size) {
        return sendStream(chat_id,"sendPhoto", "image/jpeg", "photo", *stream, size);
    }

    #if FS_SUPPORT == true  // #support for <FS.h> is needed
    inline bool sendPhotoByFile(int64_t chat_id, const char* filename, fs::FS &fs) {
        File file = fs.open(filename, "r");
        Serial.println(file.size());
        bool res = sendStream(chat_id,"sendPhoto", "image/jpeg", "photo", file, file.size());
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
    bool endQuery(const TBMessage &msg, const char* message, bool alertMode = false);

    // remove an active reply keyboard for a selected user, sending a message
    // params:
    //   msg      : the TBMessage telegram recipient with the telegram user ID
    //   message  : the message to be show to the selected user ID
    //   selective: enable selective mode (hide the keyboard for specific users only)
    //              Targets: 1) users that are @mentioned in the text of the Message object;
    //                       2) if the bot's message is a reply (has reply_to_message_id), sender of the original message
    // return:
    //   true if no error occurred
    bool removeReplyKeyboard(const TBMessage &msg, const char* message, bool selective = false);

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

    // If bot is a member of a group, return the id of group (negative number)
    // In order to be sure library is able to catch the id,
    // add bot to group while it is running, so the joining message can be parsed
    inline int64_t getGroupId(const TBMessage &msg) {
        if(msg.group.id < 0)
            return msg.group.id;
        return 0;
    }

    // keep track of defined inline keybaord in order to call cb function
    // params: pointer to inline keyboard
    inline void addInlineKeyboard(InlineKeyboard* keyb)
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
	bool editMessage(int32_t chat_id, int32_t message_id, const String& txt, const String &keyboard);

    inline bool editMessage(const TBMessage &msg, const String& txt, const String &keyboard) {
		return editMessage(msg.sender.id, msg.messageID, txt, keyboard);
	}

    inline bool editMessage(int32_t chat_id, int32_t message_id, const String& txt, InlineKeyboard &keyboard) {
        return editMessage(chat_id, message_id, txt, keyboard.getJSON());
    }

	inline bool editMessage(const TBMessage &msg, const String& txt, InlineKeyboard &keyboard) {
		return editMessage(msg.sender.id, msg.messageID, txt, keyboard.getJSON());
	}


private:
    Client*         telegramClient;
    const char*     m_token;
    String          m_rxbuffer;
    String          m_botusername;      // Store only botname, instead TBUser struct

    int32_t         m_lastUpdateId = 0;
    uint32_t        m_lastUpdateTime;
    uint32_t        m_minUpdateTime = MIN_UPDATE_TIME;

    uint32_t        m_lastmsg_timestamp;
    bool            m_waitingReply;

    InlineKeyboard* m_keyboards[10];
    uint8_t         m_keyboardCount = 0;

    void setformData(int64_t chat_id, const char* cmd, const char* type, const char* propName, size_t size, String &formData, String& request);
    bool sendStream( int64_t chat_id, const char* command, const char* contentType, const char* binaryPropertyName, Stream& stream, size_t size);
    bool sendBuffer(int64_t chat_id, const char* cmd, const char* type, const char* propName, uint8_t *data, size_t size);

    // send commands to the telegram server. For info about commands, check the telegram api https://core.telegram.org/bots/api
    // params
    //   command   : the command to send, i.e. getMe
    //   parameters: optional parameters
    // returns
    //   an empty string if error
    //   a string containing the Telegram JSON response

    bool sendCommand(const char* const &command, const char* payload, bool blocking = false);

        // query server for new incoming messages
    // returns
    //   http response payload if no error occurred
    //bool getUpdates(JsonDocument &doc);

    bool getUpdates();

    // get some information about the bot
    // params
    //   user: the data structure that will contains the data retreived
    // returns
    //   true if no error occurred
    bool getMe();

    // check if connection with server is active
    // returns
    //   true on connected
    bool checkConnection();

};

#endif




/*
static const unsigned char telegram_cer[] PROGMEM = {
    0x30, 0x82, 0x04, 0x7d, 0x30, 0x82, 0x03, 0x65, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x03, 0x1b, 0xe7, 0x15,
    0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x30, 0x63, 0x31,
    0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x21, 0x30, 0x1f, 0x06, 0x03,
    0x55, 0x04, 0x0a, 0x13, 0x18, 0x54, 0x68, 0x65, 0x20, 0x47, 0x6f, 0x20, 0x44, 0x61, 0x64, 0x64, 0x79, 0x20,
    0x47, 0x72, 0x6f, 0x75, 0x70, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x31, 0x31, 0x30, 0x2f, 0x06, 0x03, 0x55,
    0x04, 0x0b, 0x13, 0x28, 0x47, 0x6f, 0x20, 0x44, 0x61, 0x64, 0x64, 0x79, 0x20, 0x43, 0x6c, 0x61, 0x73, 0x73,
    0x20, 0x32, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x41,
    0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x34, 0x30, 0x31, 0x30, 0x31,
    0x30, 0x37, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x33, 0x31, 0x30, 0x35, 0x33, 0x30, 0x30, 0x37, 0x30,
    0x30, 0x30, 0x30, 0x5a, 0x30, 0x81, 0x83, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
    0x55, 0x53, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x07, 0x41, 0x72, 0x69, 0x7a, 0x6f,
    0x6e, 0x61, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x0a, 0x53, 0x63, 0x6f, 0x74, 0x74,
    0x73, 0x64, 0x61, 0x6c, 0x65, 0x31, 0x1a, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x11, 0x47, 0x6f,
    0x44, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x31, 0x31, 0x30,
    0x2f, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x28, 0x47, 0x6f, 0x20, 0x44, 0x61, 0x64, 0x64, 0x79, 0x20, 0x52,
    0x6f, 0x6f, 0x74, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x20, 0x41, 0x75,
    0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x20, 0x2d, 0x20, 0x47, 0x32, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0xbf, 0x71, 0x62, 0x08, 0xf1, 0xfa, 0x59, 0x34, 0xf7,
    0x1b, 0xc9, 0x18, 0xa3, 0xf7, 0x80, 0x49, 0x58, 0xe9, 0x22, 0x83, 0x13, 0xa6, 0xc5, 0x20, 0x43, 0x01, 0x3b,
    0x84, 0xf1, 0xe6, 0x85, 0x49, 0x9f, 0x27, 0xea, 0xf6, 0x84, 0x1b, 0x4e, 0xa0, 0xb4, 0xdb, 0x70, 0x98, 0xc7,
    0x32, 0x01, 0xb1, 0x05, 0x3e, 0x07, 0x4e, 0xee, 0xf4, 0xfa, 0x4f, 0x2f, 0x59, 0x30, 0x22, 0xe7, 0xab, 0x19,
    0x56, 0x6b, 0xe2, 0x80, 0x07, 0xfc, 0xf3, 0x16, 0x75, 0x80, 0x39, 0x51, 0x7b, 0xe5, 0xf9, 0x35, 0xb6, 0x74,
    0x4e, 0xa9, 0x8d, 0x82, 0x13, 0xe4, 0xb6, 0x3f, 0xa9, 0x03, 0x83, 0xfa, 0xa2, 0xbe, 0x8a, 0x15, 0x6a, 0x7f,
    0xde, 0x0b, 0xc3, 0xb6, 0x19, 0x14, 0x05, 0xca, 0xea, 0xc3, 0xa8, 0x04, 0x94, 0x3b, 0x46, 0x7c, 0x32, 0x0d,
    0xf3, 0x00, 0x66, 0x22, 0xc8, 0x8d, 0x69, 0x6d, 0x36, 0x8c, 0x11, 0x18, 0xb7, 0xd3, 0xb2, 0x1c, 0x60, 0xb4,
    0x38, 0xfa, 0x02, 0x8c, 0xce, 0xd3, 0xdd, 0x46, 0x07, 0xde, 0x0a, 0x3e, 0xeb, 0x5d, 0x7c, 0xc8, 0x7c, 0xfb,
    0xb0, 0x2b, 0x53, 0xa4, 0x92, 0x62, 0x69, 0x51, 0x25, 0x05, 0x61, 0x1a, 0x44, 0x81, 0x8c, 0x2c, 0xa9, 0x43,
    0x96, 0x23, 0xdf, 0xac, 0x3a, 0x81, 0x9a, 0x0e, 0x29, 0xc5, 0x1c, 0xa9, 0xe9, 0x5d, 0x1e, 0xb6, 0x9e, 0x9e,
    0x30, 0x0a, 0x39, 0xce, 0xf1, 0x88, 0x80, 0xfb, 0x4b, 0x5d, 0xcc, 0x32, 0xec, 0x85, 0x62, 0x43, 0x25, 0x34,
    0x02, 0x56, 0x27, 0x01, 0x91, 0xb4, 0x3b, 0x70, 0x2a, 0x3f, 0x6e, 0xb1, 0xe8, 0x9c, 0x88, 0x01, 0x7d, 0x9f,
    0xd4, 0xf9, 0xdb, 0x53, 0x6d, 0x60, 0x9d, 0xbf, 0x2c, 0xe7, 0x58, 0xab, 0xb8, 0x5f, 0x46, 0xfc, 0xce, 0xc4,
    0x1b, 0x03, 0x3c, 0x09, 0xeb, 0x49, 0x31, 0x5c, 0x69, 0x46, 0xb3, 0xe0, 0x47, 0x02, 0x03, 0x01, 0x00, 0x01,
    0xa3, 0x82, 0x01, 0x17, 0x30, 0x82, 0x01, 0x13, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
    0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x1d, 0x0f, 0x01, 0x01, 0xff, 0x04,
    0x04, 0x03, 0x02, 0x01, 0x06, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x3a, 0x9a,
    0x85, 0x07, 0x10, 0x67, 0x28, 0xb6, 0xef, 0xf6, 0xbd, 0x05, 0x41, 0x6e, 0x20, 0xc1, 0x94, 0xda, 0x0f, 0xde,
    0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0xd2, 0xc4, 0xb0, 0xd2, 0x91,
    0xd4, 0x4c, 0x11, 0x71, 0xb3, 0x61, 0xcb, 0x3d, 0xa1, 0xfe, 0xdd, 0xa8, 0x6a, 0xd4, 0xe3, 0x30, 0x34, 0x06,
    0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01, 0x04, 0x28, 0x30, 0x26, 0x30, 0x24, 0x06, 0x08, 0x2b,
    0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x86, 0x18, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6f, 0x63,
    0x73, 0x70, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x30, 0x32, 0x06,
    0x03, 0x55, 0x1d, 0x1f, 0x04, 0x2b, 0x30, 0x29, 0x30, 0x27, 0xa0, 0x25, 0xa0, 0x23, 0x86, 0x21, 0x68, 0x74,
    0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72, 0x6c, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63,
    0x6f, 0x6d, 0x2f, 0x67, 0x64, 0x72, 0x6f, 0x6f, 0x74, 0x2e, 0x63, 0x72, 0x6c, 0x30, 0x46, 0x06, 0x03, 0x55,
    0x1d, 0x20, 0x04, 0x3f, 0x30, 0x3d, 0x30, 0x3b, 0x06, 0x04, 0x55, 0x1d, 0x20, 0x00, 0x30, 0x33, 0x30, 0x31,
    0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x02, 0x01, 0x16, 0x25, 0x68, 0x74, 0x74, 0x70, 0x73, 0x3a,
    0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74, 0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f,
    0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72, 0x79, 0x2f, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x59, 0x0b, 0x53,
    0xbd, 0x92, 0x86, 0x11, 0xa7, 0x24, 0x7b, 0xed, 0x5b, 0x31, 0xcf, 0x1d, 0x1f, 0x6c, 0x70, 0xc5, 0xb8, 0x6e,
    0xbe, 0x4e, 0xbb, 0xf6, 0xbe, 0x97, 0x50, 0xe1, 0x30, 0x7f, 0xba, 0x28, 0x5c, 0x62, 0x94, 0xc2, 0xe3, 0x7e,
    0x33, 0xf7, 0xfb, 0x42, 0x76, 0x85, 0xdb, 0x95, 0x1c, 0x8c, 0x22, 0x58, 0x75, 0x09, 0x0c, 0x88, 0x65, 0x67,
    0x39, 0x0a, 0x16, 0x09, 0xc5, 0xa0, 0x38, 0x97, 0xa4, 0xc5, 0x23, 0x93, 0x3f, 0xb4, 0x18, 0xa6, 0x01, 0x06,
    0x44, 0x91, 0xe3, 0xa7, 0x69, 0x27, 0xb4, 0x5a, 0x25, 0x7f, 0x3a, 0xb7, 0x32, 0xcd, 0xdd, 0x84, 0xff, 0x2a,
    0x38, 0x29, 0x33, 0xa4, 0xdd, 0x67, 0xb2, 0x85, 0xfe, 0xa1, 0x88, 0x20, 0x1c, 0x50, 0x89, 0xc8, 0xdc, 0x2a,
    0xf6, 0x42, 0x03, 0x37, 0x4c, 0xe6, 0x88, 0xdf, 0xd5, 0xaf, 0x24, 0xf2, 0xb1, 0xc3, 0xdf, 0xcc, 0xb5, 0xec,
    0xe0, 0x99, 0x5e, 0xb7, 0x49, 0x54, 0x20, 0x3c, 0x94, 0x18, 0x0c, 0xc7, 0x1c, 0x52, 0x18, 0x49, 0xa4, 0x6d,
    0xe1, 0xb3, 0x58, 0x0b, 0xc9, 0xd8, 0xec, 0xd9, 0xae, 0x1c, 0x32, 0x8e, 0x28, 0x70, 0x0d, 0xe2, 0xfe, 0xa6,
    0x17, 0x9e, 0x84, 0x0f, 0xbd, 0x57, 0x70, 0xb3, 0x5a, 0xe9, 0x1f, 0xa0, 0x86, 0x53, 0xbb, 0xef, 0x7c, 0xff,
    0x69, 0x0b, 0xe0, 0x48, 0xc3, 0xb7, 0x93, 0x0b, 0xc8, 0x0a, 0x54, 0xc4, 0xac, 0x5d, 0x14, 0x67, 0x37, 0x6c,
    0xca, 0xa5, 0x2f, 0x31, 0x08, 0x37, 0xaa, 0x6e, 0x6f, 0x8c, 0xbc, 0x9b, 0xe2, 0x57, 0x5d, 0x24, 0x81, 0xaf,
    0x97, 0x97, 0x9c, 0x84, 0xad, 0x6c, 0xac, 0x37, 0x4c, 0x66, 0xf3, 0x61, 0x91, 0x11, 0x20, 0xe4, 0xbe, 0x30,
    0x9f, 0x7a, 0xa4, 0x29, 0x09, 0xb0, 0xe1, 0x34, 0x5f, 0x64, 0x77, 0x18, 0x40, 0x51, 0xdf, 0x8c, 0x30, 0xa6, 0xaf
};
*/
