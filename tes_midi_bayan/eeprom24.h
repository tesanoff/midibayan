/*
 * This module implements a wrapper class around a lower-level API for AT24C16 chips
 */
#pragma once
#include <stdint.h>

// this type can address the whole capacity of AT24C16
typedef uint16_t    EepromAddress;

#define baseI2CAddress  0x50
#define maxI2CAddress   0x57
#define maxLsbAddress   0xFF

#define updateBufferSize    256     // high RAM consumption. It's ok for RP2040 but may be a problem for Adruino nano

class eeprom24 {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // initializes the instance
    void    init(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // reads a byte from the specified address
    uint8_t readByte(EepromAddress  address);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // reads a 16-bit word from the specified address
    uint16_t readUint16(EepromAddress  address);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Reads a specified number of bytes.
    // Returns the number of bytes actually read.
    int read(EepromAddress address, void *buffer, int size);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // writes a byte to the specified address
    void writeByte(EepromAddress  address, uint8_t value);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // writes a 16-bit word to the specified address
    bool writeUint16(EepromAddress  address, uint16_t value);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Writes a specified number of bytes.
    // Returns the number of bytes actually written.
    int write(EepromAddress address, void *buffer, int size);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Writes a byte, if the new value differs from the old one
    void updateByte(EepromAddress  address, uint8_t value);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Writes a 16-bit word, if the new value differs from the old one
    bool updateUint16(EepromAddress  address, uint16_t value);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Updates a specified number of bytes. Writing to EEPROM is done only for those bytes which differ
    // from the current content of EEPROM.
    // Returns the number of bytes actually written.
    int update(EepromAddress address, void *buffer, int size);

private:
    uint8_t     _update_buffer[updateBufferSize];
};

extern  eeprom24    EEPROM;

