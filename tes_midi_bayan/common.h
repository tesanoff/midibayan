#pragma once

/*
 * This file contains definitions of common types used throughout the sketch.
 */

#include <stdint.h>

// timer - for implementing millis()-based timers
// 16-bit timers would be enough, but they require type casting like "(uint16_t)(millis() - timestamp)", which may be forgotten.
// So, I'm using 32-bit timers in order to avoid potential mistakes.
typedef uint32_t    timer_t;

