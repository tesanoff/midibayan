#include <string.h>
#include "common.h"
#include "BUILD_ID.hpp"

const   uint32_t    build_id    = BUILD_ID;

///////////////////////////////////////////////////////////////////////////
// Functions for manipulating with bitmaps
//

// Clears the bitmap
void    bitmap_clear(uint8_t * bitmap, uint8_t size){
    memset(bitmap, 0, size);
}

// Sets the specified bit
void    bitmap_set(uint8_t * bitmap, uint8_t id, uint8_t value){
    // calculating byteIndex and bitIndex for accessing the bitmap
    uint8_t byteIndex = id / 8;
    uint8_t bitIndex  = id % 8;

    value &= (uint8_t)1;    // clear unused bits
    bitmap[byteIndex] = (bitmap[byteIndex] & ~((uint8_t)1 << bitIndex)) | (value << bitIndex);
}

// Gets the value of the specified bit
uint8_t bitmap_get(const uint8_t * bitmap, uint8_t id){
    // calculating byteIndex and bitIndex for accessing the bitmap
    uint8_t byteIndex = id / 8;
    uint8_t bitIndex  = id % 8;
    // return the value
    return (bitmap[byteIndex] >> bitIndex) & (uint8_t)1;
}

