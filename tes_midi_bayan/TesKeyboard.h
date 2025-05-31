
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
#include "TesCtlButtonController.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//                                 Public part
///////////////////////////////////////////////////////////////////////////////////////////////////

#define TES_NUMBER_OF_CONTROL_BUTTONS 22        // as per 1-Jul-2024
#define TES_NUMBER_OF_ALL_MUSIC_BUTTONS   107   // that's how many music buttons we have

#define TES_KEYBOARD_CASCADE_SIZE 18            // 5 right-front + 3 right-back + 3 control panel + 7 left

// names of control buttons
#define ctlButtonUp             0      // "repeat"
#define ctlButtonDown           1      // "repeat"
#define ctlButtonLeft           2      // "repeat"
#define ctlButtonRight          3      // "repeat"
#define ctlButtonOk             4       // "hold"
#define ctlButtonMenu           5       // "hold"
#define ctlButtonSynthSync      6
#define ctlButtonPressure       7
#define ctlButtonPreset1        8       // "hold"
#define ctlButtonPreset2        9       // "hold"
#define ctlButtonPreset3        10      // "hold"
#define ctlButtonPreset4        11      // "hold"
#define ctlButtonPreset5        12      // "hold"
#define ctlButtonPreset6        13      // "hold"
#define ctlButtonF1             14      // "hold"
#define ctlButtonF2             15      // "hold"
#define ctlButtonF3             16      // "hold"
#define ctlButtonF4             17      // "hold"
#define ctlButtonDrumsControl   18      // "hold"
#define ctlButtonDrumsStyle     19      // "hold"
#define ctlButtonTempoMinus     20      // "repeat"
#define ctlButtonTempoPlus      21      // "repeat"

// now the class itself
class TesKeyboard {
public:
    TesKeyboard(void);

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
    ////////// for control buttons /////////
    TesCtlButtonController  _ctlButtonController;
    struct {
        uint8_t     _delayed_init_done: 1;
    };

    ///////////////////////////////////////////////////////////////////////
    // Returns a "pressed" level for the specified button
    uint8_t getPressedLevel(uint8_t buttonId);

    //////////////////////////////////////////////////////////////////////
    // Returns True if the specified button is a control button
    //
    bool isCtlButton(uint8_t buttonIndex);

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

// There's the only Keyboard
extern  TesKeyboard     theKeyboard;

#endif

