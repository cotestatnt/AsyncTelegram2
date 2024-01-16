
#ifndef REPLY_KEYBOARD
#define REPLY_KEYBOARD

#define ARDUINOJSON_USE_LONG_LONG 	1
#define ARDUINOJSON_DECODE_UNICODE  1
#include <ArduinoJson.h>
#include "DataStructures.h"
#include "serial_log.h"

#if ARDUINOJSON_VERSION_MAJOR > 6
    #define JSON_DOC(x) JsonDocument root
#else
    #define JSON_DOC(x) DynamicJsonDocument root((size_t)x)
#endif

enum ReplyKeyboardButtonType {
  KeyboardButtonSimple   = 1,
  KeyboardButtonContact  = 2,
  KeyboardButtonLocation = 3
};


class ReplyKeyboard
{
private:
  String m_json;
  size_t m_jsonSize;

public:
  ReplyKeyboard(size_t size = BUFFER_SMALL);
  ~ReplyKeyboard();

  // add a new empty row of buttons
  // return:
  //    true if no error occurred
  bool addRow(void);

  // add a button in the current row
  // params:
  //   text      : the text displayed as button label
  //   buttonType: the type of the button (simple text, contact request, location request)
  // return:
  //    true if no error occurred
  bool addButton(const char* text, ReplyKeyboardButtonType buttonType = KeyboardButtonSimple);

  // enable reply keyboard autoresizing (default: the same size of the standard keyboard)
  void enableResize(void);

  // hide the reply keyboard as soon as it's been used
  void enableOneTime(void);

  // Use this parameter if you want to show the keyboard for specific users only.
    // Targets: 1) users that are @mentioned in the text of the Message object;
  //          2) if the bot's message is a reply (has reply_to_message_id), sender of the original message
  void enableSelective(void);

  // generate a string that contains the inline keyboard formatted in a JSON structure.
  // returns:
  //   the JSON of the inline keyboard
  String getJSON(void);
  String getJSONPretty();


  inline void clear() {
    m_json = "{\"keyboard\":[[]]}\"";
  }
};

#endif
