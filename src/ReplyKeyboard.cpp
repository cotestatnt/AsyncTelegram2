#include "ReplyKeyboard.h"

ReplyKeyboard::ReplyKeyboard(size_t size)
{
  m_jsonSize = size;
  m_json.reserve(m_jsonSize);
  m_json = "{\"keyboard\":[[]]}\"";
}

ReplyKeyboard::~ReplyKeyboard() {
  m_json = "";
}

bool ReplyKeyboard::addRow()
{
  JSON_DOC(m_jsonSize);

  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
    return false;
  }

#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonArray rows = root["keyboard"].as<JsonArray>();
  rows.add<JsonVariant>();
#else
  JsonArray rows = root["keyboard"];
  rows.createNestedArray();
#endif
  m_json = "";
  serializeJson(root, m_json);
  return true;
}
bool ReplyKeyboard::addButton(const char *text, ReplyKeyboardButtonType buttonType)
{
  if ((buttonType != KeyboardButtonContact) &&
      (buttonType != KeyboardButtonLocation) &&
      (buttonType != KeyboardButtonSimple))
  {
    Serial.println("no button type");
    return false;
  }

  JSON_DOC(m_jsonSize);

  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
    return false;
  }

#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonArray rows = root["keyboard"].as<JsonArray>();
  JsonObject button = rows[rows.size()-1].add<JsonObject>();
#else
  JsonArray rows = root["keyboard"];
  JsonObject button = rows[rows.size() - 1].createNestedObject();
#endif

  button["text"] = text;
  switch (buttonType)
  {
  case KeyboardButtonContact:
    button["request_contact"] = true;
    break;
  case KeyboardButtonLocation:
    button["request_location"] = true;
    break;
  default:
    break;
  }

  // Store inline keyboard json structure
  m_json = "";
  serializeJson(root, m_json);
  return true;
}

void ReplyKeyboard::enableResize()
{

  JSON_DOC(m_jsonSize);
  deserializeJson(root, m_json);
  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
  }
  m_json = "";
  root["resize_keyboard"] = true;
  serializeJson(root, m_json);
}

void ReplyKeyboard::enableOneTime()
{
  JSON_DOC(m_jsonSize);
  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
  }
  m_json = "";
  root["one_time_keyboard"] = true;
  serializeJson(root, m_json);
}

void ReplyKeyboard::enableSelective()
{
  JSON_DOC(m_jsonSize);
  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
  }
  m_json = "";
  root["selective"] = true;
  serializeJson(root, m_json);
}

String ReplyKeyboard::getJSON()
{
  return m_json;
}

String ReplyKeyboard::getJSONPretty()
{
  JSON_DOC(m_jsonSize);
  DeserializationError err = deserializeJson(root, m_json);
  if (err)
  {
    log_debug("deserializeJson() failed: %s\n", err.c_str());
  }

  String serialized;
  serializeJsonPretty(root, serialized);
  return serialized;
}
