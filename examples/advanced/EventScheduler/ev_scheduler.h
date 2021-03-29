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
extern ReplyKeyboard weekdaysKbd;
extern InlineKeyboard scheduleKbd;
extern ReplyKeyboard myKbd;

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
    if(index == MAX_EVENTS){
        events[lastEvent++] = event;
    }
    else if (index <= lastEvent){
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

void listScheduledEvents(TBMessage* msg, AsyncTelegram2* myBot) {

    for(uint8_t i=0; i<lastEvent; i++){
        Event_t event = events[i];
        snprintf(replyBuffer, MAX_MSG_LEN, "Setpoint %d.%d °C\nStart: %02d:%02d\nEnd: %02d:%02d\n",
            (int)event.setpoint, (int)(event.setpoint * 10.0)%10,
            (int)event.start / 3600, (int) (event.start % 3600)/60,
            (int)event.stop / 3600, (int) (event.stop % 3600)/60
        );
        strcat(replyBuffer, "[");
        for(uint8_t i=0; i<7; i++){
            if(bitRead(event.days, i)) {
                strcat(replyBuffer, dayNames[i]);
                strcat(replyBuffer, ", ");
            }
        }
        strcat(replyBuffer, "]");
        myBot->sendMessage(*msg, replyBuffer);
        delay(100);
    }
}


bool schedulerHandler(TBMessage* msg, AsyncTelegram2* myBot) {
    static uint8_t waitState = IDLE;
    static uint32_t waitTime;

    static Event_t newEvent;
    static uint8_t eventIndex = MAX_EVENTS;  //(if eventIndex == MAX_EVENTS -> add event, otherwise edit)
    int hour = 0, min = 0;

    // This handler only parses messages of type MessageQuery and MessageText
    if (msg->messageType == MessageQuery ) {
        waitTime = millis();
        waitState = WAIT_START;

        // received a callback query message
        if (!strcmp (msg->callbackQueryData, SINGLE_DAY)) {
            waitState = WAIT_DAY;
            myBot->endQuery(*msg, SINGLE_DAY);
            myBot->sendMessage(*msg, "Wich days?", weekdaysKbd);
            return true;
        }
        else if (!strcmp (msg->callbackQueryData, ALL_DAYS)) {
            Serial.println(ALL_DAYS);
            for(uint8_t i=0; i<7; i++){
                bitSet(newEvent.days, i);
            }
            myBot->endQuery(*msg, ALL_DAYS);
            myBot->sendMessage(*msg, "Start time? (ex. 15:30)");
            return true;
        }
        else if (!strcmp (msg->callbackQueryData, WORK_DAYS)) {
            Serial.println(WORK_DAYS);
            for(uint8_t i=0; i<5; i++){
                bitSet(newEvent.days, i);
            }
            myBot->endQuery(*msg, WORK_DAYS);
            myBot->sendMessage(*msg, "Start time? (ex. 15:30)");
            return true;
        }
        else if (!strcmp (msg->callbackQueryData, WEEKEND)) {
            Serial.println(WEEKEND);
            for(uint8_t i=5; i<7; i++){
                bitSet(newEvent.days, i);
            }
            myBot->endQuery(*msg, WEEKEND);
            myBot->sendMessage(*msg, "Start time? (ex. 15:30)");
            return true;
        }
        else {
            myBot->endQuery(*msg, "");
            return true;
        }
    }
    // (messgeText)
    else {
        String msgText = msg->text;
        // Is a supported scheduler command?
        if( msgText.equalsIgnoreCase("/edit")) {
            waitState = EDIT_EVENT;
            myBot->sendMessage(*msg, "Number of event to be edited:");
            return true;
        }
        else if( msgText.equalsIgnoreCase("/list") ){
            listScheduledEvents(msg, myBot);
            return true;
        }
        else if (msgText.equalsIgnoreCase("/add")) {
            myBot->sendMessage(*msg, "Andd new scheduled event:", scheduleKbd);
            return true;
        }
        else if (msgText.equalsIgnoreCase("/clear")) {
            lastEvent = 0;
            return true;
        }

        // No, maybe is the text associated with button?
        switch (waitState) {
            case WAIT_DAY:
                waitState = IDLE;
                waitTime = millis();
                for(uint8_t i=0; i<7; i++){
                    if (msgText.equalsIgnoreCase(dayNames[i])) {
                        bitSet(newEvent.days, i);
                        myBot->sendMessage(*msg, "Start time? (ex. 15:30)");
                        waitState = WAIT_START;
                        return true;
                    }
                }
                break;

            case EDIT_EVENT:
                sscanf (msgText.c_str(), "%hhu", &eventIndex);
                eventIndex--;       // array starts from 0, but humans don't like usually
                snprintf(replyBuffer, MAX_MSG_LEN, "Editing event n° %d", eventIndex + 1 );
                myBot->sendMessage(*msg, replyBuffer, scheduleKbd);
                return true;

            case WAIT_START:
                waitState = WAIT_STOP;
                waitTime = millis();
                hour = 0; min = 0;
                sscanf (msgText.c_str(), "%d:%d", &hour, &min);
                newEvent.start = hour*3600 + min*60;
                myBot->sendMessage(*msg, "Stop time? (ex. 16:30)");
                return true;

            case WAIT_STOP:
                waitState = SETPOINT;
                hour = 0; min = 0;
                sscanf (msgText.c_str(), "%d:%d", &hour, &min);
                newEvent.stop = hour*3600 + min*60;
                myBot->sendMessage(*msg, "Temperature setpoint? (es. 25.0)");
                return true;

            case SETPOINT:
                waitState = IDLE;
                int integer = 0, fraction = 0;
                sscanf (msgText.c_str(), "%d.%d", &integer, &fraction);
                newEvent.setpoint = integer + (fraction/10);

                snprintf(replyBuffer, MAX_MSG_LEN, "Setpoint %d.%d °C\nStart: %02d:%02d\nStop: %02d:%02d\n",
                    (int)newEvent.setpoint, (int)(newEvent.setpoint * 10.0)%10,
                    (int)newEvent.start/3600, (int) (newEvent.start % 3600)/60,
                    (int)newEvent.stop/3600, (int) (newEvent.stop % 3600)/60
                );
                strcat(replyBuffer, "[");
                for(uint8_t i=0; i<7; i++){
                    if(bitRead(newEvent.days, i)) {
                        strcat(replyBuffer, dayNames[i]);
                        strcat(replyBuffer, ", ");
                    }
                }
                strcat(replyBuffer, "]");
                myBot->sendMessage(*msg, replyBuffer);

                // Now we have collected all required data, add event to list
                addEvent(newEvent, eventIndex);
                eventIndex = MAX_EVENTS;
                newEvent.days = 0;
                return true;
        }
    }


    if(millis() - waitTime > 15000 && waitState != IDLE){
        waitState = IDLE;
        eventIndex = MAX_EVENTS;
        myBot->sendMessage(*msg, "You did not respond in time", "");
    }

    // This message was'nt for this handler
    return false;
}

