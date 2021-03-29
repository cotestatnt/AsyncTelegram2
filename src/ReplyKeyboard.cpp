#include "ReplyKeyboard.h"

ReplyKeyboard::ReplyKeyboard()
{
  m_json = "{\"keyboard\":[[]]}\"";
}

ReplyKeyboard::~ReplyKeyboard() {}


bool ReplyKeyboard::addRow()
{
  if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(m_jsonSize + 128);	 // Current size + space for new row (empty)

  deserializeJson(doc, m_json);
  JsonArray rows = doc["keyboard"];
  rows.createNestedArray();
  m_json = "";
  serializeJson(doc, m_json);
  doc.shrinkToFit();
  m_jsonSize = doc.memoryUsage();
  return true;
}


bool ReplyKeyboard::addButton(const char* text, ReplyKeyboardButtonType buttonType)
{
  if ((buttonType != KeyboardButtonContact) &&
    (buttonType != KeyboardButtonLocation) &&
    (buttonType != KeyboardButtonSimple))
    return false;
  // As reccomended use local JsonDocument instead global
  // inline keyboard json structure will be stored in a String var
  if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(m_jsonSize + 256);	 // Current size + space for new object (button)
  deserializeJson(doc, m_json);

  JsonArray  rows = doc["keyboard"];
  JsonObject button = rows[rows.size()-1].createNestedObject();

  button["text"] = text;
  switch (buttonType){
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
  serializeJson(doc, m_json);
  doc.shrinkToFit();
  m_jsonSize = doc.memoryUsage();
  return true;
}


void ReplyKeyboard::enableResize()
{
  if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(m_jsonSize + 128);   // Current size + space for new field
  deserializeJson(doc, m_json);
  doc["resize_keyboard"] = true;
  m_json = "";
  serializeJson(doc, m_json);
}

void ReplyKeyboard::enableOneTime()
{
  if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(m_jsonSize + 128);	// Current size + space for new field
  deserializeJson(doc, m_json);
  doc["one_time_keyboard"] = true;
  m_json = "";
  serializeJson(doc, m_json);
}

void ReplyKeyboard::enableSelective()
{
  if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(m_jsonSize + 128);  // Current size + space for new field
  deserializeJson(doc, m_json);
  doc["selective"] = true;
  m_json = "";
  serializeJson(doc, m_json);
}

String ReplyKeyboard::getJSON() const
{
  return m_json;
}

String ReplyKeyboard::getJSONPretty() const
{
  uint16_t jsonSize;
  if(m_jsonSize < BUFFER_MEDIUM) jsonSize = BUFFER_MEDIUM;
  DynamicJsonDocument doc(jsonSize + 128);	// Current size + space for new lines
  deserializeJson(doc, m_json);

  String serialized;
  serializeJsonPretty(doc, serialized);
  return serialized;
}


