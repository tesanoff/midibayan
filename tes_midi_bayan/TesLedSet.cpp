#include "TesLedSet.h"
#include "swer.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesLedSet::TesLedSet(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t num_of_LEDs ){
    // save the parameters
    _latchPin = latchPin;
    _clockPin = clockPin;
    _dataPin = dataPin;
    _num_of_LEDs = num_of_LEDs;

    // this is the actual number of chips that will be processed
    _n_of_chips = ( _num_of_LEDs - 1 ) / 8 + 1;

    // reset the cascade update flag
    _cascade_update_is_required = false;

    // clear the buffer
    for (int i = 0; i < _TES_LED_SET_NUMBER_OF_CHIPS; i++){
        _buffer[i] = 0;
    }

    // initialize auxiliary data specific for each LED
    for (int i = 0; i < _TES_LED_PANEL_LED_MODE_BYTES; i++){
        _led_mode[i] = 0;   // the default LED mode is Normal = 0; so, we may do this
    }

    //set the pins up
    pinMode(_latchPin, OUTPUT);
    pinMode(_clockPin, OUTPUT);
    pinMode(_dataPin, OUTPUT);

    // update the cascade the first time
    updateCascade();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Switches all LEDs either to OFF or to ON - depending on the parameter
void TesLedSet::reset(uint8_t On) {
    // Reset the LEDs
    // Put them to the specified state
    for (int i = 0; i < _n_of_chips ; i++) {
        _buffer[i] = On ? 0xFF : 0;
    }
    // return all LEDs to the normal mode
    for (int i = 0; i < _num_of_LEDs; i++){
        setLEDMode(i, teslpNormal);
    }

    // raise the update flag
    // (the actual update is done in tick() only)
    _cascade_update_is_required = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Sets the specified LED
void TesLedSet::setLED(uint8_t led_index, uint8_t value){
    // get rid of extra bits (if any)
    value = value ? 1 : 0;
    // now, we're sure that only the lowest bit is used

    //calculate the chip index
    uint8_t chipIndex = led_index / 8;
    // calculate the bit index
    int bitIndex = led_index % 8;
    // set the bit
    _buffer[chipIndex] = _buffer[chipIndex] & ~((uint8_t)1 << bitIndex) | (value << bitIndex);

    // raise the update flag
    // (the actual update is done in tick() only)
    _cascade_update_is_required = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the status of the specified LED
uint8_t TesLedSet::getLED(uint8_t led_index){
    //calculate the chip index
    uint8_t chipIndex = led_index / 8;
    // calculate the bit index
    int bitIndex = led_index % 8;

    // return the value
    return (_buffer[chipIndex] & ((uint8_t)1 << bitIndex)) >> bitIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Starts blink mode for the specified LED
// (use resetLED() if you need to stop blinking)
void TesLedSet::startBlink(uint8_t led_index, uint8_t mode){
    // chech if the blink mode is valid
    if ( (mode != teslpBlink) && (mode != teslpSlowBlink) ){
        // invalid blink mode
        SWER(swerLedSet01);
    }
    // set the blink mode
    setLEDMode(led_index, mode);
    // start the blink timer
    _blink_timer[led_index] = millis();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Resets the specified LED:
// - stops blinking
// - turns OFF
void TesLedSet::resetLED(uint8_t led_index){
    // set the normal mode if resetLED() was called while the LED was blinking or flashing
    setLEDMode(led_index, teslpNormal);

    // just call setLED() for the rest
    setLED(led_index, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Makes a single flash.
// The specified LED goes off after the flash even if it was blinking before.
// (The upper-level code should take care of switching between LED modes properly).
void TesLedSet::flashLED(uint8_t led_index){
    // As the current state of the LED is ignored, we can use the timer 
    // used for the "blink" mode.

    // set the flash mode
    setLEDMode(led_index, teslpFlash);
    // start the timer
    _blink_timer[led_index] = millis();
    // and turn the LED on
    setLED(led_index, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// tick method
void TesLedSet::tick(void){
    // take the current timer value
    timer_t millis_snapshot = millis();

    for (int i = 0; i < _num_of_LEDs; i++){
        if ( ledIsBlinking(i) ){
            // blink period may be different
            uint32_t    blink_period = (getLEDMode(i) == teslpBlink) ? _TES_LED_PANEL_BLINK_PERIOD : _TES_LED_PANEL_SLOW_BLINK_PERIOD;
            if ( (millis_snapshot - _blink_timer[i]) > blink_period ){
                // process blinking LEDs

                // toggle the LED and restart the timer
                setLED(i, (getLED(i) ? 0 : 1) );
                _blink_timer[i] = millis_snapshot;
            }
        }
        else if ( ledIsFlashing(i) && ((millis_snapshot - _blink_timer[i]) > _TES_LED_PANEL_FLASH_DURATION) ){
            // process flashing LEDs

            // reset the LED, because the flash duration is over now
            resetLED(i);
        }
    }

    if ( _cascade_update_is_required ){
        // actual updates should be done only here
        updateCascade();
    }
}

///////////////////////////////////////////////////////////
// For debugging.
// Sets the LEDs according to bits in the parameters
void TesLedSet::showDebugCode(uint8_t lsb, uint8_t msb){
    _buffer[0] = lsb;
    _buffer[1] = msb;
    updateCascade();
}

//////////////////////////////////////////
////////// private members
//////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////
// transmits the buffer to the cascade
void TesLedSet::updateCascade(void)
{
    digitalWrite(_latchPin, LOW);
    for (int i = _n_of_chips - 1; i >= 0 ; i--) {
        shiftOut(_dataPin, _clockPin, LSBFIRST, _buffer[i] );
    }
    digitalWrite(_latchPin, HIGH);
    // reset the flag
    _cascade_update_is_required = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the mode of the specified LED
uint8_t TesLedSet::getLEDMode(uint8_t led_index){
    // calculate shift & index
    uint8_t index = led_index / 4;
    uint8_t shift = (led_index % 4 ) * 2;
    // extract & return the value
    return (_led_mode[index] & ((uint8_t)0b00000011 << shift)) >> shift;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Sets the mode of the specified LED
void TesLedSet::setLEDMode(uint8_t led_index, uint8_t value){
    // clear unused bits in 'value' just in case...
    value &= (uint8_t)0b00000011;
    // calculate shift & index
    uint8_t index = led_index / 4;
    uint8_t shift = (led_index % 4 ) * 2;
    _led_mode[index] = _led_mode[index] & ~((uint8_t)0b00000011 << shift) | (value << shift);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the LED is blinking
bool TesLedSet::ledIsBlinking(uint8_t led_index){
    uint8_t led_mode = getLEDMode(led_index);
    return ((led_mode == teslpBlink) || (led_mode == teslpSlowBlink));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Returns true if the LED is flashing
bool TesLedSet::ledIsFlashing(uint8_t led_index){
    return getLEDMode(led_index) == teslpFlash;
}


