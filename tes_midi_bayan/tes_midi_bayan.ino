/*
 *  This sketch implements a MIDI-bayan based on "Rubin-5".
 * 
 *  All rights reserved (c) Alexander Tesanov
 *  2024
 */

#include "TesLedSet.h"
#include "TesKeyboard.h"
#include "TesPressureSensor.h"
#include "TesBatterySensor.h"
#include "TesGyroSensor.h"
#include "TesMidiController.h"
#include "swer.h"
#include "common.h"
#include "eeprom24.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// Shows an error code on LEDs and goes into an infinite loop,
// so that the system stops reacting to anything.
void    SWER(uint8_t   error_code){

    ledSet.showDebugCode( error_code, 0x01 );  // SWER always turns the "MIDI out" LED on
    PRINT_2_HEX("*** SWER = ", error_code);
    for(;;);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
    Serial1.begin(31250);    // standard MIDI baud rate
    while(!Serial1)
        ;
#if defined(TES_MIDI_DEBUG) || defined(DEBUG_ON)
    Serial.begin(9600);
    while(!Serial)
        ;
#endif

    // we're setting this here, because the i2c intgerface is used not only by the display in this version;
    // so this setting is common now
    Wire.setClock(800000L); // Maximum speed

    // SPI settings are commom for all SPI devices
    SPI1.setSCK(10);
    SPI1.setMOSI(11);
    SPI1.setMISO(12);
    SPI1.begin();

    // Initialize the eeprom BEFORE the MIDI controller!
    EEPROM.init();
    // Init the LED panel
    ledSet.reset();
    // Initialize the Keyboard
    theKeyboard.init();
    // Initialize the pressure sensor
    pressureSensor.init();
    // Initialize the battery sensor
    batterySensor.init();
    // Initialize the Gyro sensor
    gyroSensor.init();
    // Initialize the MIDI controller
    theMIDIController.init();
}

/////////////////////////////////////////////////////////////
// the loop
void loop() {
    ledSet.tick();
    theKeyboard.tick();
    pressureSensor.tick();
    batterySensor.tick();
    gyroSensor.tick();
    theMIDIController.tick();

    // very simplified event dispatcher
    while ( !theQueue.isEmpty() ){
        tesEvent    *event = theQueue.pullEvent();
        
        // call all event processors here
        theMIDIController.processEvent(event);
        gyroSensor.processEvent(event);
    }
}

