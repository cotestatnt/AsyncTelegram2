
#ifndef INLINE_KEYBOARD
#define INLINE_KEYBOARD

#define ARDUINOJSON_USE_LONG_LONG 	1
#define ARDUINOJSON_DECODE_UNICODE  1
#include <ArduinoJson.h>
// #include <functional>
#include "DataStructures.h"

enum InlineKeyboardButtonType {
  KeyboardButtonURL    = 1,
  KeyboardButtonQuery  = 2
};


class InlineKeyboard
{
 typedef void(*CallbackType)(const TBMessage &msg);
//using CallbackType = std::function<void(const TBMessage &msg)>;

struct InlineButton{
  char 		*btnName;
  CallbackType argCallback;
  InlineButton *nextButton;
} ;

public:
  InlineKeyboard();
  InlineKeyboard(const String& keyboard);
  ~InlineKeyboard();

  // Get total number of keyboard buttons
  int getButtonsNumber() ;

  // add a new empty row of buttons
  // return:
  //    true if no error occurred
  bool addRow(void);

  // add a button in the current row
  // params:
  //   text   : the text displayed as button label
  //   command: URL (if buttonType is CTBotKeyboardButtonURL)
  //            callback query data (if buttonType is CTBotKeyboardButtonQuery)
  // return:
  //    true if no error occurred
  bool addButton(const char* text, const char* command, InlineKeyboardButtonType buttonType, CallbackType onClick = nullptr);

  // generate a string that contains the inline keyboard formatted in a JSON structure.
  // Useful for CTBot::sendMessage()
  // returns:
  //   the JSON of the inline keyboard
  String getJSON(void) const ;
  String getJSONPretty(void) const;

  inline void clear() {
    m_json = "{\"inline_keyboard\":[[]]}\"";
  }

private:
  friend class AsyncTelegram2;
  String 			m_json;
  String 			m_name;

  uint8_t			m_buttonsCounter = 0;
  InlineButton 	*_firstButton = nullptr;
  InlineButton 	*_lastButton = nullptr;

  // Check if a callback function has to be called for a button query reply message
  void checkCallback(const TBMessage &msg) ;

};



#endif
