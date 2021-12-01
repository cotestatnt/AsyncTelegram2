#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C"
{
#endif

// Windows
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

// Linux, Mac
// #define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define _LOG_FORMAT(letter, format)  "\n[" #letter "][%s:%u] %s():\t" format, __FILE_NAME__, __LINE__, __FUNCTION__

#if DEBUG_ENABLE
#define log_debug(format, ...) Serial.printf(_LOG_FORMAT(D, format), ##__VA_ARGS__)
#define log_error(format, ...) { Serial.println(); Serial.printf(_LOG_FORMAT(E, format), ##__VA_ARGS__); }
#define log_info(format, ...) Serial.printf(_LOG_FORMAT(I, format), ##__VA_ARGS__)
#define lineTrap() {Serial.printf("[%s:%u] - ", __FILE_NAME__, __LINE__); Serial.print(__func__); Serial.println("()");}
#else
#define log_debug(format, ...)
#define log_error(format, ...)
#define log_info(format, ...)
#define lineTRap()
#endif

#define DEBUG_F true
#if DEBUG_F
	#ifdef ESP32
		#define functionLog() { \
		Serial.printf("Heap memory %6d / %6d", heap_caps_get_free_size(0), heap_caps_get_largest_free_block(0));\
		Serial.print("\t\t\t--- "); Serial.print(millis()); Serial.print("mS > ");  Serial.print(__func__); Serial.println("()"); }
	#elif defined(ESP8266)
		#define functionLog() { \
		uint32_t free; uint16_t max; uint8_t frag; ESP.getHeapStats(&free, &max, &frag); Serial.printf("free: %5d - max: %5d <- ", free, max);\
		Serial.printf("[%s:%u]\t--- ", __FILE__, __LINE__); Serial.print(millis()); Serial.print("mS > ");  Serial.print(__func__); Serial.println("()"); }
	#endif
#else
    #define functionLog()
#endif


#ifdef __cplusplus
}
#endif

#endif
