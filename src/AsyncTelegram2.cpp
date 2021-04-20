#include "AsyncTelegram2.h"

#if DEBUG_ENABLE
#define debugJson(X, Y)  { log_debug(); Serial.println(); serializeJsonPretty(X, Y); Serial.println();}
#define errorJson(E)  { log_error(); Serial.println(); Serial.println(E);}
#else
#define debugJson(X, Y)
#define errorJson(E)
#endif

AsyncTelegram2::AsyncTelegram2(Client &client)
{
    m_botusername.reserve(32); // Telegram username is 5-32 chars lenght
    m_rxbuffer.reserve(BUFFER_BIG);
    this->telegramClient = &client;
    m_minUpdateTime = MIN_UPDATE_TIME;
}

AsyncTelegram2::~AsyncTelegram2() {};


bool AsyncTelegram2::checkConnection()
{
    // Start connection with Telegramn server (if necessary)
    if (!telegramClient->connected()) {
        static uint32_t lastCTime;
        m_lastmsg_timestamp = millis();
        log_debug("Start handshaking...");
        if (!telegramClient->connect(TELEGRAM_HOST, TELEGRAM_PORT)) {
            Serial.printf("\n\nUnable to connect to Telegram server\n");
        }
        else {
            log_debug("Connected using Telegram hostname\n"
                      "Last connection was %d seconds ago\n",
                      (int)(millis() - lastCTime)/1000);
            lastCTime = millis();
        }
    }
    return telegramClient->connected();
}


bool AsyncTelegram2::begin()
{
    checkConnection();
    return getMe();
}


bool AsyncTelegram2::reset(void)
{
    log_debug("Restart Telegram connection\n");
    telegramClient->stop();
    m_lastmsg_timestamp = millis();
    m_waitingReply = false;
    return checkConnection();
}


// Blocking https POST to server (used with ESP8266)
/*
bool AsyncTelegram2::sendCommand(const char* const &command, JsonDocument &doc, bool blocking )
{
    if(checkConnection()) {
        // JsonDocument doc is used as input for request preparation and then reused as output result
        String httpBuffer((char *)0);
        httpBuffer.reserve(BUFFER_BIG);
        httpBuffer = "POST https://" TELEGRAM_HOST "/bot";
        httpBuffer += m_token;
        httpBuffer += "/";
        httpBuffer += command;
        // Let's use 1.0 protocol in order to avoid chunked transfer encoding
        httpBuffer += " HTTP/1.0" "\nHost: api.telegram.org" "\nConnection: keep-alive" "\nContent-Type: application/json";
        httpBuffer += "\nContent-Length: ";
        httpBuffer += measureJson(doc);
        httpBuffer += "\n\n";
        httpBuffer += doc.as<String>();
         telegramClient->print(httpBuffer);
        // Serial.println(httpBuffer);

        m_waitingReply = true;
        // Blocking mode
        if (blocking) {
            httpBuffer = "";
            // skip headers
            if (telegramClient->connected()) {
                httpBuffer = telegramClient->readStringUntil('\n');
                while (httpBuffer != "\r") {
                    httpBuffer = telegramClient->readStringUntil('\n');
                }
            }
            // If there are incoming bytes available from the server, read them and print them:
            httpBuffer = "";
            while (telegramClient->available()) {
                yield();
                httpBuffer  += (char) telegramClient->read();
            }
            m_waitingReply = false;

            DeserializationError err = deserializeJson(doc, httpBuffer);
            debugJson(doc, Serial);
            return (err == 0 && doc.containsKey("ok"));
        }
    }
    return false;
}


bool AsyncTelegram2::getUpdates(JsonDocument &doc){
    // No response from Telegram server for a long time
    if(millis() - m_lastmsg_timestamp > 10*m_minUpdateTime) {
        reset();
    }

    // Send message to Telegram server only if enough time has passed since last
    if(millis() - m_lastUpdateTime > m_minUpdateTime){
        m_lastUpdateTime = millis();

        // If previuos reply from server was received (and parsed)
        if( m_waitingReply == false ) {
            StaticJsonDocument<BUFFER_SMALL> updateDoc;
            updateDoc["limit"] = 1;
            updateDoc["timeout"] = 0;    // zero for short polling.
            //updateDoc["allowed_updates"] = "message,callback_query";
            if (m_lastUpdateId != 0) {
                updateDoc["offset"] = m_lastUpdateId;
            }
            sendCommand("getUpdates", updateDoc);
        }
    }

    DeserializationError err;
    if(telegramClient->connected() && telegramClient->available()) {
        // We have a message, parse data received
        //uint32_t t1 = millis();
        bool close_connection = false;
        String payload((char *)0);
        payload.reserve(BUFFER_BIG);
        // Skip headers
        payload = telegramClient->readStringUntil('\n');
        while (payload != "\r") {
            yield();
            payload = telegramClient->readStringUntil('\n');
            if (payload.indexOf("close") > -1) {
                close_connection = true;
                log_debug("%s\n", payload.c_str());
            }
        }

        // If there are incoming bytes available from the server, read them and store:
        payload = "";
        while (telegramClient->available()){
            yield();
            payload += (char) telegramClient->read();
        }
        err = deserializeJson(doc, payload);

        m_lastmsg_timestamp = millis();
        m_waitingReply = false;
        if (close_connection)
            telegramClient->stop();
        //Serial.println(millis() - t1);
    }
    return (!err && doc.containsKey("ok"));
}

*/


bool AsyncTelegram2::sendCommand(const char* const &command, const char* payload, bool blocking )
{
    if(checkConnection()) {
        // JsonDocument doc is used as input for request preparation and then reused as output result
        String httpBuffer((char *)0);
        httpBuffer.reserve(BUFFER_BIG);
        httpBuffer = "POST https://" TELEGRAM_HOST "/bot";
        httpBuffer += m_token;
        httpBuffer += "/";
        httpBuffer += command;
        // Let's use 1.0 protocol in order to avoid chunked transfer encoding
        httpBuffer += " HTTP/1.0" "\nHost: api.telegram.org" "\nConnection: keep-alive" "\nContent-Type: application/json";
        httpBuffer += "\nContent-Length: ";
        httpBuffer += strlen(payload);
        httpBuffer += "\n\n";
        httpBuffer += payload;
        telegramClient->print(httpBuffer);
        //Serial.println(httpBuffer);

        m_waitingReply = true;
        // Blocking mode
        if (blocking) {
            char endOfHeaders[] = "\r\n\r\n";
            if (!telegramClient->find(endOfHeaders)) {
                log_error("Invalid HTTP response");
                telegramClient->stop();
                return false;
            }
            // If there are incoming bytes available from the server, read them and print them:
            m_rxbuffer = "";
            while (telegramClient->available()) {
                yield();
                m_rxbuffer  += (char) telegramClient->read();
            }
            m_waitingReply = false;
            if(m_rxbuffer.indexOf("ok") > -1) {
                return true;
            }
        }
    }
    return false;
}


bool AsyncTelegram2::getUpdates(){
    // No response from Telegram server for a long time
    if(millis() - m_lastmsg_timestamp > 10*m_minUpdateTime) {
        reset();
    }

    // Send message to Telegram server only if enough time has passed since last
    if(millis() - m_lastUpdateTime > m_minUpdateTime){
        m_lastUpdateTime = millis();

        // If previuos reply from server was received (and parsed)
        if( m_waitingReply == false ) {
            char payload[BUFFER_SMALL];
            snprintf(payload, BUFFER_SMALL, "{\"limit\":1,\"timeout\":0,\"offset\":%d}", m_lastUpdateId);
            sendCommand("getUpdates", payload);
        }
    }

    if(telegramClient->connected() && telegramClient->available()) {
        // We have a message, parse data received
        bool close_connection = false;

        // Skip headers
        while (telegramClient->connected()) {
            String line = telegramClient->readStringUntil('\n');
            if (line == "\r") { break; }
            if (line.indexOf("close") > -1) { close_connection = true; }
        }

        // If there are incoming bytes available from the server, read them and store:
        m_rxbuffer = "";
        while (telegramClient->available()) {
            yield();
            m_rxbuffer  += (char) telegramClient->read();
        }
        m_waitingReply = false;
        m_lastmsg_timestamp = millis();

        if (close_connection)
            telegramClient->stop();

        if(m_rxbuffer.indexOf("ok") < 0) {
            log_error("%s", m_rxbuffer.c_str());
            return false;
        }
        return true;
    }
    return false;
}


// Parse message received from Telegram server
MessageType AsyncTelegram2::getNewMessage(TBMessage &message )
{
    message.messageType = MessageNoData;

    // We have a message, parse data received
    if (getUpdates()) {
        DynamicJsonDocument updateDoc(BUFFER_BIG);
        deserializeJson(updateDoc, m_rxbuffer);
        m_rxbuffer = "";

        if (!updateDoc.containsKey("result")) {
            log_error("deserializeJson() failed with code");
            serializeJsonPretty(updateDoc, Serial);
            return MessageNoData;
        }

        uint32_t updateID = updateDoc["result"][0]["update_id"];
        if (!updateID) return MessageNoData;

        m_lastUpdateId = updateID + 1;
        debugJson(updateDoc, Serial);



        if (updateDoc["result"][0]["callback_query"]["id"]) {
            // this is a callback query
            message.chatId            = updateDoc["result"][0]["callback_query"]["message"]["chat"]["id"];
            message.sender.id         = updateDoc["result"][0]["callback_query"]["from"]["id"];
            message.sender.username   = updateDoc["result"][0]["callback_query"]["from"]["username"];
            message.sender.firstName  = updateDoc["result"][0]["callback_query"]["from"]["first_name"];
            message.sender.lastName   = updateDoc["result"][0]["callback_query"]["from"]["last_name"];
            message.messageID         = updateDoc["result"][0]["callback_query"]["message"]["message_id"];
            message.date              = updateDoc["result"][0]["callback_query"]["message"]["date"];
            message.chatInstance      = updateDoc["result"][0]["callback_query"]["chat_instance"];
            message.callbackQueryID   = updateDoc["result"][0]["callback_query"]["id"];
            message.callbackQueryData = updateDoc["result"][0]["callback_query"]["data"];
            message.text              = updateDoc["result"][0]["callback_query"]["message"]["text"].as<String>();
            message.messageType       = MessageQuery;

            // Check if callback function is defined for this button query
            for(uint8_t i=0; i<m_keyboardCount; i++)
                m_keyboards[i]->checkCallback(message);
        }
        else if (updateDoc["result"][0]["message"]["message_id"]) {
            // this is a message
            message.messageID        = updateDoc["result"][0]["message"]["message_id"];
            message.chatId           = updateDoc["result"][0]["message"]["chat"]["id"];
            message.sender.id        = updateDoc["result"][0]["message"]["from"]["id"];
            message.sender.username  = updateDoc["result"][0]["message"]["from"]["username"];
            message.sender.firstName = updateDoc["result"][0]["message"]["from"]["first_name"];
            message.sender.lastName  = updateDoc["result"][0]["message"]["from"]["last_name"];
            message.group.id         = updateDoc["result"][0]["message"]["chat"]["id"];
            message.group.title      = updateDoc["result"][0]["message"]["chat"]["title"];
            message.date             = updateDoc["result"][0]["message"]["date"];

            if (updateDoc["result"][0]["message"]["location"]) {
                // this is a location message
                message.location.longitude = updateDoc["result"][0]["message"]["location"]["longitude"];
                message.location.latitude = updateDoc["result"][0]["message"]["location"]["latitude"];
                message.messageType = MessageLocation;
            }
            else if (updateDoc["result"][0]["message"]["contact"]) {
                // this is a contact message
                message.contact.id          = updateDoc["result"][0]["message"]["contact"]["user_id"];
                message.contact.firstName   = updateDoc["result"][0]["message"]["contact"]["first_name"];
                message.contact.lastName    = updateDoc["result"][0]["message"]["contact"]["last_name"];
                message.contact.phoneNumber = updateDoc["result"][0]["message"]["contact"]["phone_number"];
                message.contact.vCard       = updateDoc["result"][0]["message"]["contact"]["vcard"];
                message.messageType = MessageContact;
            }
            else if (updateDoc["result"][0]["message"]["document"]) {
                // this is a document message
                message.document.file_id      = updateDoc["result"][0]["message"]["document"]["file_id"];
                message.document.file_name    = updateDoc["result"][0]["message"]["document"]["file_name"];
                //message.text                  = updateDoc["result"][0]["message"]["caption"].as<String>();
                message.document.file_exists  = getFile(message.document);
                message.messageType           = MessageDocument;
            }
            else if (updateDoc["result"][0]["message"]["reply_to_message"]) {
                // this is a reply to message
                message.text        = updateDoc["result"][0]["message"]["text"].as<String>();
                message.messageType = MessageReply;
            }
            else if (updateDoc["result"][0]["message"]["text"]) {
                // this is a text message
                message.text        = updateDoc["result"][0]["message"]["text"].as<String>();
                message.messageType = MessageText;
            }
        }
        return message.messageType;
    }
    return MessageNoData;   // waiting for reply from server
}


// Blocking getMe function (we wait for a reply from Telegram server)
bool AsyncTelegram2::getMe()
{
    // getMe has to be blocking (wait server reply)
    if (!sendCommand("getMe", "", true)) {
        log_error("getMe error ");
        return false;
    }
    StaticJsonDocument<BUFFER_SMALL> smallDoc;
    deserializeJson(smallDoc, m_rxbuffer);
    debugJson(smallDoc, Serial);
    m_botusername = smallDoc["result"]["username"].as<String>();
    return true;
}


bool AsyncTelegram2::getFile(TBDocument &doc)
{
    char cmd[BUFFER_SMALL];
    snprintf(cmd, BUFFER_SMALL, "getFile?file_id=%s", doc.file_id);

    // getFile has to be blocking (wait server reply
    if (!sendCommand(cmd, "", true)) {
        log_error("getFile error");
        return false;
    }
    StaticJsonDocument<BUFFER_MEDIUM> fileDoc;
    deserializeJson(fileDoc, m_rxbuffer);
    debugJson(fileDoc, Serial);
    doc.file_path = "https://api.telegram.org/file/bot" ;
    doc.file_path += m_token;
    doc.file_path += "/";
    doc.file_path += fileDoc["result"]["file_path"].as<String>();
    doc.file_size  = fileDoc["result"]["file_size"].as<long>();
    return true;
}


bool AsyncTelegram2::noNewMessage() {
    return sendCommand("getUpdates", "", true);
}


bool AsyncTelegram2::sendMessage(const TBMessage &msg, const char* message, String keyboard)
{
    if (!strlen(message)) return false;

    DynamicJsonDocument root(BUFFER_BIG);
    // Backward compatibility
    root["chat_id"] = msg.sender.id != 0 ? msg.sender.id : msg.chatId;
    root["text"] = message;

    if(msg.isMarkdownEnabled)
        root["parse_mode"] = "MarkdownV2";

    if(msg.isHTMLenabled)
        root["parse_mode"] = "HTML";

    if(msg.disable_notification)
        root["disable_notification"] = true;

    if (keyboard.length() || msg.force_reply) {
        StaticJsonDocument<BUFFER_MEDIUM> doc;
        deserializeJson(doc, keyboard);
        JsonObject myKeyb = doc.as<JsonObject>();
        root["reply_markup"] = myKeyb;
        if(msg.force_reply) {
            root["reply_markup"]["selective"] = true,
            root["reply_markup"]["force_reply"] = true;
        }
    }
    root.shrinkToFit();

    size_t len = measureJson(root);
    char payload[len];
    serializeJson(root, payload, len);

    debugJson(root, Serial);
    const bool result = sendCommand("sendMessage", payload, NULL);
    return result;
}


bool AsyncTelegram2::forwardMessage(const TBMessage &msg, const int32_t to_chatid)
{
    char payload[BUFFER_SMALL];
    snprintf(payload, BUFFER_SMALL,
        "{\"chat_id\":%d,\"from_chat_id\":%lld,\"message_id\":%d}",
        to_chatid, msg.chatId, msg.messageID);

    const bool result = sendCommand("forwardMessage", payload);
    log_debug("%s", payload);
    return result;
}


bool AsyncTelegram2::sendPhotoByUrl(const uint32_t& chat_id,  const char* url, const char* caption)
{
    if (!strlen(url)) return false;

    char payload[BUFFER_SMALL];
    snprintf(payload, BUFFER_SMALL,
        "{\"chat_id\":%d,\"photo\":\"%s\",\"caption\":\"%s\"}",
        chat_id, url, caption);

    const bool result = sendCommand("sendPhoto", payload);
    log_debug("%s", payload);
    return result;
}


bool AsyncTelegram2::sendToChannel(const char* channel, const char* message, bool silent) {
    if (!strlen(message)) return false;

    char payload[BUFFER_MEDIUM];
    snprintf(payload, BUFFER_MEDIUM,
        "{\"chat_id\":%s,\"text\":\"%s\",\"silent\":%s}",
        channel, message, silent ? "true" : "false");

    const bool result = sendCommand("sendMessage", payload);
    log_debug("%s", payload);
    return result;
}


bool AsyncTelegram2::endQuery(const TBMessage &msg, const char* message, bool alertMode)
{
    if (! msg.callbackQueryID) return false;
    char payload[BUFFER_SMALL];
    snprintf(payload, BUFFER_SMALL,
        "{\"callback_query_id\":%s,\"text\":\"%s\",\"cache_time\":30,\"show_alert\":%s}",
        msg.callbackQueryID, message, alertMode ? "true" : "false");
    const bool result = sendCommand("answerCallbackQuery", payload, true);
    return result;
}


bool AsyncTelegram2::removeReplyKeyboard(const TBMessage &msg, const char* message, bool selective)
{
    char payload[BUFFER_SMALL];
    snprintf(payload, BUFFER_SMALL,
        "{\"remove_keyboard\":true,\"selective\":%s}", selective ? "true" : "false");
    const bool result = sendMessage(msg, message, payload);
    return result;
}


bool AsyncTelegram2::sendDocument(uint32_t chat_id, const char* command, const char* contentType, const char* binaryPropertyName, Stream* stream, size_t size)
{
    #define BOUNDARY            "----WebKitFormBoundary7MA4YWxkTrZu0gW"
    #define END_BOUNDARY        "\r\n--" BOUNDARY "--\r\n"

    if (telegramClient->connected()) {
        m_waitingReply = true;

        String formData((char *)0);
        formData = "--" BOUNDARY;
        formData += "\r\nContent-disposition: form-data; name=\"chat_id\"\r\n\r\n";
        formData += chat_id;
        formData += "\r\n--" BOUNDARY;
        formData += "\r\nContent-disposition: form-data; name=\"";
        formData += binaryPropertyName;
        formData += "\"; filename=\"";
        formData += "image.jpg";
        formData += "\"\r\nContent-Type: ";
        formData += contentType;
        formData += "\r\n\r\n";
        int contentLength = size + formData.length() + strlen(END_BOUNDARY);

        String request((char *)0);
        request = "POST /bot";
        request += m_token;
        request += "/";
        request += command;
        request += " HTTP/1.1\r\nHost: " TELEGRAM_HOST;
        request += "\r\nContent-Length: ";
        request += contentLength;
        request += "\r\nContent-Type: multipart/form-data; boundary=" BOUNDARY "\r\n";

        // Send POST request to host
        telegramClient->println(request);

        // Body of request
        telegramClient->print(formData);

        // uint32_t t1 = millis();
        uint8_t buff[BLOCK_SIZE];
        uint16_t count = 0;
        while (stream->available()) {
            yield();
            buff[count++] = (uint8_t)stream->read();
            if (count == BLOCK_SIZE ) {
                //log_debug("\nSending binary photo full buffer");
                telegramClient->write((const uint8_t *)buff, BLOCK_SIZE);
                count = 0;
                m_lastmsg_timestamp = millis();
            }
        }
        if (count > 0) {
            //log_debug("\nSending binary photo remaining buffer");
            telegramClient->write((const uint8_t *)buff, count);
        }

        telegramClient->print(END_BOUNDARY);

        // Serial.printf("\nUpload time: %d\n", millis() - t1);
        m_lastmsg_timestamp = millis();
        m_waitingReply = false;
    }
    else {
        Serial.println("\nError: client not connected");
        return false;
    }
    return true;
}
