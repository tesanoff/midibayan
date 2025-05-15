#pragma once

/*
 * This file contains definitions of common types used throughout the sketch.
 */

#include <stdint.h>

// a constant that keeps a unique ID of the build
// it's calculated as a check sum of all md5 hashes of all files *.cpp *.h *.ino
extern  const   uint32_t    build_id;

// timer - for implementing millis()-based timers
// 16-bit timers would be enough, but they require type casting like "(uint16_t)(millis() - timestamp)", which may be forgotten.
// So, I'm using 32-bit timers in order to avoid potential mistakes.
typedef uint32_t    timer_t;

// debug mode
//#define DEBUG_ON

#ifdef DEBUG_ON
#define PRINT_1(value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.println(value);}
#define PRINT_2(text,value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.print(text); Serial.println(value);}
#define PRINT_2_HEX(text,value) {Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); Serial.print(text); Serial.println(value, HEX);}
#else
#define PRINT_1(value)
#define PRINT_2(text,value)
#define PRINT_2_HEX(text,value)
#endif

///////////////////////////////////////////////////////////////////////////
// Functions for manipulating with bitmaps
//

// Clears the bitmap
void    bitmap_clear(uint8_t * bitmap, uint8_t size);

// Sets the specified bit
void    bitmap_set(uint8_t * bitmap, uint8_t id, uint8_t value);

// Gets the value of the specified bit
uint8_t bitmap_get(const uint8_t * bitmap, uint8_t id);


