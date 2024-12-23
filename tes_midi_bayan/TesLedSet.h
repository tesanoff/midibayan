#ifndef _TES_LED_SET_LIBRARY
#define _TES_LED_SET_LIBRARY

#include <Arduino.h>
#include "common.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// constants which shoudl be used to refer to particular LEDs

#define ledPreset1  0
#define ledPreset2  1
#define ledPreset3  2
#define ledPreset4  3
#define ledPreset5  4
#define ledPreset6  5
#define ledDrums    6
#define ledSystem   7
#define ledMidiOut  8

//
///////////////////////////////////////////////////////////////////////////////////////////////////

// the maximum number of chips in a cascade
#define _TES_LED_SET_NUMBER_OF_CHIPS 2

// parameters of LED modes
#define _TES_LED_PANEL_BLINK_PERIOD 100
#define _TES_LED_PANEL_SLOW_BLINK_PERIOD 1000
#define _TES_LED_PANEL_FLASH_DURATION 15

// 2 bits are allocated for LED mode
enum {teslpNormal = 0, teslpBlink, teslpSlowBlink, teslpFlash};

// calculate the number of bytes for storing LED modes
#define _TES_LED_PANEL_LED_MODE_BYTES  (_TES_LED_SET_NUMBER_OF_CHIPS * 8 / 4)

// This is the TesLedSet class
class TesLedSet {
public:
    // constructor
    TesLedSet(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t num_of_LEDs );

    ////////////////////////////////////////////////////////////
    // Switches all LEDs either to OFF or to ON - depending on the parameter
    void reset(uint8_t On = false); 

    ////////////////////////////////////////////////////////////
    // Sets the specified LED
    void setLED(uint8_t led_index, uint8_t value);

    ////////////////////////////////////////////////////////////
    // Returns the status of the specified LED
    uint8_t getLED(uint8_t led_index);

    ///////////////////////////////////////////////////////////
    // Starts blink mode for the specified LED
    // (use resetLED() if you need to stop blinking)
    void startBlink(uint8_t led_index, uint8_t mode = teslpBlink);

    ////////////////////////////////////////////////////////////
    // Resets the specified LED:
    // - stops blinking
    // - turns OFF
    void resetLED(uint8_t led_index);

    ///////////////////////////////////////////////////////////
    // Makes a single flash.
    // The specified LED goes off after the flash even if it was blinking before.
    // (The upper-level code should take care of switching between LED modes properly).
    void flashLED(uint8_t led_index);

    ///////////////////////////////////////////////////////////
    // tick method
    void tick(void);

    ///////////////////////////////////////////////////////////
    // For debugging.
    // Sets the LEDs according to bits in the parameters
    void showDebugCode(uint8_t lsb, uint8_t msb);

private:
    // for operations with registers
    uint8_t _latchPin;
    uint8_t _clockPin;
    uint8_t _dataPin;
    // the number of actual LEDs (can be less than 8 * number_of_chips)
    uint8_t _num_of_LEDs;
    // the number of chips in the cascade
    uint8_t _n_of_chips;
    // local buffer
    // stores the full set of data to be transmitted to the cascade during each change
    uint8_t _buffer[ _TES_LED_SET_NUMBER_OF_CHIPS ];     // no choice: we have to have 1 byte for each chip
    // stores the current mode for each LED
    // (possible states are defined as enum above)
    // IMPORTANT: 2 bits are allocated for a sligle status, i.e. each byte can store statuses for 4 LEDs.
    uint8_t _led_mode[ _TES_LED_PANEL_LED_MODE_BYTES ];
    timer_t _blink_timer[ _TES_LED_SET_NUMBER_OF_CHIPS * 8 ];
    // this flag shows that an update of the cascade is required
    uint8_t _cascade_update_is_required;

    // transmits the buffer to the cascade
    void updateCascade(void);

    ///////////////////////////////////////////////////////////
    // Returns the mode of the specified LED
    uint8_t getLEDMode(uint8_t led_index);

    ///////////////////////////////////////////////////////////
    // Sets the mode of the specified LED
    void setLEDMode(uint8_t led_index, uint8_t value);

    ////////////////////////////////////////////////////////////
    // Returns true if the LED is blinking
    bool ledIsBlinking(uint8_t led_index);

    ////////////////////////////////////////////////////////////
    // Returns true if the LED is flashing
    bool ledIsFlashing(uint8_t led_index);
};

#endif
