#include "InlineKeyboard.h"


InlineKeyboard::InlineKeyboard()
{
  m_json = "{\"inline_keyboard\":[[]]}\"";
}

InlineKeyboard::~InlineKeyboard(){}

bool InlineKeyboard::addRow()
{
  DynamicJsonDocument doc(BUFFER_BIG+BUFFER_MEDIUM);
  deserializeJson(doc, m_json);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, m_json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonArray  rows = doc["inline_keyboard"];
  rows.createNestedArray();
  m_json = "";
  doc.shrinkToFit();
  serializeJson(doc, m_json);

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

  DynamicJsonDocument doc(BUFFER_BIG+BUFFER_MEDIUM);
  DeserializationError error = deserializeJson(doc, m_json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonArray  rows = doc["inline_keyboard"];
  JsonObject button = rows[rows.size()-1].createNestedObject();

  button["text"] = text ;
  if(KeyboardButtonURL == buttonType)
    button["url"] = command;
  else if (KeyboardButtonQuery == buttonType)
    button["callback_data"] = command;

  // Store inline keyboard json structure
  m_json = "";
  doc.shrinkToFit();
  serializeJson(doc, m_json);
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
  StaticJsonDocument<BUFFER_MEDIUM> doc;
  deserializeJson(doc, m_json);

  String serialized;
  serializeJsonPretty(doc, serialized);
  return serialized;
}

