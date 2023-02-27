#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

// #define _LOG_FORMAT(letter, format)  "\n[" #letter "][%s:%u] %s():\t" format, __FILE_NAME__, __LINE__, __FUNCTION__

#if DEBUG_ENABLE
// Windows
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
// Linux, Mac
// #define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#if defined(ESP32) || defined(ESP8266)
#define PRINT_TIME() {struct tm t; time_t now = time(nullptr); t = *localtime(&now); \
char str[30]; strftime(str, sizeof(str), "%c", &t); Serial.print(str);}
#else
#define PRINT_TIME() Serial.print(millis());
#endif


#define PRINT_FILE_LINE() {PRINT_TIME() Serial.print(" [");Serial.print(__FILE_NAME__); \
Serial.print(":");Serial.print(__LINE__);Serial.print(" "); Serial.print(__FUNCTION__); Serial.print("()]\t");}
#if defined(ESP32) || defined(ESP8266)
#define log_debug(format, args...) { PRINT_FILE_LINE() Serial.printf(format, args); Serial.println();}
#else
#define log_debug(format, args...) { PRINT_FILE_LINE() char buf[250];	sprintf(buf, format, args); Serial.println(buf);}
#endif

#define log_error(x) {PRINT_FILE_LINE() Serial.println(x);}
#define log_info(x) { PRINT_FILE_LINE() Serial.println(x);}
#define log_warning(args) { PRINT_FILE_LINE() Serial.print("-W-"); Serial.println(args);}
// #define log_debug(format, ...) Serial.printf(_LOG_FORMAT(D, format), ##__VA_ARGS__)
// #define log_error(format, ...) { Serial.println(); Serial.printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
#define debugJson(X, Y) { PRINT_FILE_LINE() Serial.println(); serializeJsonPretty(X, Y); Serial.println();}
#define errorJson(E) { PRINT_FILE_LINE() Serial.println(); Serial.println(E); }

#else
#define log_debug(format, ...)
#define log_warning(args)
#define log_info(x)
#define log_error(x)

#define debugJson(X, Y)
#define errorJson(E)
#endif


#ifdef __cplusplus
}
#endif

#endif
