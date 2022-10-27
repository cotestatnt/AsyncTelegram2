#include "ReplyKeyboard.h"

ReplyKeyboard::ReplyKeyboard()
{
  m_json.reserve(BUFFER_MEDIUM);
  m_json = "{\"keyboard\":[[]]}\"";
}

ReplyKeyboard::~ReplyKeyboard() {}

bool ReplyKeyboard::addRow()
{
#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif

  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
    return false;
  }

  JsonArray rows = doc["keyboard"];
  rows.createNestedArray();
  m_json = "";
  serializeJson(doc, m_json);
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

#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif

  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
    return false;
  }

  JsonArray rows = doc["keyboard"];
  JsonObject button = rows[rows.size() - 1].createNestedObject();

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
  serializeJson(doc, m_json);
  return true;
}

// bool ReplyKeyboard::addRow()
// {
//   if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
//   DynamicJsonDocument doc(m_jsonSize + 128);	 // Current size + space for new row (empty)

//   deserializeJson(doc, m_json);
//   JsonArray rows = doc["keyboard"];
//   rows.createNestedArray();
//   m_json = "";
//   serializeJson(doc, m_json);
//   doc.shrinkToFit();
//   m_jsonSize = doc.memoryUsage();
//   return true;
// }

// bool ReplyKeyboard::addButton(const char* text, ReplyKeyboardButtonType buttonType)
// {
//   if ((buttonType != KeyboardButtonContact) &&
//     (buttonType != KeyboardButtonLocation) &&
//     (buttonType != KeyboardButtonSimple))
//     return false;
//   // As reccomended use local JsonDocument instead global
//   // inline keyboard json structure will be stored in a String var
//   if(m_jsonSize < BUFFER_MEDIUM) m_jsonSize = BUFFER_MEDIUM;
//   DynamicJsonDocument doc(m_jsonSize + 256);	 // Current size + space for new object (button)
//   deserializeJson(doc, m_json);

//   JsonArray  rows = doc["keyboard"];
//   JsonObject button = rows[rows.size()-1].createNestedObject();

//   button["text"] = text;
//   switch (buttonType){
//     case KeyboardButtonContact:
//       button["request_contact"] = true;
//       break;
//     case KeyboardButtonLocation:
//       button["request_location"] = true;
//       break;
//     default:
//       break;
//   }

//   // Store inline keyboard json structure
//   m_json = "";
//   serializeJson(doc, m_json);
//   doc.shrinkToFit();
//   m_jsonSize = doc.memoryUsage();
//   return true;
// }

void ReplyKeyboard::enableResize()
{
  // if (m_jsonSize < BUFFER_MEDIUM)
  //   m_jsonSize = BUFFER_MEDIUM;
  // DynamicJsonDocument doc(m_jsonSize + 128); // Current size + space for new field
  // deserializeJson(doc, m_json);
  // doc["resize_keyboard"] = true;
  // m_json = "";
  // serializeJson(doc, m_json);


#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif
  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
  }

  doc["resize_keyboard"] = true;
  m_json = "";
  serializeJson(doc, m_json);

  Serial.println(m_json);
}

void ReplyKeyboard::enableOneTime()
{
  // if (m_jsonSize < BUFFER_MEDIUM)
  //   m_jsonSize = BUFFER_MEDIUM;
  // DynamicJsonDocument doc(m_jsonSize + 128); // Current size + space for new field
  // deserializeJson(doc, m_json);
  // doc["one_time_keyboard"] = true;
  // m_json = "";
  // serializeJson(doc, m_json);


#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif
  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
  }

  doc["one_time_keyboard"] = true;
  m_json = "";
  serializeJson(doc, m_json);
}

void ReplyKeyboard::enableSelective()
{
  // if (m_jsonSize < BUFFER_MEDIUM)
  //   m_jsonSize = BUFFER_MEDIUM;
  // DynamicJsonDocument doc(m_jsonSize + 128); // Current size + space for new field
  // deserializeJson(doc, m_json);
  // doc["selective"] = true;
  // m_json = "";
  // serializeJson(doc, m_json);


#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif
  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
  }

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
  // uint16_t jsonSize;
  // if (m_jsonSize < BUFFER_MEDIUM)
  //   jsonSize = BUFFER_MEDIUM;
  // DynamicJsonDocument doc(jsonSize + 128); // Current size + space for new lines
  // deserializeJson(doc, m_json);

  // String serialized;
  // serializeJsonPretty(doc, serialized);
  // return serialized;

#if defined(ESP8266)
  DynamicJsonDocument doc(ESP.getMaxFreeBlockSize() - BUFFER_MEDIUM);
#elif defined(ESP32)
  DynamicJsonDocument doc(ESP.getMaxAllocHeap() - BUFFER_BIG);
#else
  DynamicJsonDocument doc(BUFFER_BIG);
#endif
  DeserializationError err = deserializeJson(doc, m_json);
  if (err)
  {
    log_error("deserializeJson() failed: %s\n", err.c_str());
  }

  String serialized;
  serializeJsonPretty(doc, serialized);
  return serialized;
}
