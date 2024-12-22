/*
 * taken from https://programmersought.com/article/598311367038/
 */

#include <Arduino.h>
#include <Wire.h>
#include "eeprom24.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////
/// Original functions from at24cxx.c
///

// the only instance of the EEPROM class
eeprom24    EEPROM;

//+++++
// definitions from the original at24cxx.h file
#define AT24C16

#ifdef AT24C02
#define PAGE_SIZE   (8)
#endif
#if defined (AT24C04) || defined (AT24C08) || defined (AT24C16)
#define PAGE_SIZE   (16)
#endif
#if defined(AT24C32) || defined(AT24C64)
#define PAGE_SIZE   (32)
#endif
// +++++++++

static void at24cxx_init(void)
{
    // the assumption is that calling Wire.begin() multiple times doesn't cause any issues
    Wire.begin();
}

static void at24cxx_wait(int i2c_address)
{
    int resault = 0;
    do
    {
        Wire.beginTransmission(i2c_address);
        resault = Wire.endTransmission();
    } while (resault != 0);
}

static void at24cxx_write_byte(int i2c_address, int address_lsb, int data)
{
    at24cxx_wait(i2c_address);
    Wire.beginTransmission(i2c_address);
    Wire.write(address_lsb);  /*Send writing address*/
    Wire.write(data);  /*Write data*/
    Wire.endTransmission();
}

static int at24cxx_read_byte(int i2c_address, int address_lsb)
{
    int  ret = 0xFF;
    at24cxx_wait(i2c_address);
    Wire.beginTransmission(i2c_address);
    Wire.write(address_lsb);  /*Send reading address*/
    Wire.endTransmission();   
    /*Read data*/
    Wire.requestFrom(i2c_address, 1);
    ret = Wire.read();
    return ret;
}

static int at24cxx_write(int i2c_address, int address_lsb, const uint8_t *buf, int buflen)
{
    int left_len = buflen;
    int send_len = PAGE_SIZE-(address_lsb%PAGE_SIZE);
    send_len = (left_len>send_len) ? send_len:left_len;
    while(left_len)
    {
        at24cxx_wait(i2c_address);
        Wire.beginTransmission(i2c_address);
        Wire.write(address_lsb);  /*Send writing address*/
        Wire.write(buf, send_len);  /*Write data*/
        Wire.endTransmission();
        left_len -= send_len;
        address_lsb += send_len;
        buf += send_len;
        send_len = (left_len>PAGE_SIZE) ? PAGE_SIZE:left_len;
    }
    return buflen;
}


static int at24cxx_read(int i2c_address, int address_lsb, uint8_t *buf, int buflen)
{
    int left_len = buflen;
    int get_len = 0;
    while(left_len)
    {
        if(left_len > 32)
        {
            get_len = 32;
        }
        else
        {
            get_len = left_len;
        }
        at24cxx_wait(i2c_address);
        Wire.beginTransmission(i2c_address);
        Wire.write(address_lsb);  /*Send reading address*/
        Wire.endTransmission();   
        /*Read data*/
        Wire.requestFrom(i2c_address, get_len);
        for(int i=0; i<get_len; i++)
        {
            buf[i] = Wire.read();
        }
        left_len -= get_len;
        buf += get_len;
        address_lsb += get_len;
    }
    return buflen;
}

/// End of Original functions from at24cxx.c
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Implementation of our EEPROM class
///

///////////////////////////////////////////////////////////////////////////////////////////////
// initializes the instance
void    eeprom24::init(void){
    at24cxx_init();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// reads a byte from the specified address
uint8_t eeprom24::readByte(EepromAddress  address){
    uint8_t i2c_address     = (uint8_t)baseI2CAddress | ((address >> 8) & (uint8_t)0x07);
    uint8_t address_lsb     = address & (uint16_t)0x00FF;

    return at24cxx_read_byte(i2c_address, address_lsb);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// reads a 16-bit word from the specified address
uint16_t eeprom24::readUint16(EepromAddress  address){
    uint16_t    ret_val = 0;

    read(address, &ret_val, sizeof(ret_val));
    return ret_val;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Reads a specified number of bytes.
// Returns the number of bytes actually read.
int eeprom24::read(EepromAddress address, void *buffer, int size){
    uint8_t i2c_address     = (uint8_t)baseI2CAddress | ((address >> 8) & (uint8_t)0x07);
    uint8_t address_lsb     = address & (uint16_t)0x00FF;
    int     read_size       = 0;

    int size_to_read_later = 0;
    do {
        int size_to_read_now    = ((((int)address_lsb + size) - 1) <= maxLsbAddress)? size : (maxLsbAddress - address_lsb + 1) ;
        size_to_read_later  = size - size_to_read_now;

        // read the current portion
        at24cxx_read(i2c_address, address_lsb, (uint8_t*)buffer, size_to_read_now);
        // adjust pointers & counters
        buffer = (uint8_t *)buffer + size_to_read_now;  // position for the next read
        read_size += size_to_read_now;  // counting the number of read bytes
        if (size_to_read_later > 0){
            if (i2c_address++ == maxI2CAddress){
                // unable to read further
                return read_size;
            }
            address_lsb = 0;            // the next block will be read from the 1st address
            size = size_to_read_later;  // this value will be used again in the beginning of the cycle
        }
    } while (size_to_read_later > 0);

    return  read_size;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// writes a byte to the specified address
void eeprom24::writeByte(EepromAddress  address, uint8_t value){
    uint8_t i2c_address     = (uint8_t)baseI2CAddress | ((address >> 8) & (uint8_t)0x07);
    uint8_t address_lsb     = address & (uint16_t)0x00FF;

    at24cxx_write_byte(i2c_address, address_lsb, value);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// writes a 16-bit word to the specified address
bool eeprom24::writeUint16(EepromAddress  address, uint16_t value){

    return (sizeof(value) == write(address, &value, sizeof(value)));
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Writes a specified number of bytes.
// Returns the number of bytes actually written.
int eeprom24::write(EepromAddress address, void *buffer, int size){
    uint8_t i2c_address     = (uint8_t)baseI2CAddress | ((address >> 8) & (uint8_t)0x07);
    uint8_t address_lsb     = address & (uint16_t)0x00FF;
    int     written_size       = 0;

    int size_to_write_later = 0;
    do {
        int size_to_write_now    = ((((int)address_lsb + size) - 1) <= maxLsbAddress)? size : (maxLsbAddress - address_lsb + 1) ;
        size_to_write_later  = size - size_to_write_now;

        // write the current portion
        at24cxx_write(i2c_address, address_lsb, (uint8_t*)buffer, size_to_write_now);
        // adjust pointers & counters
        buffer = (uint8_t *)buffer + size_to_write_now;        // position for the next write
        written_size += size_to_write_now;  // counting the number of written bytes
        if (size_to_write_later > 0){
            if (i2c_address++ == maxI2CAddress){
                // unable to write further
                return written_size;
            }
            address_lsb = 0;                // the next block will be written from the 1st address
            size = size_to_write_later;     // this value will be used again in the beginning of the cycle
        }
    } while (size_to_write_later > 0);

    return  written_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Writes a byte, if the new value differs from the old one
void eeprom24::updateByte(EepromAddress  address, uint8_t value){
    uint8_t i2c_address     = (uint8_t)baseI2CAddress | ((address >> 8) & (uint8_t)0x07);
    uint8_t address_lsb     = address & (uint16_t)0x00FF;

    uint8_t old_value   = at24cxx_read_byte(i2c_address, address_lsb);
    if ( old_value != value){
        at24cxx_write_byte(i2c_address, address_lsb, value);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////
// Writes a 16-bit word, if the new value differs from the old one
bool eeprom24::updateUint16(EepromAddress  address, uint16_t value){
    uint16_t    old_value = 0;

    read(address, &old_value, sizeof(old_value));
    if (old_value != value){
        return (sizeof(value) == write(address, &value, sizeof(value)));
    }
    else {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Updates a specified number of bytes. Writing to EEPROM is done only for those bytes which differ
// from the current content of EEPROM.
// Returns the number of bytes actually written.
int eeprom24::update(EepromAddress address, void *buffer, int size){
    int     written_size       = 0;

    int size_to_write_later = 0;
    do {
        int size_to_write_now   = (size <= updateBufferSize)? size : updateBufferSize ;
        size_to_write_later     = size - size_to_write_now;

        // read the current portion
        int actually_read = read(address, _update_buffer, size_to_write_now);
        if(actually_read != size_to_write_now){
            // there's an attempt to write beyond the EEPROM capacity.
            // Limit the operation to the available space in EEPROM.
            size_to_write_now = actually_read;
            size_to_write_later = 0;
        }

        // compare the current portion and write those parts which differ
        bool    write_is_required   = false;
        int     chunk_start = 0;
        for(int i=0; i<size_to_write_now; i++ ){
            if(_update_buffer[i] != *((uint8_t *)buffer + i)){
                if (!write_is_required){
                    // we have found the 1st position of difference
                    write_is_required = true;
                    chunk_start = i;
                }
                else{
                    // nothing to do; keep counting different bytes
                }
            }
            else{
                // *** the current byte doesn't differ
                if(write_is_required){
                    // *** we have found the end of the current chunk
                    // write the current chunk
                    int chunk_size = i - chunk_start;
                    if (chunk_size == 1){   // kind of optimization :)
                        writeByte(address + chunk_start, *((uint8_t *)buffer + chunk_start));
                    }
                    else{
                        write(address + chunk_start, ((uint8_t *)buffer + chunk_start), chunk_size);
                    }
                    // reset the flag
                    write_is_required = false;
                }
            }
        }
        // If the last byte also differs, then we didn't find the end of a chunk in the loop above.
        // This means, we may need to write the last chunk
        if (write_is_required){
            // we know the chunk_start, and the last byte of the buffer is the last byte of the chunk
            write(address + chunk_start, ((uint8_t *)buffer + chunk_start), updateBufferSize -chunk_start);
        }

        // adjust pointers & counters
        buffer = (uint8_t *)buffer + size_to_write_now;        // position for the next write
        written_size += size_to_write_now;  // counting the number of written bytes
        address += size_to_write_now;       // address of the next chunk (if any)
        size = size_to_write_later;         // this value will be used again in the beginning of the cycle
    } while (size_to_write_later > 0);

    return  written_size;
}


