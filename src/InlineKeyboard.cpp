#include "InlineKeyboard.h"


InlineKeyboard::InlineKeyboard(size_t size)
{
  m_jsonSize = size;
  m_json.reserve(m_jsonSize);
  m_json = "{\"inline_keyboard\":[[]]}\"";
}

InlineKeyboard::InlineKeyboard(const String& keyboard, size_t size){
  m_jsonSize = size;
  m_json.reserve(m_jsonSize);
  m_json = keyboard;
}

InlineKeyboard::~InlineKeyboard(){
  m_json = "";
}

bool InlineKeyboard::addRow()
{
  JSON_DOC(m_jsonSize);
  deserializeJson(root, m_json);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(root, m_json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonArray rows = root["inline_keyboard"].as<JsonArray>();
  rows.add<JsonVariant>();
#else
  JsonArray rows = root["inline_keyboard"];
  rows.createNestedArray();
#endif 
  m_json = "";
  serializeJson(root, m_json);
  return true;
}

bool InlineKeyboard::addButton(const char* text, const char* command, InlineKeyboardButtonType buttonType, CallbackType onClick)
{
  if ((buttonType != KeyboardButtonURL) && (buttonType != KeyboardButtonQuery))
    return false;

  InlineButton *inlineButton = new InlineButton();
  if (_firstButton == nullptr)
    _firstButton = inlineButton;
  else
    _lastButton->nextButton = inlineButton;
  inlineButton->argCallback = onClick;
  inlineButton->btnName = (char*)command;
  _lastButton = inlineButton;
  m_buttonsCounter++;

  JSON_DOC(m_jsonSize);
  DeserializationError error = deserializeJson(root, m_json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

#if ARDUINOJSON_VERSION_MAJOR > 6
  JsonArray rows = root["inline_keyboard"].as<JsonArray>();
  JsonObject button = rows[rows.size()-1].add<JsonObject>();
#else
  JsonArray rows = root["inline_keyboard"];
  JsonObject button = rows[rows.size()-1].createNestedObject();
#endif

  button["text"] = text ;
  if(KeyboardButtonURL == buttonType)
    button["url"] = command;
  else if (KeyboardButtonQuery == buttonType)
    button["callback_data"] = command;

  // Store inline keyboard json structure
  m_json = "";
  serializeJson(root, m_json);
  return true;
}

// Check if a callback function has to be called for this button query message
void InlineKeyboard::checkCallback( const TBMessage &msg)  {
  for(InlineButton *_button = _firstButton; _button != nullptr; _button = _button->nextButton){
    if( msg.callbackQueryData.equals(_button->btnName) && _button->argCallback != nullptr){
      _button->argCallback(msg);
    }
  }
}

// Get total number of keyboard buttons
int InlineKeyboard::getButtonsNumber()
{
  return m_buttonsCounter;
}

String InlineKeyboard::getJSON() const
{
  return m_json;
}

String InlineKeyboard::getJSONPretty() const
{
  JSON_DOC(m_jsonSize);
  deserializeJson(root, m_json);

  String serialized;
  serializeJsonPretty(root, serialized);
  return serialized;
}

