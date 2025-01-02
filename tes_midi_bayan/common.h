#pragma once

/*
 * This file contains definitions of common types used throughout the sketch.
 */

#include <stdint.h>

// timer - for implementing millis()-based timers
// 16-bit timers would be enough, but they require type casting like "(uint16_t)(millis() - timestamp)", which may be forgotten.
// So, I'm using 32-bit timers in order to avoid potential mistakes.
typedef uint32_t    timer_t;

// debug mode
#define DEBUG_ON

#ifdef DEBUG_ON
#define PRINT_1(value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.println(value);}
#define PRINT_2(text,value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.print(text); Serial.println(value);}
#define PRINT_2_HEX(text,value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.print(text); Serial.println(value, HEX);}
#else
#define PRINT_1
#define PRINT_2
#define PRINT_2_HEX
#endif
