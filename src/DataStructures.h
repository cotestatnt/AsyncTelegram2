
#ifndef DATA_STRUCTURES
#define DATA_STRUCTURES

#define BUFFER_BIG          2048 		// json parser buffer size (ArduinoJson v6)
#define BUFFER_MEDIUM     	1028 		// json parser buffer size (ArduinoJson v6)
#define BUFFER_SMALL      	512 		// json parser buffer size (ArduinoJson v6)

enum MessageType {
  MessageNoData   = 0,
  MessageText     = 1,
  MessageQuery    = 2,
  MessageLocation = 3,
  MessageContact  = 4,
  MessageDocument = 5,
  MessageReply 	= 6
};

struct TBUser {
  bool          isBot;
  int64_t       id = 0;
  const char*   firstName;
  const char*   lastName;
  const char*   username;
  const char*   languageCode;
};

struct TBGroup {
  int64_t id;
  const char*  title;
};

struct TBLocation{
  float longitude;
  float latitude;
};

struct TBContact {
  int64_t 	   id;
  const char*  phoneNumber;
  const char*  firstName;
  const char*  lastName;
  const char*  vCard;
};

struct TBDocument {
  bool         file_exists;
  int32_t      file_size;
  const char*  file_id;
  const char*  file_name;
  String	   file_path;
};

struct TBMessage {
  MessageType 	  messageType;
  bool			  isHTMLenabled = false;
  bool       	  isMarkdownEnabled = false;
  bool 	     	  disable_notification = false;
  bool			  force_reply = false;
  int32_t         date;
  int32_t         chatInstance;
  int64_t         chatId;
  int32_t         messageID;
  TBUser          sender;
  TBGroup         group;
  TBLocation      location;
  TBContact       contact;
  TBDocument      document;
  const char*     callbackQueryID;
  const char*     callbackQueryData;
  String      	  text;
};

#endif

