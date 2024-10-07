#ifndef _TES_MIDI_GUI_H
#define _TES_MIDI_GUI_H

#include <GyverOLED.h>
#include "TesLedSet.h"

// forward declaration
class TesMIDIController;

/////////////////////////////////////////////////////////////
// Aux stuff
struct  TesEditorStatus{
    struct {
        uint8_t selectorX: 2;           // specifies the active Column (relative to the editor area)
        uint8_t selectorY: 3;           // specifies the active Row (relative to the editor area)
        uint8_t topIndex: 4;            // the index of a row which is shown in the top position
        uint8_t columnIndex: 2;         // the number of the selected column
        uint8_t editMode: 1;            // true - the edit mode is active, and arrows change the value
        uint8_t screenMode: 2;          // 0 - the main screen;
                                        // 1 - MIDI parameters editor
                                        // 2 - system paramenetr editor
                                        // 3 - drums parameters editor
        uint8_t sysParSelector: 3;      // specifies the active system parameter
        uint8_t drumsParSelector: 3;    // specifies the active drums parameter
    };
};

struct  TesFKeyProfile{
    TesEditorStatus     editorStatus;

    TesFKeyProfile(void){
        // default settings
        editorStatus.selectorX          = 0;
        editorStatus.selectorY          = 0;
        editorStatus.topIndex           = 0;
        editorStatus.columnIndex        = 0;
        editorStatus.editMode           = true;
        editorStatus.screenMode         = 1;
        editorStatus.sysParSelector     = 0;
        editorStatus.drumsParSelector   = 0;
    }
};

// this is for F-key State Machine
#define fKeyWriteConfirmationPeriod     3000    // 3 sec

// this is for Preset State Machine
#define presetWriteConfirmationPeriod   3000    // 3 sec

enum {psNormal = 0, psWriteConfirmation};

struct  TesPresetStateMachine{
    struct {
        uint8_t state: 2;
        uint8_t writePreset: 3;         // the preset being processed
    };
    uint16_t    writeTimer;             // starts when the user holds the preset button

    TesPresetStateMachine(void){
        state = psNormal;
        writePreset = 0;
    }
};


struct  TesFKeyStateMachine{
    struct {
        uint8_t state: 2;
        uint8_t writeFKey: 2;           // the F-key being processed (F1 .. F4)
    };
    uint16_t    writeTimer;             // starts when the user holds the preset button

    TesFKeyStateMachine(void){
        state       = psNormal;
        writeFKey   = 0;
    }
};

// list of drums parameter IDs
enum {dpDrumsVolume = 0, dpDrumsetNumber, dpSoundForBass, dpSoundForBassVelocity, dpSoundForChord, dpSoundForChordVelocity};

/////////////////////////////////////////
// aux flags
struct  TesUIFlags {
    uint8_t display_update_is_required: 1;  // true - when the display require updating
    uint8_t menu_hold_detected: 1;          // true - when the "Menu" key is held
    uint8_t drums_control_hold_detected: 1; // true - when the "Drums" key is held

    TesUIFlags(void){
        display_update_is_required  = false;
        menu_hold_detected          = false;
        drums_control_hold_detected = false;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// a helper class - to isolate drawing functionality from the main MIDI controller class
// It also processes control keyboard events.
//
class TesMidiUI {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // constructor
    TesMidiUI(GyverOLED<SSH1106_128x64> * oled, TesMIDIController * mc, TesLedSet * led);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Initializes the instance.
    // Should be called from setup()
    void    init(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // standard tick() method
    void tick(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // This method processes control button events
    void processCtlButtonEvent(tesEvent *event);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // This method processes FreeBass events
    void processFreeBassEvent(tesEvent *event);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Changes the state of the Drum Machine
    void setDrumMachineState(uint8_t new_state);

private:
    GyverOLED<SSH1106_128x64>   * _oled;    // OLED display instance
    TesMIDIController           * _mc;      // MIDI Controller instance
    TesUIFlags                  _flags;     // aux flags
    uint16_t                    _display_update_timer;      // we update display by a timer, not in each work cycle
    TesLedSet                   * _led_set;                 // the LED panel to be used
    TesEditorStatus             _editor_status;
    uint16_t                    _inactivity_timer;          // if no user activity - each editor switches back to Main Screen
    TesPresetStateMachine       _preset_sm;                 // all data required to work with presets
    TesFKeyStateMachine         _fkey_sm;                   // all data required to work with F-keys

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the top line of the screen (shared between Main screen & MIDI Param Editor
    void drawKbdHeader(void);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the main screen
    void drawMainScreen(void);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Sets the cursor for text output
    void setTextCursor(uint8_t X, uint8_t Y);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  prints a number according to the specified format string located in PROGMEM
    void printFormatted(PGM_P format, uint8_t value);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the active screen
    void drawActiveScreen(void);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the MIDI parameters editor screen
    void drawMidiParamEditorScreen(void);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the System parameters editor screen
    void drawSystemParamEditorScreen(void);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Draws the Drums parameters editor screen
    void drawDrumsParamEditorScreen(void);
    
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  control button event handler to Midi Param Editor more
    void processCtlButtonEventMidiParamEditor(tesEvent *event);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  control button event handler to System Param Editor more
    void processCtlButtonEventSystemParamEditor(tesEvent *event);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  control button event handler to Drums Param Editor more
    void processCtlButtonEventDrumsParamEditor(tesEvent *event);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //  Activates the specified preset
    void activatePreset(uint8_t preset_id);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Returns the next/previous value for the bank ID
    uint8_t getNextAtempBankId(uint8_t kbd_id, bool forward = true);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Returns the next/previous value for the drumset ID
    // (relative to the current value in _mc->_settings.preset.drumsetNumber)
    uint8_t getNextAtempDrumsetId(bool forward);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Adjusts bank ID to be is allowed for the specified Program and to be the closest to the current
    // bank ID.
    // - Returns True if the bank ID was changed.
    bool adjustBankId(uint8_t kbd_id);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Starts indicating a not saved preset
    void indicatePresetChange(void);
};

#endif

