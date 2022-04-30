#include "AsyncTelegram2.h"

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

const char* dayNames[] = {MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY };
enum WaitState {IDLE, EDIT_EVENT, WAIT_DAY, WAIT_START, WAIT_STOP, SETPOINT};

#define MAX_MSG_LEN 128
#define MAX_EVENTS  20
uint8_t lastEvent = 0;

extern void saveConfigFile();

//extern InlineKeyboard scheduleKbd;
//extern ReplyKeyboard weekdaysKbd;

typedef struct  {
  float setpoint;
  uint32_t start;
  uint32_t stop;
  uint8_t days;
  bool active;
} Event_t;

Event_t events[MAX_EVENTS];
char replyBuffer[MAX_MSG_LEN];

// Add or edit an event
void addEvent(Event_t event, uint8_t index) {
  if (index == MAX_EVENTS) {
    events[lastEvent++] = event;
  }
  else if (index <= lastEvent) {
    events[index] = event;
  }
  saveConfigFile();
}

void getSchedulerKeyboard(InlineKeyboard *kbd) {
  kbd->addButton("Single", SINGLE_DAY, KeyboardButtonQuery);
  kbd->addButton("All days", ALL_DAYS, KeyboardButtonQuery);
  kbd->addButton("Work days", WORK_DAYS, KeyboardButtonQuery);
  kbd->addButton("Weekend", WEEKEND, KeyboardButtonQuery);
}

void getCommandKeyboard(ReplyKeyboard *kbd) {
  kbd->enableResize();
  kbd->addButton("/time");
  kbd->addButton("/list");
  kbd->addButton("/edit");
  kbd->addButton("/add");
}

void getWeekdayKeyboard(ReplyKeyboard *kbd) {
  kbd->enableOneTime();
  kbd->enableSelective();
  kbd->addButton(MONDAY);
  kbd->addButton(TUESDAY);
  kbd->addButton(WEDNESDAY);
  kbd->addRow();
  kbd->addButton(THURSDAY);
  kbd->addButton(FRIDAY);
  kbd->addButton(SATURDAY);
  kbd->addRow();
  kbd->addButton(SUNDAY);
  kbd->addButton("CANCEL");
}

void listScheduledEvents(TBMessage &theMsg, AsyncTelegram2 &theBot) {
  if (lastEvent != 0) {
    for (uint8_t i = 0; i < lastEvent; i++) {
      Event_t event = events[i];
      snprintf(replyBuffer, MAX_MSG_LEN, "Event n.%d\nSetpoint %d.%d °C\nStart: %02d:%02d\nEnd: %02d:%02d\n",
               i + 1, (int)event.setpoint, (int)(event.setpoint * 10.0) % 10,
               (int)event.start / 3600, (int) (event.start % 3600) / 60,
               (int)event.stop / 3600, (int) (event.stop % 3600) / 60
              );
      strcat(replyBuffer, "[");
      for (uint8_t i = 0; i < 7; i++) {
        if (bitRead(event.days, i)) {
          strcat(replyBuffer, dayNames[i]);
          strcat(replyBuffer, ", ");
        }
      }
      strcat(replyBuffer, "]");
      theBot.sendMessage(theMsg, replyBuffer);
      delay(100);
    }
  }
  else
    theBot.sendMessage(theMsg, "Event list empty");
}


bool schedulerHandler(TBMessage &theMsg, AsyncTelegram2 &theBot) {
  static uint8_t waitState = IDLE;         // State for event handling state machine 
  static uint32_t waitTime;                // Timeout for waiting user messages

  static Event_t theEvent;                 // New istance of Event_t struct      
  static uint8_t eventIndex = MAX_EVENTS;  // if eventIndex == MAX_EVENTS -> add event, otherwise edit
  int hour = 0, min = 0;

  // This handler only parses messages of type MessageQuery and MessageText
  if (theMsg.messageType == MessageQuery ) {
    waitTime = millis();
    waitState = WAIT_START;

    // received a callback query message
    if (!strcmp (theMsg.callbackQueryData.c_str(), SINGLE_DAY)) {
      waitState = WAIT_DAY;
      theBot.endQuery(theMsg, SINGLE_DAY);

      // Create ReplyKeyboard keyboard and send with message
      ReplyKeyboard weekdaysKbd;
      getWeekdayKeyboard(&weekdaysKbd);
      theBot.sendMessage(theMsg, "Wich days?", weekdaysKbd);
      return true;
    }
    else if (!strcmp (theMsg.callbackQueryData.c_str(), ALL_DAYS)) {
      Serial.println(ALL_DAYS);
      for (uint8_t i = 0; i < 7; i++) {
        bitSet(theEvent.days, i);
      }
      theBot.endQuery(theMsg, ALL_DAYS);
      theBot.sendMessage(theMsg, "Start time? (ex. 15:30)");
      return true;
    }
    else if (!strcmp (theMsg.callbackQueryData.c_str(), WORK_DAYS)) {
      Serial.println(WORK_DAYS);
      for (uint8_t i = 0; i < 5; i++) {
        bitSet(theEvent.days, i);
      }
      theBot.endQuery(theMsg, WORK_DAYS);
      theBot.sendMessage(theMsg, "Start time? (ex. 15:30)");
      return true;
    }
    else if (!strcmp (theMsg.callbackQueryData.c_str(), WEEKEND)) {
      Serial.println(WEEKEND);
      for (uint8_t i = 5; i < 7; i++) {
        bitSet(theEvent.days, i);
      }
      theBot.endQuery(theMsg, WEEKEND);
      theBot.sendMessage(theMsg, "Start time? (ex. 15:30)");
      return true;
    }
    else {
      theBot.endQuery(theMsg, "");
      return true;
    }
  }
  
  // (messageText)
  else {
    String msgText = theMsg.text;
    // Is a supported scheduler command?
    if ( msgText.equalsIgnoreCase("/edit")) {
      waitState = EDIT_EVENT;
      theBot.sendMessage(theMsg, "Number of event to be edited:");
      return true;
    }
    else if ( msgText.equalsIgnoreCase("/list") ) {
      listScheduledEvents(theMsg, theBot);
      return true;
    }
    else if (msgText.equalsIgnoreCase("/add")) {
      // Create InlineKeyboard keyboard and send with message
      InlineKeyboard scheduleKbd;
      getSchedulerKeyboard(&scheduleKbd);
      theBot.sendMessage(theMsg, "Andd new scheduled event:", scheduleKbd);
      return true;
    }
    else if (msgText.equalsIgnoreCase("/clear")) {
      lastEvent = 0;
      return true;
    }

    // No, maybe is the text associated with button?
    switch (waitState) {
      case WAIT_DAY:
        {
          waitState = IDLE;
          waitTime = millis();
          for (uint8_t i = 0; i < 7; i++) {
            if (msgText.equalsIgnoreCase(dayNames[i])) {
              bitSet(theEvent.days, i);
              theBot.sendMessage(theMsg, "Start time? (ex. 15:30)");
              waitState = WAIT_START;
              return true;
            }
          }
          break;
        }
      case EDIT_EVENT:
        {
          sscanf (msgText.c_str(), "%hhu", &eventIndex);
          eventIndex--;       // array starts from 0, but humans don't like usually
          snprintf(replyBuffer, MAX_MSG_LEN, "Editing event n° %d", eventIndex + 1 );

          // Create InlineKeyboard keyboard and send with message
          InlineKeyboard scheduleKbd;
          getSchedulerKeyboard(&scheduleKbd);
          theBot.sendMessage(theMsg, replyBuffer, scheduleKbd);
          return true;
        }
      case WAIT_START:
        {
          waitState = WAIT_STOP;
          waitTime = millis();
          hour = 0; min = 0;
          sscanf (msgText.c_str(), "%d:%d", &hour, &min);
          theEvent.start = hour * 3600 + min * 60;
          theBot.sendMessage(theMsg, "Stop time? (ex. 16:30)");
          return true;
        }

      case WAIT_STOP:
        {
          waitState = SETPOINT;
          hour = 0; min = 0;
          sscanf (msgText.c_str(), "%d:%d", &hour, &min);
          theEvent.stop = hour * 3600 + min * 60;
          theBot.sendMessage(theMsg, "Temperature setpoint? (es. 25.0)");
          return true;
        }

      case SETPOINT:
        {
          waitState = IDLE;
          int integer = 0, fraction = 0;
          sscanf (msgText.c_str(), "%d.%d", &integer, &fraction);
          theEvent.setpoint = integer + (fraction / 10);

          snprintf(replyBuffer, MAX_MSG_LEN, "Setpoint %d.%d °C\nStart: %02d:%02d\nStop: %02d:%02d\n",
                   (int)theEvent.setpoint, (int)(theEvent.setpoint * 10.0) % 10,
                   (int)theEvent.start / 3600, (int) (theEvent.start % 3600) / 60,
                   (int)theEvent.stop / 3600, (int) (theEvent.stop % 3600) / 60
                  );
          strcat(replyBuffer, "[");
          for (uint8_t i = 0; i < 7; i++) {
            if (bitRead(theEvent.days, i)) {
              strcat(replyBuffer, dayNames[i]);
              strcat(replyBuffer, ", ");
            }
          }
          strcat(replyBuffer, "]");
          theBot.sendMessage(theMsg, replyBuffer);

          // Now we have collected all required data, add event to list
          addEvent(theEvent, eventIndex);
          eventIndex = MAX_EVENTS;
          theEvent.days = 0;
          return true;
        }
    }
  }


  if (millis() - waitTime > 15000 && waitState != IDLE) {
    waitState = IDLE;
    eventIndex = MAX_EVENTS;
    theBot.sendMessage(theMsg, "You did not respond in time", "");
  }

  // This message was'nt for this handler
  return false;
}
