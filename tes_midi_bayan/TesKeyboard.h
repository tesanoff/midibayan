
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
#ifndef TES_KEYBOARD_H
#define TES_KEYBOARD_H
#include "TesEventQueue.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                 Public part
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TES_NUMBER_OF_CONTROL_BUTTONS 22        // as per 1-Jul-2024
#define TES_NUMBER_OF_ALL_MUSIC_BUTTONS   107   // that's how many music buttons we have

#define TES_KEYBOARD_CASCADE_SIZE 18            // 5 right-front + 3 right-back + 3 control panel + 7 left

// names of control buttons
#define ctlButtonUp             0
#define ctlButtonDown           1
#define ctlButtonLeft           2
#define ctlButtonRight          3
#define ctlButtonOk             4
#define ctlButtonMenu           5
#define ctlButtonSynthSync      6
#define ctlButtonPressure       7
#define ctlButtonPreset1        8
#define ctlButtonPreset2        9
#define ctlButtonPreset3        10
#define ctlButtonPreset4        11
#define ctlButtonPreset5        12
#define ctlButtonPreset6        13
#define ctlButtonF1             14
#define ctlButtonF2             15
#define ctlButtonF3             16
#define ctlButtonF4             17
#define ctlButtonDrumsControl   18
#define ctlButtonDrumsStyle     19
#define ctlButtonTempoMinus     20
#define ctlButtonTempoPlus      21

// now the class itself
class TesKeyboard {
public:
    TesKeyboard(TesEventQueue *queue);

    ///////////////////////////////////////////////////////////
    // Initializes the keyboard
    void init(void);

    ///////////////////////////////////////////////////////////
    // tick method
    void tick(void);

private:
    // for operations with registers
    uint8_t _buffer [ TES_KEYBOARD_CASCADE_SIZE ];  // The input buffer
    uint8_t _previous_buffer [ TES_KEYBOARD_CASCADE_SIZE ]; // This is for comparing old and new values
    TesEventQueue   * _eventQueue;                      // the queue to work with
    ////////// for control buttons /////////
    uint16_t    _hold_timer[TES_NUMBER_OF_CONTROL_BUTTONS];         // The same timers are used for both "Hold" and "Fast Repeat" functions
    uint8_t     _ctlButtonState[ (TES_NUMBER_OF_CONTROL_BUTTONS-1) / 4 + 1 ]; // each byte stores states for 4 buttons
                                                                              // 2 bits per button
    uint8_t     _ctlButtonFastRepeatState[ (TES_NUMBER_OF_CONTROL_BUTTONS-1) / 8 + 1 ];     // this bitmap stores "virtual" pressed/released statuses
                                                                                            // 1 bit per button

    ///////////////////////////////////////////////////////////////////////
    // Returns a "pressed" level for the specified button
    uint8_t getPressedLevel(uint8_t buttonId);

    //////////////////////////////////////////////////////////////////////
    // Returns a state of the specified control button
    //
    uint8_t getCtlButtonState(uint8_t ctlButtonIndex);

    //////////////////////////////////////////////////////////////////////
    // Sets a state of the specified control button
    //
    void setCtlButtonState(uint8_t ctlButtonIndex, uint8_t value);

    //////////////////////////////////////////////////////////////////////
    // Returns a virtual "released/pressed" status of the specified control button
    //
    uint8_t getCtlButtonFastRepeatState(uint8_t ctlButtonIndex);

    //////////////////////////////////////////////////////////////////////
    // Sets a virtual "released/pressed" status of the specified control button
    //
    void setCtlButtonFastRepeatState(uint8_t ctlButtonIndex, uint8_t value);

    //////////////////////////////////////////////////////////////////////
    // Returns True if the specified button is a control button
    //
    bool isCtlButton(uint8_t buttonIndex);

    //////////////////////////////////////////////////////////////////////
    // Returns True if the "Hold" function is enabled for 
    // the specified control button
    //
    bool holdIsEnabled(uint8_t ctlButtonIndex);

    //////////////////////////////////////////////////////////////////////
    // Returns True if the "Fast Repeat" function is enabled for 
    // the specified control button
    //
    bool fastRepeatIsEnabled(uint8_t ctlButtonIndex);

    ///////////////////////////////////////////////////////////////////////
    // Reads all data from the cascade(s)
    // INFO: It reads a 2-registers cascade in 0,2655 ms
    //
    void readCascade(void);

    ///////////////////////////////////////////////////////////
    // Process data from _buffer
    void processNewData(void);

    ///////////////////////////////////////////////////////////
    // Translates the specified global button ID into a Control Button ID
    uint8_t getCtlButtonId(uint8_t  globalButtonId);

    ///////////////////////////////////////////////////////////
    // Translates the specified global button ID into a Music Button ID
    // i.e. into ID of a button from left/right music keyboard (not from the control panel)
    uint8_t getMusicButtonId(uint8_t  globalButtonId);
};

#endif

