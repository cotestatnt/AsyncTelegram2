#include "AsyncTelegram2.h"

#define HEADERS_END "\r\n\r\n"

AsyncTelegram2::AsyncTelegram2(Client &client, uint32_t bufferSize)
{
    m_botusername.reserve(32); // Telegram username is 5-32 chars lenght
    m_rxbuffer.reserve(bufferSize);
    this->telegramClient = &client;
    m_minUpdateTime = MIN_UPDATE_TIME;
}

AsyncTelegram2::~AsyncTelegram2(){};

bool AsyncTelegram2::checkConnection()
{
    // Start connection with Telegramn server (if necessary)
    if (!telegramClient->connected())
    {

        m_lastmsg_timestamp = millis();
        log_info("Start handshaking...");

        // ESP8266 Soft watch dog reset issue
        #ifdef ESP8266
        ESP.wdtDisable();
        *((volatile uint32_t*) 0x60000900) &= ~(1); // Hardware WDT OFF
        #endif
        if (!telegramClient->connect(TELEGRAM_HOST, TELEGRAM_PORT))
        {
            Serial.println("\n\nUnable to connect to Telegram server");
            reset();
        }
#if DEBUG_ENABLE
        else
        {
	    static uint32_t lastCTime;
            log_debug("Connected using Telegram hostname\n"
                      "Last connection was %d seconds ago\n",
                      (int)(millis() - lastCTime) / 1000);
            lastCTime = millis();
        }
#endif
    }
    #ifdef ESP8266
    ESP.wdtEnable(10000);
    *((volatile uint32_t*) 0x60000900) |= 1; // Hardware WDT ON
    #endif

    return telegramClient->connected();
}

bool AsyncTelegram2::begin()
{
    checkConnection();
    return getMe();
}

bool AsyncTelegram2::reset(void)
{
    static uint32_t lastResetTime;
    if (millis() - lastResetTime > 5000) {
        lastResetTime = millis();
        log_info("Restart Telegram connection\n");
        telegramClient->stop();
        m_lastmsg_timestamp = millis();
        m_waitingReply = false;
    }
    return telegramClient->connected();
}

bool AsyncTelegram2::sendCommand(const char *command, const char *payload, bool blocking)
{
    if (checkConnection())
    {
        String httpBuffer((char *)0);
        httpBuffer.reserve(BUFFER_BIG);
        httpBuffer = "POST https://" TELEGRAM_HOST "/bot";
        httpBuffer += m_token;
        httpBuffer += "/";
        httpBuffer += command;
        // Let's use 1.0 protocol in order to avoid chunked transfer encoding
        httpBuffer += " HTTP/1.0"
                      "\nHost: " TELEGRAM_HOST
                      "\nConnection: keep-alive"
                      "\nContent-Type: application/json";
        httpBuffer += "\nContent-Length: ";
        httpBuffer += strlen(payload);
        httpBuffer += "\n\n";
        httpBuffer += payload;

        #if DEBUG_ENABLE
        if (strcmp(command, "getUpdates") != 0) {
            log_debug("Command %s, payload: %s\n", command, payload);
        }
        #endif

        // Send the whole request in one go is much faster
        telegramClient->print(httpBuffer);

        m_waitingReply = true;
        // Blocking mode
        if (blocking)
        {
            if (!telegramClient->find((char *)HEADERS_END))
            {
                log_error("Invalid HTTP response");
                telegramClient->stop();
                return false;
            }
            // If there are incoming bytes available from the server, read them and print them:
            m_rxbuffer = "";
            while (telegramClient->available())
            {
                yield();
                m_rxbuffer += (char)telegramClient->read();
            }
            m_waitingReply = false;
            if (m_rxbuffer.indexOf("\"ok\":true") > -1)
                return true;
        }
    }

    return false;
}

bool AsyncTelegram2::getUpdates()
{

    // No response from Telegram server for a long time
    if (millis() - m_lastmsg_timestamp > 10 * m_minUpdateTime)
    {
        reset();
    }

    // Send message to Telegram server only if enough time has passed since last
    if (millis() - m_lastUpdateTime > m_minUpdateTime)
    {
        m_lastUpdateTime = millis();

        // If previuos reply from server was received (and parsed)
        if (m_waitingReply == false)
        {
            char payload[BUFFER_SMALL];
            snprintf(payload, BUFFER_SMALL, "{\"limit\":1,\"timeout\":0,\"offset\":%" INT32 "}", m_lastUpdateId);
            sendCommand("getUpdates", payload);
        }
    }

    if (telegramClient->connected() && telegramClient->available())
    {
        // We have a message, parse data received
        bool close_connection = false;
        uint16_t len = 0, pos = 0;
        // Skip headers
        while (telegramClient->connected())
        {
            String line = telegramClient->readStringUntil('\n');
            if (line == "\r")
                break;
            if (line.indexOf("close") > -1)
            {
                close_connection = true;
            }
            if (line.indexOf("Content-Length:") > -1)
            {
                len = line.substring(strlen("Content-Length: ")).toInt();
            }
        }

        // If there are incoming bytes available from the server, read them and store:
        m_rxbuffer = "";
        for (uint32_t timeout = millis(); (millis() - timeout > 1000) || pos < len;)
        {
            if (telegramClient->available())
            {
                m_rxbuffer += (char)telegramClient->read();
                pos++;
            }
        }
        m_waitingReply = false;
        m_lastmsg_timestamp = millis();

        // WiFiNINA error "No socket avalaible"
        // (close the connection before it became inactive from server side)
        #if !defined(ESP32) && !defined(ESP8266)
            static uint32_t closeTime;
            // Telegram server close opened connections more ore less after 255 seconds
            if(millis() - closeTime > 240000) {
                closeTime = millis();
                log_info("Connection closed (WiFiNINA)");
                telegramClient->stop();
            }
        #endif

        if (close_connection)
        {
            telegramClient->stop();
            log_info("Connection closed from server");
        }

        if (m_rxbuffer.indexOf("\"ok\":true") > -1)
        {
            if (m_sentCallback != nullptr && m_waitSent)
            {
                if (m_rxbuffer.indexOf(String(m_lastSentMsgId)) > -1)
                {
                    m_sentCallback(m_waitSent);
                    m_waitSent = false;
                }
            }
            return true;
        }
        else
        {
            log_error(m_rxbuffer.c_str());
            if (m_sentCallback != nullptr && m_waitSent)
            {
                m_waitSent = false;
                m_sentCallback(m_waitSent);
            }
            return false;
        }
    }
    return false;
}

// Parse message received from Telegram server
MessageType AsyncTelegram2::getNewMessage(TBMessage &message)
{
    message.messageType = MessageNoData;

    // Last sent message timeout
    if (millis() - m_lastSentTime > m_sentTimeout && m_waitSent && m_sentCallback != nullptr)
    {
        m_waitSent = false;
        m_sentCallback(m_waitSent);
    }

    // We have a message, parse data received
    if (getUpdates())
    {
        JsonVariant result;
        { // Add as scope in order to destroy updateDoc as soon as possible
            DynamicJsonDocument updateDoc(m_JsonBufferSize);
            DeserializationError err = deserializeJson(updateDoc, m_rxbuffer);
            if (err)
            {
                log_error("deserializeJson() failed\n");
                log_debug("%s", err.c_str());
                log_error();
                log_error(m_rxbuffer);
                // Skip this message id due to the impossibility to parse correctly
                m_lastUpdateId = m_rxbuffer.substring(m_rxbuffer.indexOf(F("\"update_id\":")) + strlen("\"update_id\":")).toInt() + 1;
                // int64_t chat_id = m_rxbuffer.substring( m_rxbuffer.indexOf("{\"id\":") + strlen("{\"id\":")).toInt();
                m_rxbuffer = "";

                // Inform the user about parsing error (blocking)
                sendTo(message.chatId, "[ERROR] - No memory: inrease buffer size with \"setJsonBufferSize(buf_size)\" method");
                return MessageNoData;
            }
            updateDoc.shrinkToFit();
            m_rxbuffer = "";

            if (!updateDoc.containsKey("result"))
            {
                log_error("JSON data not expected");
                serializeJsonPretty(updateDoc, Serial);
                return MessageNoData;
            }

            result = updateDoc["result"];
            if (result.is<JsonArray>())
            {
                result = result[0];
            }
        }

        // This a reply after send message
        if (result["message_id"])
        {
            // m_lastSentMsgId = result["message_id"];
            if (m_sentCallback != nullptr && m_waitSent)
            {
                m_waitSent = false;
                if (result["message_id"] > m_lastSentMsgId  )
                {
                    m_lastSentMsgId = result["message_id"];
                    m_sentCallback(true);
                }
                else {
                    m_sentCallback(false);
                }
            }
        }

        uint32_t updateID = result["update_id"];
        if (updateID)
        {
            m_lastUpdateId = updateID + 1;
        }
        else
        {
            // In case of forwarded message reply, we need to get original text
            // so don't skip parsing the reply to just sent forwarMessage command
            if (!result["forward_from"])
                return MessageNoData;
        }

        debugJson(result, Serial);
        if (result["callback_query"]["id"])
        {
            // this is a callback query
            message.sender.id = result["callback_query"]["from"]["id"];
            message.sender.username = result["callback_query"]["from"]["username"].as<String>();
            message.sender.firstName = result["callback_query"]["from"]["first_name"].as<String>();
            message.sender.lastName = result["callback_query"]["from"]["last_name"].as<String>();

            message.chatId = result["callback_query"]["message"]["chat"]["id"];
            message.messageID = result["callback_query"]["message"]["message_id"];
            message.date = result["callback_query"]["message"]["date"];
            message.chatInstance = result["callback_query"]["chat_instance"];
            message.callbackQueryID = result["callback_query"]["id"];
            message.callbackQueryData = result["callback_query"]["data"].as<String>();
            message.text = result["callback_query"]["message"]["text"].as<String>();
            message.messageType = MessageQuery;

            // Check if callback function is defined for this button query
            for (uint8_t i = 0; i < m_keyboardCount; i++)
                m_keyboards[i]->checkCallback(message);
        }
        else if (result["forward_from"])
        {
            // this is a forwarded message from user or group
            message.sender.id = result["forward_from"]["id"];
            message.sender.username = result["forward_from"]["username"].as<String>();
            message.sender.firstName = result["forward_from"]["first_name"].as<String>();
            message.sender.lastName = result["forward_from"]["last_name"].as<String>();
            message.text = result["text"].as<String>();
            message.messageType = MessageForwarded;
        }
		else if (result["channel_post"])
        {
            // this is a channel message
            message.sender.id = result["channel_post"]["sender_chat"]["id"];
            message.sender.username = result["channel_post"]["sender_chat"]["title"].as<String>();
			message.chatId = result["channel_post"]["chat"]["id"];
            message.text = result["channel_post"]["text"].as<String>();
            message.messageType = MessageText;
        }

        else if (result["message"]["message_id"])
        {

            // this is a message
            message.sender.id = result["message"]["from"]["id"];
            message.sender.username = result["message"]["from"]["username"].as<String>();
            message.sender.firstName = result["message"]["from"]["first_name"].as<String>();
            message.sender.lastName = result["message"]["from"]["last_name"].as<String>();

            message.messageID = result["message"]["message_id"];
            message.chatId = result["message"]["chat"]["id"];
            message.date = result["message"]["date"];

            if (result["message"]["location"])
            {
                // this is a location message
                message.location.longitude = result["message"]["location"]["longitude"];
                message.location.latitude = result["message"]["location"]["latitude"];
                message.messageType = MessageLocation;
            }
            else if (result["message"]["contact"])
            {
                // this is a contact message
                message.contact.id = result["message"]["contact"]["user_id"];
                message.contact.firstName = result["message"]["contact"]["first_name"].as<String>();
                message.contact.lastName = result["message"]["contact"]["last_name"].as<String>();
                message.contact.phoneNumber = result["message"]["contact"]["phone_number"].as<String>();
                message.contact.vCard = result["message"]["contact"]["vcard"].as<String>();
                message.messageType = MessageContact;
            }
            else if (result["message"]["new_chat_member"])
            {
                // this is a add member message
                message.member.isBot = result["message"]["new_chat_member"]["is_bot"];
                message.member.id = result["message"]["new_chat_member"]["id"];
                message.member.firstName = result["message"]["new_chat_member"]["first_name"].as<String>();
                message.member.lastName = result["message"]["new_chat_member"]["last_name"].as<String>();
                message.member.username = result["message"]["new_chat_member"]["username"].as<String>();
                message.messageType = MessageNewMember;
            }
            else if (result["message"]["left_chat_member"])
            {
                // this is a left member message
                message.member.isBot = result["message"]["new_chat_member"]["is_bot"];
                message.member.id = result["message"]["new_chat_member"]["id"];
                message.member.firstName = result["message"]["new_chat_member"]["first_name"].as<String>();
                message.member.lastName = result["message"]["new_chat_member"]["last_name"].as<String>();
                message.member.username = result["message"]["new_chat_member"]["username"].as<String>();
                message.messageType = MessageLeftMember;
            }
            else if (result["message"]["document"])
            {
                // this is a document message
                message.document.file_id = result["message"]["document"]["file_id"].as<String>();
                message.document.file_name = result["message"]["document"]["file_name"].as<String>();
                message.text = result["message"]["caption"].as<String>();
                message.document.file_exists = getFile(message.document);
                message.messageType = MessageDocument;
            }
            else if (result["message"]["reply_to_message"])
            {
                // this is a reply to message
                message.text = result["message"]["text"].as<String>();
                message.messageType = MessageReply;
            }
            else if (result["message"]["text"])
            {
                // this is a text message
                message.text = result["message"]["text"].as<String>();
                message.messageType = MessageText;
            }
        }
        // m_lastSentMsgId = message.messageID;
        return message.messageType;
    }
    return MessageNoData; // waiting for reply from server
}


// Blocking getMe function (we wait for a reply from Telegram server)
bool AsyncTelegram2::getMe()
{
    // getMe has to be blocking (wait server reply)
    if (!sendCommand("getMe", "", true))
    {
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
    snprintf(cmd, BUFFER_SMALL, "getFile?file_id=%s", doc.file_id.c_str());

    // getFile has to be blocking (wait server reply
    if (!sendCommand(cmd, "", true))
    {
        log_error("getFile error");
        return false;
    }
    StaticJsonDocument<BUFFER_MEDIUM> fileDoc;
    deserializeJson(fileDoc, m_rxbuffer);
    debugJson(fileDoc, Serial);
    doc.file_path = "https://api.telegram.org/file/bot";
    doc.file_path += m_token;
    doc.file_path += "/";
    doc.file_path += fileDoc["result"]["file_path"].as<String>();
    doc.file_size = fileDoc["result"]["file_size"].as<long>();
    return true;
}

bool AsyncTelegram2::noNewMessage()
{

    TBMessage msg;
    this->reset();
    this->getNewMessage(msg);
    while (!this->getUpdates())
    {
        delay(100);
    }
    return true;
}

bool AsyncTelegram2::sendMessage(const TBMessage &msg, const char *message, char *keyboard, bool wait)
{

    if (!strlen(message))
        return false;
    m_waitSent = true;
    m_lastSentTime = millis();

    DynamicJsonDocument root(m_JsonBufferSize);
    root["chat_id"] = msg.chatId;
    root["text"] = message;

    switch (m_formatType)
    {
    case FormatStyle::NONE:
        break;
    case FormatStyle::HTML:
        root["parse_mode"] = "HTML";
        break;
    case FormatStyle::MARKDOWN:
        root["parse_mode"] = "MarkdownV2";
        break;
    }

    if (msg.disable_notification)
        root["disable_notification"] = true;

    if (keyboard != nullptr)
    {
        size_t len = strlen(keyboard);
        if (len || msg.force_reply)
        {
            DynamicJsonDocument doc(len*2);
            DeserializationError err = deserializeJson(doc, keyboard);
            if (err)
            {
                log_debug("deserializeJson() failed: %s\n", err.c_str());
            }
            JsonObject myKeyb = doc.as<JsonObject>();
            root["reply_markup"] = myKeyb;
            if (msg.force_reply)
            {
                root["reply_markup"]["selective"] = true;
                root["reply_markup"]["force_reply"] = true;
            }
        }
    }
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("sendMessage", payload.c_str(),  wait);
}

bool AsyncTelegram2::forwardMessage(const TBMessage &msg, const int64_t to_chatid)
{
    DynamicJsonDocument root(BUFFER_SMALL);
    root["chat_id"] = to_chatid;
    root["from_chat_id"] = msg.chatId;
    root["message_id"] = msg.messageID;
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("forwardMessage", payload.c_str());
}

bool AsyncTelegram2::sendPhotoByUrl(const int64_t &chat_id, const char *url, const char *caption)
{
    if (!strlen(url))
        return false;

    DynamicJsonDocument root(BUFFER_SMALL);
    root["chat_id"] = chat_id;
    root["photo"] = url;
    root["caption"] = caption;
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("sendPhoto", payload.c_str());
}

bool AsyncTelegram2::sendAnimationByUrl(const int64_t &chat_id, const char *url, const char *caption)
{
    if (!strlen(url))
        return false;

    DynamicJsonDocument root(BUFFER_SMALL);
    root["chat_id"] = chat_id;
    root["video"] = url;
    root["caption"] = caption;
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("sendVideo", payload.c_str());
}

bool AsyncTelegram2::sendToChannel(const char *channel, const char *message, bool silent)
{
    if (!strlen(message))
        return false;

    DynamicJsonDocument root(BUFFER_BIG);
    root["chat_id"] = channel;
    root["text"] = message;
    root["silent"] = silent ? "true" : "false";

    switch (m_formatType)
    {
    case FormatStyle::NONE:
        break;
    case FormatStyle::HTML:
        root["parse_mode"] = "HTML";
        break;
    case FormatStyle::MARKDOWN:
        root["parse_mode"] = "MarkdownV2";
        break;
    }
    String payload;
    serializeJson(root, payload);
    return sendCommand("sendMessage", payload.c_str());
}

bool AsyncTelegram2::endQuery(const TBMessage &msg, const char *message, bool alertMode)
{
    if (!msg.callbackQueryID)
        return false;

    DynamicJsonDocument root(m_JsonBufferSize);
    root["callback_query_id"] = msg.callbackQueryID;
    root["text"] = message;
    root["cache_time"] = 2;
    root["show_alert"] = alertMode ? "true" : "false";
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("answerCallbackQuery", payload.c_str(), true);
}

bool AsyncTelegram2::removeReplyKeyboard(const TBMessage &msg, const char *message, bool selective)
{
    DynamicJsonDocument root(BUFFER_SMALL);
    root["remove_keyboard"] = true;
    root["selective"] = selective ? true : false;
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendMessage(msg, message, payload);
}

// enum DocumentType { DOCUMENT, PHOTO, ANIMATION, AUDIO, VOICE, VIDEO};
bool AsyncTelegram2::sendDocument(int64_t chat_id, Stream &stream, size_t size,
                                    DocumentType doc, const char *filename, const char *caption)
{
    switch (doc)
    {
    case JSON:
        return sendStream(chat_id, "sendDocument", "application/json", "document", stream, size, filename, caption);
    case CSV:
        return sendStream(chat_id, "sendDocument", "text/csv", "document", stream, size, filename, caption);
    case ZIP:
        return sendStream(chat_id, "sendDocument", "application/zip", "document", stream, size, filename, caption);
    case PDF:
        return sendStream(chat_id, "sendDocument", "application/pdf", "document", stream, size, filename, caption);
    case PHOTO:
        return sendStream(chat_id, "sendPhoto", "image/jpeg", "photo", stream, size, filename, caption);
    case AUDIO:
        return sendStream(chat_id, "sendDocument", "audio/mp3", "audio", stream, size, filename, caption);
    default:
        return sendStream(chat_id, "sendDocument", "text/plain", "document", stream, size, filename, caption);
    }

    return false;
}

void AsyncTelegram2::setformData(int64_t chat_id, const char *cmd, const char *type,
                                 const char *propName, size_t size, String &formData,
                                 String &request,const char *filename, const char *caption)
{

#define BOUNDARY "----WebKitFormBoundary7MA4YWxkTrZu0gW"
#define END_BOUNDARY "\r\n--" BOUNDARY "--\r\n"

    char int64_buf[22] = {0};
    snprintf(int64_buf, sizeof(int64_buf), "%lld", chat_id);

    formData = "--" BOUNDARY "\r\nContent-disposition: form-data; name=\"chat_id\"\r\n\r\n";
    formData += int64_buf;

    if (caption != nullptr)
    {
        formData += "\r\n--" BOUNDARY "\r\nContent-disposition: form-data; name=\"caption\"\r\n\r\n";
        formData += caption;
    }

    formData += "\r\n--" BOUNDARY "\r\nContent-disposition: form-data; name=\"";
    formData += propName;
    formData += "\"; filename=\"";
    formData += filename != nullptr ? filename : "image.jpg";
    formData += "\"\r\nContent-Type: ";
    formData += type;
    formData += "\"\r\n\r\n";

    request = "POST /bot";
    request += m_token;
    request += "/";
    request += cmd;
    request +=  " HTTP/1.0"
                "\r\nConnection: keep-alive"
                "\r\nHost: " TELEGRAM_HOST
                "\r\nContent-Length: ";
    request += (size + formData.length() + strlen(END_BOUNDARY));
    request += "\r\nContent-Type: multipart/form-data; boundary=" BOUNDARY "\r\n";
}

bool AsyncTelegram2::sendStream(int64_t chat_id, const char *cmd, const char *type, const char *propName,
                                    Stream &stream, size_t size, const char *filename,const char *caption)
{
    bool res = false;
    if (checkConnection())
    {
        m_waitingReply = true;
        String formData;
        formData.reserve(512);
        String request;
        request.reserve(256);
        setformData(chat_id, cmd, type, propName, size, formData, request, filename, caption);

#if DEBUG_ENABLE
        uint32_t t1 = millis();
        Serial.println(request);
        Serial.println(formData);
#endif
        // Send POST request to host
        telegramClient->println(request);
        // Body of request
        telegramClient->print(formData);

        uint8_t data[BLOCK_SIZE];
        int n_block = trunc(size / BLOCK_SIZE);
        int lastBytes = size - (n_block * BLOCK_SIZE);

        for (uint16_t pos = 0; pos < n_block; pos++)
        {
            stream.readBytes(data, BLOCK_SIZE);
            telegramClient->write(data, BLOCK_SIZE);
            yield();
        }
        stream.readBytes(data, lastBytes);
        telegramClient->write(data, lastBytes);

        // Close the request form-data
        telegramClient->println(END_BOUNDARY);
        // telegramClient->flush();
        m_waitSent = true;
        m_lastSentTime = millis();

#if DEBUG_ENABLE
        log_debug("Raw upload time: %lums\n", millis() - t1);
        t1 = millis();
#endif
        // Handle reply with getUpdates() method
    }
    else
        Serial.println("\nError: client not connected");
    return res;
}

bool AsyncTelegram2::sendBuffer(int64_t chat_id, const char *cmd, const char *type, const char *propName, uint8_t *data, size_t size, const char *caption)
{
    bool res = false;
    if (checkConnection())
    {
        m_waitingReply = true;
        String formData;
        formData.reserve(512);
        String request;
        request.reserve(256);
        setformData(chat_id, cmd, type, propName, size, formData, request, caption, caption);

#if DEBUG_ENABLE
        uint32_t t1 = millis();
#endif
        // Send POST request to host
        telegramClient->println(request);
        // Body of request
        telegramClient->print(formData);

        // Serial.println(telegramClient->write((const uint8_t *) data, size));
        uint16_t pos = 0;
        int n_block = trunc(size / BLOCK_SIZE);
        int lastBytes = size - (n_block * BLOCK_SIZE);

        for (pos = 0; pos < n_block; pos++)
        {
            telegramClient->write((const uint8_t *)data + pos * BLOCK_SIZE, BLOCK_SIZE);
            yield();
        }
        telegramClient->write((const uint8_t *)data + pos * BLOCK_SIZE, lastBytes);

        // Close the request form-data
        telegramClient->println(END_BOUNDARY);
        // telegramClient->flush();
        m_waitSent = true;
        m_lastSentTime = millis();

#if DEBUG_ENABLE
        log_debug("Raw upload time: %lums\n", millis() - t1);
        t1 = millis();
#endif

    }
    else
        Serial.println("\nError: client not connected");
    return res;
}

void AsyncTelegram2::getMyCommands(String &cmdList)
{
    if (!sendCommand("getMyCommands", "", true))
    {
        log_error("getMyCommands error ");
        return;
    }
    StaticJsonDocument<BUFFER_MEDIUM> doc;
    DeserializationError err = deserializeJson(doc, m_rxbuffer);
    if (err)
    {
        return;
    }
    debugJson(doc, Serial);
    // cmdList = doc["result"].as<String>();
    serializeJsonPretty(doc["result"], cmdList);
}

bool AsyncTelegram2::deleteMyCommands()
{
    if (!sendCommand("deleteMyCommands", "", true))
    {
        log_error("getMyCommands error ");
        return "";
    }
    return true;
}

bool AsyncTelegram2::setMyCommands(const String &cmd, const String &desc)
{
    // get actual list of commands
    if (!sendCommand("getMyCommands", "", true))
    {
        log_error("getMyCommands error ");
        return "";
    }
    DynamicJsonDocument doc(BUFFER_MEDIUM);
    DeserializationError err = deserializeJson(doc, m_rxbuffer);
    if (err)
    {
        return false;
    }

    // Check if command already present in list
    for (JsonObject key : doc["result"].as<JsonArray>())
    {
        if (key["command"] == cmd)
        {
            return false;
        }
    }

    StaticJsonDocument<256> obj;
    obj["command"] = cmd;
    obj["description"] = desc;
    doc["result"].as<JsonArray>().add(obj);

    StaticJsonDocument<BUFFER_MEDIUM> doc2;
    doc2["commands"] = doc["result"].as<JsonArray>();

    String payload;
    serializeJson(doc2, payload);
    return sendCommand("setMyCommands", payload.c_str(), true);
}

bool AsyncTelegram2::editMessage(int64_t chat_id, int32_t message_id, const String &txt, const String &keyboard)
{
    DynamicJsonDocument root(m_JsonBufferSize);
    root["chat_id"] = chat_id;
    root["message_id"] = message_id;
    root["text"] = txt;
    if (keyboard.length()) {
        root["reply_markup"] = keyboard;
    }
    root.shrinkToFit();
    String payload;
    serializeJson(root, payload);
    return sendCommand("editMessageText", payload.c_str());
}
