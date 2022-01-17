#include <SPI.h>
#include <Ethernet.h>
#include <SSLClient.h>

#include <AsyncTelegram2.h>
#include <tg_certificate.h>

const char* token =  "xxxxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxx";

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);

EthernetClient base_client;
SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR );

AsyncTelegram2 myBot(client);
ReplyKeyboard myReplyKbd;   // reply keyboard object helper
InlineKeyboard myInlineKbd, myInlineKbd2; // inline keyboard object helper
bool isKeyboardActive;      // store if the reply keyboard is shown

#define LIGHT_ON_CALLBACK  "lightON"   // callback data sent when "LIGHT ON" button is pressed
#define LIGHT_OFF_CALLBACK "lightOFF"  // callback data sent when "LIGHT OFF" button is pressed
#define BUTTON1_CALLBACK   "Button1"   // callback data sent when "Button1" button is pressed
#define BUTTON2_CALLBACK   "Button2"   // callback data sent when "Button1" button is pressed

const byte BLINK_LED = 7;
const byte LED = 8;

void button1Pressed(const TBMessage &queryMsg){
  Serial.printf("\nButton 1 pressed (callback); \nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "You pressed Button 1", true);
}

void button2Pressed(const TBMessage &queryMsg){
  Serial.printf("\nButton 2 pressed (callback); \nQueryId: %s\n\n", queryMsg.callbackQueryID);
  myBot.endQuery(queryMsg, "You pressed Button 2", false);
}


void setup() {
  pinMode(BLINK_LED, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(10, OUTPUT);
  SPI.begin();
  // You can use Ethernet.init(pin) to configure the CS pin
  Ethernet.init(10);  // Most Arduino shields

  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);

  // Set the Telegram bot properies
  myBot.setUpdateTime(3000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");
  // Add reply keyboard
  isKeyboardActive = false;
  // add a button that send a message with "Simple button" text
  myReplyKbd.addButton("Button1");
  myReplyKbd.addButton("Button2");
  myReplyKbd.addButton("Button3");
  // add a new empty button row
  myReplyKbd.addRow();
  // add another button that send the user position (location)
  myReplyKbd.addButton("Send Location", KeyboardButtonLocation);
  // add another button that send the user contact
  myReplyKbd.addButton("Send contact", KeyboardButtonContact);
  // add a new empty button row
  myReplyKbd.addRow();
  // add a button that send a message with "Hide replyKeyboard" text
  // (it will be used to hide the reply keyboard)
  myReplyKbd.addButton("/hide_keyboard");
  // resize the keyboard to fit only the needed space
  myReplyKbd.enableResize();
  // Add sample inline keyboard
  myInlineKbd.addButton("ON", LIGHT_ON_CALLBACK, KeyboardButtonQuery);
  myInlineKbd.addButton("OFF", LIGHT_OFF_CALLBACK, KeyboardButtonQuery);
  myInlineKbd.addRow();
  myInlineKbd.addButton("GitHub", "https://github.com/cotestatnt/AsyncTelegram2/", KeyboardButtonURL);
  myBot.addInlineKeyboard(&myInlineKbd);

  // Add another inline keyboard
  myInlineKbd2.addButton("Button 1", BUTTON1_CALLBACK, KeyboardButtonQuery, button1Pressed);
  myInlineKbd2.addButton("Button 2", BUTTON2_CALLBACK, KeyboardButtonQuery, button2Pressed);
  // Add pointer to this keyboard to bot (in order to run callback function)
  myBot.addInlineKeyboard(&myInlineKbd2);

  // Send a welcome message to specific user
  char welcome_msg[128];
  snprintf(welcome_msg, 128, "BOT @%s online\n/help all commands avalaible.", myBot.getBotName());
  int32_t chat_id = 436865110; // You can discover your own chat id, with "Json Dump Bot"
  myBot.sendTo(chat_id, welcome_msg);
}



void loop() {

  static uint32_t ledTime = millis();
  if (millis() - ledTime > 200) {
    ledTime = millis();
    digitalWrite(BLINK_LED, !digitalRead(BLINK_LED));
  }

// a variable to store telegram message data
  TBMessage msg;

  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    // check what kind of message I received
    String tgReply;
    MessageType msgType = msg.messageType;

    switch (msgType) {
      case MessageText :
        // received a text message
        tgReply = msg.text;
        Serial.print("\nText message received: ");
        Serial.println(tgReply);

        // check if is show keyboard command
        if (tgReply.equalsIgnoreCase("/reply_keyboard")) {
          // the user is asking to show the reply keyboard --> show it
          myBot.sendMessage(msg, "This is reply keyboard:", myReplyKbd);
          isKeyboardActive = true;
        }
        else if (tgReply.equalsIgnoreCase("/inline_keyboard")) {
          myBot.sendMessage(msg, "This is inline keyboard:", myInlineKbd);
        }
        else if (tgReply.equalsIgnoreCase("/inline_keyboard2")) {
          myBot.sendMessage(msg, "This is inline keyboard 2:", myInlineKbd2);
        }

        // check if the reply keyboard is active
        else if (isKeyboardActive) {
          // is active -> manage the text messages sent by pressing the reply keyboard buttons
          if (tgReply.equalsIgnoreCase("/hide_keyboard")) {
            // sent the "hide keyboard" message --> hide the reply keyboard
            myBot.removeReplyKeyboard(msg, "Reply keyboard removed");
            isKeyboardActive = false;
          } else {
            // print every others messages received
            myBot.sendMessage(msg, msg.text);
          }
        }

        // the user write anything else and the reply keyboard is not active --> show a hint message
        else {
          myBot.sendMessage(msg, "Try /reply_keyboard or /inline_keyboard or /inline_keyboard2");
        }
        break;

      case MessageQuery:
        // received a callback query message
        tgReply = msg.callbackQueryData;
        Serial.print("\nCallback query message received: ");
        Serial.println(tgReply);

        if (tgReply.equalsIgnoreCase(LIGHT_ON_CALLBACK)) {
          // pushed "LIGHT ON" button...
          Serial.println("\nSet light ON");
          digitalWrite(LED, HIGH);
          // terminate the callback with an alert message
          myBot.endQuery(msg, "Light is on", true);

          myBot.forwardMessage(msg, 1345177361);
        }
        else if (tgReply.equalsIgnoreCase(LIGHT_OFF_CALLBACK)) {
          // pushed "LIGHT OFF" button...
          Serial.println("\nSet light OFF");
          digitalWrite(LED, LOW);
          // terminate the callback with a popup message
          myBot.endQuery(msg, "Light is off");
        }

        break;

      case MessageLocation:
        // received a location message --> send a message with the location coordinates
        char bufL[50];
        snprintf(bufL, sizeof(bufL), "Longitude: %f\nLatitude: %f\n", msg.location.longitude, msg.location.latitude) ;
        myBot.sendMessage(msg, bufL);
        Serial.println(bufL);
        break;

      case MessageContact:
        char bufC[50];
        snprintf(bufC, sizeof(bufC), "Contact information received: %s - %s\n", msg.contact.firstName, msg.contact.phoneNumber ) ;
        // received a contact message --> send a message with the contact information
        myBot.sendMessage(msg, bufC);
        Serial.println(bufC);
        break;

      default:
        break;
    }
  }

}
