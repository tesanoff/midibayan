
/*************************************************************************************
 *  This class implements a full keyboard
 * (including keys, control buttons and button-like sensors)
 *
 * Important: the hardware configuration is hardcoded here. You will need to re-write
 * this class if you have a different set of keys/buttons/sensors.
 *
 * This class is a source of events. It reads data from the shift register cascade(s)
 * and puts respective events to the queue.
 *************************************************************************************/

#include "TesKeyboard.h"
#include "swer.h"
#include "common.h"
#include "TesCtlButtonController.h"

// There's the only Keyboard
TesKeyboard     theKeyboard;

//////////////////////////// settings for control buttons //////////////////////////////
//

// The layout is shown in a declaration oftesButtonPressedLevels below.

// Basic parameters
#define TES_FIRST_CONTROL_BUTTON        64                  // the index of the 1st ctl button
#define TES_LAST_CONTROL_BUTTON         (TES_FIRST_CONTROL_BUTTON + TES_NUMBER_OF_CONTROL_BUTTONS - 1)  // the index of the last ctl button
                                                                                                        // (!) The assumption is that there's just one
                                                                                                        // range of control buttons without any gaps it it.
/////////////////////// Settings for ALL buttons //////////////////////////////////////////////////
//

// "Pressed" and "released" levels are configured for each individual button,
// because musical buttons may be "LOW" when released (in some implementations), and control buttons may have
// different implementation too.
//
// This is implemented as a bitmap. The number of bits is equal to the number of buttons.
const uint8_t   tesButtonPressedLevels[] = {
// Right keyboard:
//    7      0    15     8    23     16   31     24   39     32   47     40   55     48   63     56
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
//                                                    ~~~                                 ~~~~~~    <- not used
// Control Panel:
//    71     64   79     72   87     80
    0b00000000, 0b00000000, 0b00000000,
//                            ~~        <- not used
// Left keyboard
//    95     88   103    96   111    104  119    112  127    120  135    128  143    136
    0b00000001, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000
//           ~ <- FreeBass sensor                                             ~~~     <- not used, and not counted in totalNumberOfButtons
};


/////////////////////// Settings for the HW (2 cascades of 74HC165) //////////////////////////////////////////////////
//

// We configure the HW right here, because there's no sense to do that somewhere else.
// Anyway, if the HW configuration changes - the code is to be adjusted as well.

#define latchPin    7
#define clockPin    8
#define dataPinR    14      // Right keyboard and Control Panel
#define dataPinL    15      // Left keyboard & FreeBass sensor

#define longCascadeSize         11      // 5 right-front + 3 right-back + 3 control panel
#define shortCascadeSize        7       // just left keyboard
#define freeBassSensorIndex     88      // global index of the Free Bass sensor

#define totalNumberOfButtons    141     // this is the number of all buttons/sensors processed via all 74HC165 cascades
                                        // Gaps (unused pins) are also counted. Only the trailing pins are not counted here.

// 
// #define TES_KEYBOARD_CASCADE_SIZE 18		<-- defined in the .h file
//
// Here's a visual representation of the buffer used for reading data from cascades.
// +-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
// |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |  10   |   11  |  12   |   13  |   14  |   15  |   16  |   17  |
// +-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+-------+
//  ^^^^^^^^^^^^^^^^^^^^^^^^^^  long cascade  (right & control)  ^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^  short cascade (left & Free Bass)  ^^^^^^^^


///////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////  now the class itself ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// Public methods

///////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
TesKeyboard::TesKeyboard(void)
{
    //set the pins up
    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPinR, INPUT);
    pinMode(dataPinL, INPUT);
}

///////////////////////////////////////////////////////////
// Initializes the keyboard
void TesKeyboard::init(void){
    // read the current input data for future comparison
    readCascade();  // garbage
    readCascade();  // real data

    // As the FreeBass sensor is part of the left keyboard, we already have it's status after 'readCascade()'.
    // Need to read its status from the buffer and send the very first FreeBass event, so that the MIDI Controller knows the status
    // of the Free Bass switch.
    byte freeBassStatus = bitmap_get(_buffer, freeBassSensorIndex);
    // compose and send a freeBass event
    tesEvent    event;
    event.eventType = tesEvFreeBass;
    event.freeBassOn = (freeBassStatus == getPressedLevel(freeBassSensorIndex));    // "pressed" means "turned on" in this context
    theQueue.pushEvent(&event);
    
    // check if the "OK" button is pressed
    uint8_t buttonId = TES_FIRST_CONTROL_BUTTON + ctlButtonOk;
    uint8_t value = bitmap_get(_buffer, buttonId);
    if (value == getPressedLevel(buttonId)){
        // send Eeprom Reset message
        tesEvent    event;
        event.eventType = tesEvSystemControl;
        event.command   = scEepromReset;
        theQueue.pushEvent(&event);
    }

    // check if the "UP" button is pressed
    buttonId = TES_FIRST_CONTROL_BUTTON + ctlButtonUp;
    value = bitmap_get(_buffer, buttonId);
    if (value == getPressedLevel(buttonId)){
        // send Recalibrate Gyro message
        tesEvent    event;
        event.eventType = tesEvSystemControl;
        event.command   = scRecalibrateGyroSensor;
        theQueue.pushEvent(&event);
    }
}

///////////////////////////////////////////////////////////
// tick method
void TesKeyboard::tick(void){
    // read data
    readCascade();

    // analyze and create new events, if required
    processNewData();

    _ctlButtonController.tick();
}

// Private members

///////////////////////////////////////////////////////////////////////
// Returns a "pressed" level for the specified button
uint8_t TesKeyboard::getPressedLevel(uint8_t buttonId){
    return bitmap_get(tesButtonPressedLevels, buttonId);
}

//////////////////////////////////////////////////////////////////////
// Returns True if the specified button is a control button
//
bool TesKeyboard::isCtlButton(uint8_t buttonIndex){
    return (buttonIndex >= TES_FIRST_CONTROL_BUTTON) && (buttonIndex <= TES_LAST_CONTROL_BUTTON);
}

///////////////////////////////////////////////////////////////////////
// Reads all data from the cascade(s)
// INFO: It reads a 2-registers cascade in 0,2655 ms
//
void TesKeyboard::readCascade(void){
    // save the current buffer
    for (int i = 0; i < TES_KEYBOARD_CASCADE_SIZE; i++){
        _previous_buffer[i] = _buffer[i];
    }

    // step 1: lock the input data
    digitalWrite(latchPin, LOW);
    digitalWrite(latchPin, HIGH);

    // step 2: read the input data
    for (int i = 0; i < longCascadeSize; i++) {
        // use bit by bit reading
        // (I'm not sure if this is the most efficient way,
        // but shiftIn() loses the very first bit for some reason.)
        _buffer[i] = 0;
        if (i < shortCascadeSize){
            // short cascade's data is in the same buffer, but after the long cascade's data
            _buffer[longCascadeSize + i] = 0;
        }
        // we always read all 8 bits from each chip
        for (int j = 0; j < 8; j++) {
            // *** there will be 2 parallel cascades.
            // read data from the long cascade
            _buffer[i] = _buffer[i] & ~((uint8_t)1 << j) | (digitalRead(dataPinR) << j);
            // read data from the short cascade
            if (i < shortCascadeSize){
                _buffer[longCascadeSize + i] = _buffer[longCascadeSize + i] & ~((uint8_t)1 << j) | (digitalRead(dataPinL) << j);
            }
            // get the next bit
            digitalWrite(clockPin, HIGH);
            digitalWrite(clockPin, LOW);
        }
    }
}

///////////////////////////////////////////////////////////
// Process data from _buffer
void TesKeyboard::processNewData(void){
    // check if there're differences
    bool got_changes = false;
    for (int i = 0; i < TES_KEYBOARD_CASCADE_SIZE; i++){
        if (_buffer[i] != _previous_buffer[i]){
            got_changes = true;
            break;
        }
    }
    if ( !got_changes ){
        // don't lose time for bit-by-bit check
        return;
    }
    // Ok, go with a full-size processing.

    // *** we check bit by bit
    // this loop is global; i.e. for all buttons: music & control
    for (int currentButton = 0; currentButton < totalNumberOfButtons; currentButton++){
        byte oldValue = bitmap_get(_previous_buffer, currentButton);
        byte newValue = bitmap_get(_buffer, currentButton);
        if (oldValue != newValue){
            // got a change

	    // Check if this is a Free Bass sensor (this is a special case)
            if (currentButton == freeBassSensorIndex){
		// Free Bass mode has changed.
                // prepare an event record
                tesEvent    event;
                event.eventType = tesEvFreeBass;
                event.freeBassOn = (newValue == getPressedLevel(currentButton));    // "pressed" means "turned on" in this context
                theQueue.pushEvent(&event);
	    }
            else if ( isCtlButton(currentButton) ){ // check if this is a control button
                // this is all we need to do for coltrol buttons here
                _ctlButtonController.processCtlButton(getCtlButtonId(currentButton), (newValue == getPressedLevel(currentButton)));
            }
            else {
                // this is a music button.
                // prepare an event record
                tesEvent    event;
                event.eventType = tesEvButton;
                event.buttonEvent = (newValue != getPressedLevel(currentButton)) ? tesBeReleased : tesBePressed;
                event.buttonId = getMusicButtonId(currentButton);
                theQueue.pushEvent(&event);
            }
        }
    }
}

// *** Global button map
// index = a global (raw) button index (including unused pins)
// value = either Music or Control button ID
//
// Control buttons are numbered as 0 .. TES_NUMBER_OF_CONTROL_BUTTONS-1
// Music buttons are numbered as 0..106 (52 keys in the Left Keyboard and 55 keys in the right keyboard)
//
// Unused global indices are translated into 0xFC.
//

const uint8_t   globalButtonMap[] = {
// Right keyboard 1st & 2nd rows:
    52,  54, 55,  57, 58,  60, 61,  63, 64,  66, 67,  69, 70,  72, 73,  75, 76,  78, 79,  81, 82,  84, 85,  87, 88,  90, 91,
    93, 94,  96, 97,  99, 100,  102, 103,  105, 106,  0xFC, 0xFC, 0xFC,
// Right keyboard 3rd row:
    53, 56, 59, 62, 65, 68, 71, 74, 77, 80, 83, 86, 89, 92, 95, 98, 101, 104,  0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC,
// Control panel:
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0xFC, 0xFC,
// Left keyboard:
    0xFC,  // <- Free Bass sensor
    51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,
    20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0xFC, 0xFC, 0xFC
};

///////////////////////////////////////////////////////////
// Translates the specified global button ID into a Control Button ID
uint8_t TesKeyboard::getCtlButtonId(uint8_t  globalButtonId){
    // calculate the ID of a control button
    uint8_t id = globalButtonMap[globalButtonId];
    // check if we get an unused key somehow
    if ( id == 0xFC ){
        // something went wrong
        SWER(swerKeyboard01);   // wrong control button id
    }
    return id;
}

///////////////////////////////////////////////////////////
// Translates the specified global button ID into a Music Button ID
// i.e. into ID of a button from left/right music keyboard (not from the control panel)
uint8_t TesKeyboard::getMusicButtonId(uint8_t  globalButtonId){
    // calculate the ID of a music button
    uint8_t id = globalButtonMap[globalButtonId];
    // check if we get an unused key somehow
    if ( id == 0xFC ){
        // something went wrong
        SWER(swerKeyboard02);   // wrong music button id
    }
    return id;
}

