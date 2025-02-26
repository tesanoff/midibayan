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
#include "TesMidiController.h"
#include "swer.h"


////////////////////////////////////////////////////////////////////////////////////////
// The LED set stuff
//
#define ledLatchPin     10    // Pin connected to ST_CP of 74HC595
#define ledClockPin     11    // Pin connected to SH_CP of 74HC595
#define ledDataPin      12    // Pin connected to DS of 74HC595
TesLedSet ledSet(ledLatchPin, ledClockPin, ledDataPin, 9);


// there's the only queue
TesEventQueue   theQueue;

// There's the only Keyboard
TesKeyboard     theKeyboard(&theQueue);

// There's the only pressure sensor
TesPressureSensor   pressureSensor(&theQueue);

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
#ifdef TES_REAL_MIDI_PORT
    Serial.begin(31250);    // standard MIDI baud rate
#else
    Serial.begin(9600);
#endif

    // Init the LED panel
    ledSet.reset();
    // Initialize the Keyboard
    theKeyboard.init();
    // Initialize the pressure sensor
    pressureSensor.init();
    // Initialize the MIDI controller
    theMIDIController.init();
}

/////////////////////////////////////////////////////////////
// the loop
void loop() {
    ledSet.tick();
    theKeyboard.tick();
    pressureSensor.tick();
    theMIDIController.tick();

    // very simplified event dispatcher
    while ( !theQueue.isEmpty() ){
        tesEvent    *event = theQueue.pullEvent();
        
        // call all event processors here
        theMIDIController.processEvent(event);
    }
}

