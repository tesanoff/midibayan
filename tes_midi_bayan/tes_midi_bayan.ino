/*
 *  This sketch implements a MIDI-bayan based on "Rubin-5".
 * 
 *  All rights reserved (c) Alexander Tesanov
 *  2024
 */

#include <GyverOLED.h>
#include "TesLedSet.h"
#include "TesKeyboard.h"
#include "TesPressureSensor.h"
#include "TesBatterySensor.h"
#include "TesMidiController.h"
#include "swer.h"
#include "common.h"
#include "eeprom24.h"


////////////////////////////////////////////////////////////////////////////////////////
// The LED set stuff
//
#define ledLatchPin     2     // Pin connected to ST_CP of 74HC595
#define ledClockPin     3     // Pin connected to SH_CP of 74HC595
#define ledDataPin      6     // Pin connected to DS of 74HC595
TesLedSet ledSet(ledLatchPin, ledClockPin, ledDataPin, 9);


// there's the only queue
TesEventQueue   theQueue;

// There's the only Keyboard
TesKeyboard     theKeyboard(&theQueue);

// There's the only pressure sensor
TesPressureSensor   pressureSensor(&theQueue);

// There's the only battery sensor
TesBatterySensor   batterySensor(&theQueue);

// the OLED display
GyverOLED<SSH1106_128x64> oled;

// the MIDI controller
TesMIDIController   theMIDIController(&ledSet, &oled);

///////////////////////////////////////////////////////////////////////////////////////////////////
// Shows an error code on LEDs and goes into an infinite loop,
// so that the system stops reacting to anything.
void    SWER(uint8_t   error_code){

    ledSet.showDebugCode( error_code, 0x01 );  // SWER always turns the "MIDI out" LED on
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

    // Initialize the eeprom BEFORE the MIDI controller!
    EEPROM.init();
PRINT_1("EEPROM done");
    // Init the LED panel
    ledSet.reset();
PRINT_1("LED done");
    // Initialize the Keyboard
    theKeyboard.init();
PRINT_1("Kbd done");
    // Initialize the pressure sensor
    pressureSensor.init();
PRINT_1("Pressure done");
    // Initialize the battery sensor
    batterySensor.init();
PRINT_1("Battery done");
    // Initialize the MIDI controller
    theMIDIController.init();
PRINT_1("MIDI controller done");
delay(500); // TODO this is for debugging only. Remove this!
}

/////////////////////////////////////////////////////////////
// the loop
void loop() {
    ledSet.tick();
    theKeyboard.tick();
    pressureSensor.tick();
    batterySensor.tick();
    theMIDIController.tick();

    // very simplified event dispatcher
    while ( !theQueue.isEmpty() ){
        tesEvent    *event = theQueue.pullEvent();
        
        // call all event processors here
        theMIDIController.processEvent(event);
    }
}

