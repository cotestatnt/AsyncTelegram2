#include "AsyncTelegram2.h"
extern void saveConfigFile();
extern void printHeapStats();

#define ALL_DAYS    "all_day"
#define WORK_DAYS   "work_day"
#define WEEKEND     "weekend_only"
#define SINGLE_DAY  "single_day"

#define MONDAY      "Monday"
#define TUESDAY     "Tuesday"
#define WEDNESDAY   "Wednesday"
#define THURSDAY    "Thursday"
#define FRIDAY      "Friday"
#define SATURDAY    "Saturday"
#define SUNDAY      "Sunday"

const char* dayNames[] PROGMEM = {MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY } ;
enum WaitState {IDLE, EDIT_EVENT, GET_DAY, GET_START, GET_STOP};

#define MAX_MSG_LEN 128
#define MAX_EVENTS  20

char userInput[20];
int inputIndex = 0;
bool awaitUserInput = false;

// When user input is required, instead of sending a new message with keyboard every time,
// let's edit the last inline keyboard sent
bool inlineKeyboardSent = false;

typedef struct  {
  uint32_t start;
  uint32_t stop;
  uint8_t days;
  bool active;
} Event_t;

Event_t events[MAX_EVENTS];
Event_t theEvent;
uint8_t lastEvent = 0;
uint8_t eventIndex = 0;

char replyBuffer[MAX_MSG_LEN];

// Add or edit an event
int addEvent(Event_t event, uint8_t index) {
  if (index == MAX_EVENTS) {
    events[lastEvent++] = event;
    index = lastEvent - 1;
  }
  else if (index <= lastEvent) {
    events[index] = event;
  }
  saveConfigFile();
  eventIndex = MAX_EVENTS;
  theEvent.days = 0;
  inlineKeyboardSent = false;
  return index;
}

void getSchedulerKeyboard(InlineKeyboard &kbd) {
  kbd.addButton("Single", SINGLE_DAY, KeyboardButtonQuery);
  kbd.addButton("All days", ALL_DAYS, KeyboardButtonQuery);
  kbd.addButton("Work days", WORK_DAYS, KeyboardButtonQuery);
  kbd.addButton("Weekend", WEEKEND, KeyboardButtonQuery);
}

void getWeekdayKeyboard(InlineKeyboard &kbd) {
  kbd.addButton(MONDAY, MONDAY, KeyboardButtonQuery );
  kbd.addButton(TUESDAY, TUESDAY, KeyboardButtonQuery);
  kbd.addButton(WEDNESDAY, WEDNESDAY, KeyboardButtonQuery);
  kbd.addRow();
  kbd.addButton(THURSDAY, THURSDAY, KeyboardButtonQuery);
  kbd.addButton(FRIDAY, FRIDAY, KeyboardButtonQuery);
  kbd.addButton(SATURDAY, SATURDAY, KeyboardButtonQuery);
  kbd.addRow();
  kbd.addButton(SUNDAY, SUNDAY, KeyboardButtonQuery);
  kbd.addButton("CANCEL", "CANCEL", KeyboardButtonQuery);
}

void listScheduledEvents(TBMessage &theMsg, AsyncTelegram2 &theBot, int event = 0) {
  int from  = 0;
  int to = lastEvent;

  if(event != 0) {
    from = event;
    to = event + 1;
  }

  if (lastEvent != 0) {
    for (uint8_t i = from; i < to; i++) {
      Event_t event = events[i];

      snprintf(replyBuffer, MAX_MSG_LEN, "Timer n° %d\nActive from %02d:%02d to %02d:%02d\n",
               i + 1, (int)event.start/3600, (int)(event.start % 3600)/60,
               (int)event.stop/3600, (int)(event.stop % 3600)/60);
      strcat(replyBuffer, "of ");

      bool first = true;   // Used to print the ',' only when neeeded
      for (uint8_t i = 0; i < 7; i++) {
        if (bitRead(event.days, i)) {
          if (!first) strcat(replyBuffer, ", ");
          first = false;
          strcat(replyBuffer, dayNames[i]);
        }
      }
      theBot.sendMessage(theMsg, replyBuffer);
      delay(100);
    }
  }
  else
    theBot.sendMessage(theMsg, "Timer list empty");
}

void showKeypad(TBMessage &theMsg, AsyncTelegram2 &theBot, const String& msg, bool clear = true) {

  // The inline keyboard can be created at runtime, or also a static string loaded from flash
  static const char keypad_kbd[] PROGMEM =  R"EOF(
  {
  "inline_keyboard": [
    [
      {"text":"7","callback_data":"7"},
      {"text":"8","callback_data":"8"},
      {"text":"9","callback_data":"9"},
      {"text":"←","callback_data":"DELETE"}
    ],
    [
      {"text":"4","callback_data":"4"},
      {"text":"5","callback_data":"5"},
      {"text":"6","callback_data":"6"},
      {"text":"/","callback_data":"/"}
    ],
    [
      {"text":"1","callback_data":"1"},
      {"text":"2","callback_data":"2"},
      {"text":"3","callback_data":"3"},
      {"text":":","callback_data":":"}
    ],
    [
      {"text":"0","callback_data":"0"},
      {"text":".","callback_data":"."},
      {"text":"OK","callback_data": "CONFIRM"}
    ]
  ]
}
)EOF";

  // Clear input buffer ?
  if (clear) {
    memset(userInput, '\0', sizeof(userInput));
    inputIndex = 0;
  }

  if( !inlineKeyboardSent) {
    inlineKeyboardSent = true;
    theBot.sendMessage(theMsg, msg.c_str(), keypad_kbd);
  }
  else {
    theBot.editMessage(theMsg, msg, keypad_kbd);
  }

  awaitUserInput = true;
}


bool schedulerHandler(TBMessage &theMsg, AsyncTelegram2 &theBot) {
  static uint8_t waitState = IDLE;         // State for event handling state machine
  static uint32_t waitTime;                // Timeout for waiting user messages

  // This handler only parses messages of type MessageQuery and MessageText
  if (theMsg.messageType == MessageText ) {
    String msgText = theMsg.text;

    // Is a supported scheduler command?
    if ( msgText.equalsIgnoreCase("/edit")) {
      waitState = EDIT_EVENT;
      InlineKeyboard eventKbd;
      if (lastEvent != 0) {
        for (uint8_t i = 0; i < lastEvent; i++) {
          String lbl = "Timer " ;
          lbl += String(i+1);
          eventKbd.addButton(lbl.c_str(), String(i+1).c_str(), KeyboardButtonQuery );
          if (i==3) eventKbd.addRow();
        }
      }
      theBot.sendMessage(theMsg, "Select Timer to be edited:", eventKbd);
      inlineKeyboardSent = true;
      return true;
    }
    else if ( msgText.equalsIgnoreCase("/list") ) {
      listScheduledEvents(theMsg, theBot);
      return true;
    }
    else if (msgText.equalsIgnoreCase("/add")) {
      eventIndex = MAX_EVENTS;
      // Create InlineKeyboard keyboard and send with message
      InlineKeyboard scheduleKbd;
      getSchedulerKeyboard(scheduleKbd);
      theBot.sendMessage(theMsg, "Add new scheduled event:", scheduleKbd);
      inlineKeyboardSent = true;
      return true;
    }
    else if (msgText.equalsIgnoreCase("/clear")) {
      lastEvent = 0;
      return true;
    }
  }

  // Received a callback query message (aka text associated to inline buttons)
  else if (theMsg.messageType == MessageQuery ) {
    waitTime = millis();
    String queryMsg = theMsg.callbackQueryData;   // Get the callback query text

    // if awaitUserInput is true, this reply could be one of keypad inline button
    if(awaitUserInput) {
      //theBot.endQuery(theMsg, userInput);

      // Is a number or one of characters '.',  ':',  '/'
      if (queryMsg.length() == 1) {
        userInput[inputIndex++] = queryMsg[0];
        // Update keypad and text of message
        showKeypad(theMsg, theBot, userInput, false);
      }

      else if (queryMsg.equals("DELETE")) {
        inputIndex--;
        userInput[inputIndex] = '\0';
        showKeypad(theMsg, theBot, userInput, false);
      }
      else if (queryMsg.equals("CONFIRM")) {

         // User has confirmed the new start time
        if (waitState == GET_START){
          Serial.println(F("Start Time received from user"));
          int hour = 10, minute = 0;
          sscanf (userInput, "%d:%d", &hour, &minute);
          theEvent.start = hour * 3600 + minute * 60;

          // Now query user for stop time
          showKeypad(theMsg, theBot, "Please insert Stop time");
          waitState = GET_STOP;
          waitTime = millis();
        }

        // User has confirmed the new stop time
        else if (waitState == GET_STOP){
          theBot.endQuery(theMsg, "OK");
          Serial.println(F("Stop Time received from user"));
          int hour = 11, minute = 0;
          sscanf (userInput, "%d:%d", &hour, &minute);
          theEvent.stop = hour * 3600 + minute * 60;

          // Now we have collected all required data, add or edit event
          int evt = addEvent(theEvent, eventIndex);
          InlineKeyboard dummy;
          theBot.editMessage(theMsg, "Timer saved!", dummy);
          listScheduledEvents(theMsg, theBot, evt);
          waitState = IDLE;
          awaitUserInput = false;
        }
      }

      return true;
    }

    // User has just pushed SINGLE_DAY inline button
    if (queryMsg.equals(SINGLE_DAY)) {
      // Create day-name keyboard and send with message
      InlineKeyboard weekdaysKbd;
      getWeekdayKeyboard(weekdaysKbd);
      theBot.editMessage(theMsg, "Select the day", weekdaysKbd);
      inlineKeyboardSent = true;
      waitState = GET_DAY;
      return true;
    }

    // User has just pushed ALL_DAYS inline button
    else if (queryMsg.equals(ALL_DAYS)) {

      // Set active days for this event
      Serial.println(ALL_DAYS);
      for (uint8_t i = 0; i < 7; i++)
        bitSet(theEvent.days, i);

      // Query user for start time
      showKeypad(theMsg, theBot, "Please insert Start time");
      waitState = GET_START;
      return true;
    }

    // User has just pushed WORK_DAYS inline button
    else if (queryMsg.equals(WORK_DAYS)) {

      // Set active days for this event
      Serial.println(WORK_DAYS);
      for (uint8_t i = 0; i < 5; i++)
        bitSet(theEvent.days, i);

      // Query user for start time
      showKeypad(theMsg, theBot, "Please insert Start time");
      waitState = GET_START;
      return true;
    }

    // User has just pushed WEEKEND inline button
    else if (queryMsg.equals(WEEKEND)) {

      // Set active days for this event
      Serial.println(WEEKEND);
      for (uint8_t i = 5; i < 7; i++)
        bitSet(theEvent.days, i);

      // Query user for start time
      showKeypad(theMsg, theBot, "Please insert Start time");
      waitState = GET_START;
      return true;
    }

    // Depending from state take action with user inputs
    switch (waitState) {

      // If actual state is GET_DAY, we are waiting for one of day name inline button
      case GET_DAY: {
        waitState = IDLE;
        waitTime = millis();
        for (uint8_t i = 0; i < 7; i++) {
          if (queryMsg.equals(dayNames[i])) {
            bitSet(theEvent.days, i);

            // Query user for start time
            showKeypad(theMsg, theBot, "Please insert Start time");
            waitState = GET_START;
            return true;
          }
        }
        break;
      }

      // If actual state is EDIT_EVENT, we are waiting for a event number (with inline buttons)
      case EDIT_EVENT: {
        eventIndex = queryMsg.toInt();
        eventIndex--;       // array starts from 0, but humans don't like usually
        snprintf(replyBuffer, MAX_MSG_LEN, "Editing Timer n° %d.\nSelect timer type", eventIndex + 1 );

        // Ask user wich type of weekly events is
        InlineKeyboard scheduleKbd;
        getSchedulerKeyboard(scheduleKbd);
        theBot.editMessage(theMsg, replyBuffer, scheduleKbd);
        return true;
      }
    }

  }

  // Timeout
  if (millis() - waitTime > 15000 && waitState != IDLE) {
    waitState = IDLE;
    eventIndex = MAX_EVENTS;
    theBot.sendMessage(theMsg, "You did not respond in time", "");
  }

  // This message was'nt for this handler
  return false;
}
