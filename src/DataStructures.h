
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

#define BUFFER_BIG          2048 		// json parser buffer size (ArduinoJson v6)
#define BUFFER_MEDIUM     	1024 		// json parser buffer size (ArduinoJson v6)
#define BUFFER_SMALL      	512 		// json parser buffer size (ArduinoJson v6)

enum MessageType {
  MessageNoData   = 0,
  MessageText     = 1,
  MessageQuery    = 2,
  MessageLocation = 3,
  MessageContact  = 4,
  MessageDocument = 5,
  MessageReply 	  = 6,
  MessageNewMember = 7,
  MessageLeftMember =8,
  MessageForwarded = 9
};

struct TBUser {
  bool     isBot;
  int64_t  id = 0;
  String   firstName;
  String   lastName;
  String   username;
};

struct TBLocation{
  float longitude;
  float latitude;
};

struct TBContact {
  int64_t id;
  String  phoneNumber;
  String  firstName;
  String  lastName;
  String  vCard;
};

struct TBDocument {
  bool    file_exists;
  int32_t file_size;
  String  file_id;
  String  file_name;
  String  file_path;
};

struct TBMessage {
  MessageType   messageType;
  bool          isHTMLenabled = true;
  bool          isMarkdownEnabled = false;
  bool          disable_notification = false;
  bool          force_reply = false;
  int32_t       date;
  int32_t       chatInstance;
  int64_t       chatId;
  int32_t       messageID;
  TBUser        sender;
  TBUser        member;		// A user enter or leave a group
  TBLocation    location;
  TBContact     contact;
  TBDocument    document;
  int64_t       callbackQueryID;
  String        callbackQueryData;
  String      	text;
};

#endif

