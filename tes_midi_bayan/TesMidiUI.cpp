#include <Arduino.h>
#include "TesMidiController.h"
#include "TesEventQueue.h"
#include "TesKeyboard.h"
#include "TesMidiUI.h"
#include "swer.h"
#include "midi_note_names.h"
#include "prodx.h"
#include "atemp_hw.h"


// the OLED display
OledDisplay oled;


// period for updating the display (it takes too much time to update it in each work cycle)
#define TES_DISPLAY_UPDATE_PERIOD   (1000 / 3)

#define screenModeMain              0
#define screenModeMidiParamEdit     1
#define screenModeSysParamEdit      2
#define screenModeDrumsEdit         3
#define screenModeInstrumentEdit    4
#define screenModeMelodyEdit        5
#define screenModeGyroEdit          6
#define screenModeEasterEgg         7

#define EASTER_EGG_PERIOD   10000   // in milliseconds

////////////////////////////////////////////////////////////////////////////////////////////
/// aux functions
static bool isSigned(uint8_t parameter_id){
    // it's a very simple logic for now
    return (parameter_id == idxOctTranspose) ? true : false;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesMidiUI::TesMidiUI(TesMIDIController * mc){
    _mc = mc;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the instance.
// Should be called from setup()
void    TesMidiUI::init(void){
    // initialize the display
    oled.init();

    // draw the active screen
    drawActiveScreen();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// standard tick() method
void TesMidiUI::tick(void){
    timer_t     millis_snapshot = millis();

    // check the display update timer
    if (millis_snapshot - _display_update_timer > TES_DISPLAY_UPDATE_PERIOD){
        // update the display if required
        if ( _flags.display_update_is_required ){
            oled.update();    // partial update doesn't work for SSH1106 yet :(
            // reset the display update request
            _flags.display_update_is_required = false;
        }
        // restart the timer
        _display_update_timer = millis();
    }

    // check user activity
    if ((_editor_status.screenMode != screenModeMain) && (_editor_status.screenMode != screenModeEasterEgg)){
        if ((_mc->_settings.global.inactivityPeriod != 0)
                && (millis_snapshot - _inactivity_timer > (_mc->_settings.global.inactivityPeriod * 1000))){
            // switch the screen back to Main Screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
        }
    }

    // check the Easter Egg mode
    if (_editor_status.screenMode == screenModeEasterEgg){
        if (millis_snapshot - _inactivity_timer > EASTER_EGG_PERIOD){
            // switch the screen back to Main Screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
        }
    }

    // check preset write confirmation timer
    if (_preset_sm.state == psWriteConfirmation){
        if (millis_snapshot - _preset_sm.writeTimer > presetWriteConfirmationPeriod){
            // the user has confirmed writing of the preset

            // return back to the normal mode
            _preset_sm.state = psNormal;
            // write the preset to EEPROM
            _mc->writePresetToEEPROM(_preset_sm.writePreset);
            // stop blinking
            ledSet.resetLED(_preset_sm.writePreset + ledPreset1);
            ledSet.setLED(_preset_sm.writePreset + ledPreset1, (_preset_sm.writePreset == _mc->_settings.global.activePreset) );
        }
    }

    // check F-key write confirmation timer
    if (_fkey_sm.state == psWriteConfirmation){
        if (millis_snapshot - _fkey_sm.writeTimer > fKeyWriteConfirmationPeriod){
            // the user has confirmed writing of the F-key

            // return back to the normal mode
            _fkey_sm.state = psNormal;
            // save the F-key settings
            _mc->_settings.global.fkeyProfile[_fkey_sm.writeFKey][_fkey_sm.writeFKeyPage].editorStatus = _editor_status;
            // notify the controller about global settings change
            _mc->globalSettingsChangeNotification();

            // stop blinking
            ledSet.resetLED(ledSystem);
            // and flash once
            ledSet.flashLED(ledSystem);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Processes events from control buttons.
void TesMidiUI::processCtlButtonEvent(tesEvent *event){
    // User Activity detected
    _inactivity_timer = millis();   // reset the inactivity timer

    // call specific Button Event handlers first (to see if they process and clear the event)
    switch ( _editor_status.screenMode ) {
    case screenModeMain:
        {
            // "released" means a normal release; "step" means "autorepeated release"
            if ((event->buttonEvent == tesBeReleased)
                    || (event->buttonEvent == tesBeStep)){
                // key handlers specific for the main screen
                switch(event->buttonId){
                case ctlButtonTempoMinus:
                case ctlButtonTempoPlus:
                    // we change a tempo in the Standby or Playing modes only
                    if( (_mc->_var.drumsMode == dmStandby) || (_mc->_var.drumsMode == dmPlaying)){
                        _mc->_auto_drums.setTempo( _mc->_auto_drums.getTempo()
                                + ((event->buttonId == ctlButtonTempoPlus) ? 1 : -1));
                        drawActiveScreen();
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonMenu:
                    // *** switching to the Kbd or System Parameters Edit mode is possible from the main screen only
                    _editor_status.screenMode = screenModeMidiParamEdit;
                    // *** ATTENTION! each time we change screens - we turn off the edit mode
                    _editor_status.editMode = false;
                    _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    drawActiveScreen();
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonDrumsControl:
                    switch(_mc->_var.drumsMode){
                    case dmOff:
                        setDrumMachineState(dmManual);
                        break;
                    case dmManual:
                        if(_mc->_auto_drums.isInitialized()){
                            setDrumMachineState(dmStandby);
                        }
                        else{
                            setDrumMachineState(dmOff);
                        }
                        break;
                    case dmStandby:
                        setDrumMachineState(dmPlaying);
                        break;
                    case dmPlaying:
                        setDrumMachineState(dmOff);
                        break;
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    drawActiveScreen();
                    break;
                case ctlButtonDrumsStyle:
                    {
                        bool was_playing = _mc->_auto_drums.isPlaying();
                        if ( was_playing ){
                            _mc->_auto_drums.stop();
                        }
                        _mc->_auto_drums.setNextMelodyId();
                        if ( was_playing ){
                            _mc->_auto_drums.start();
                        }
                    }
                    drawActiveScreen();
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonDown:
                    PRINT_1("'Drums Fill' button was pressed");
                    if(_mc->_var.drumsMode == dmPlaying){
                        PRINT_1("** requesting drums fill");
                        _mc->_auto_drums.requestDrumsFill();
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                }
            }
            else if (event->buttonEvent == tesBeHold){
                switch(event->buttonId){
                case ctlButtonMenu:
                    // long press starts system parameters editor
                    _editor_status.screenMode = screenModeSysParamEdit;
                    // *** ATTENTION! each time we change screens - we turn off the edit mode
                    _editor_status.editMode = false;
                    _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    drawActiveScreen();
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonDrumsControl:
                    // switch to the Manual mode
                    setDrumMachineState(dmManual);
                    // start drums parameters editing
                    _editor_status.screenMode = screenModeDrumsEdit;
                    // *** ATTENTION! each time we change screens - we turn off the edit mode
                    _editor_status.editMode = false;
                    _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    drawActiveScreen();
                    break;
                case ctlButtonDrumsStyle:
                    // Calling the melody selector screen
                    _editor_status.screenMode = screenModeMelodyEdit;
                    // Adjust a pointer to the current melody
                    initMelodySelector();
                    // *** ATTENTION! each time we change screens - we turn off the edit mode
                    _editor_status.editMode = false;
                    _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    drawActiveScreen();
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                }
            }
        }
        break;
    case screenModeMidiParamEdit:
        processCtlButtonEventMidiParamEditor( event );
        break;
    case screenModeSysParamEdit:
        processCtlButtonEventSystemParamEditor( event );
        break;
    case screenModeDrumsEdit:
        processCtlButtonEventDrumsParamEditor( event );
        break;
    case screenModeInstrumentEdit:
        processCtlButtonEventInstrumentEditor( event );
        break;
    case screenModeMelodyEdit:
        processCtlButtonEventMelodySelector( event );
        break;
    case screenModeGyroEdit:
        processCtlButtonEventGyroEditor( event );
        break;
    case screenModeEasterEgg:
        // we don't process buttons here
        // ... and clear the "control button" event
        event->eventType = tesEmpty;
        break;
    default:
        SWER(swerGUI06);
    }
    // check if the event was cleared (fully processed) by specific handlers
    if (event->eventType == tesEmpty){
        // nothing to do with the event anomore; we're done
        return;
    }

    ///////////////////////////////////////////////////////////////////////////////////
    // *** generic events are processed here

    if ( event->buttonEvent == tesBeReleased ){
        switch (event->buttonId) {
            case ctlButtonPressure: {
                // toggle Pressure Sensor
                _mc->togglePressureSensor();
                indicatePresetChange();
                drawActiveScreen();
                // clear the event to indicate it has been fully processed
                event->eventType = tesEmpty;
            }
            break;
            case ctlButtonSynthSync: {
                // send parameters to active keyboards
                _mc->sendCurrentPresetToSynthesizer( _mc->_var.freeBassOn ? 0b00001001 : 0b00000111, false );   // sendng all parameters
                // clear the event to indicate it has been fully processed
                event->eventType = tesEmpty;
            }
            break;
            case ctlButtonPreset1:
            case ctlButtonPreset2:
            case ctlButtonPreset3:
            case ctlButtonPreset4:
            case ctlButtonPreset5:
            case ctlButtonPreset6: {
                uint8_t preset_id = event->buttonId - ctlButtonPreset1;
                switch ( _preset_sm.state ){
                    case psNormal: {
                        activatePreset( preset_id );
                    }
                    break;
                    case psWriteConfirmation: {
                        // check if this button is for the preset being processed
                        if (preset_id == _preset_sm.writePreset){
                            // cancel the write confirmation mode, return to the normal mode
                            _preset_sm.state = psNormal;
                            // stop blinking
                            ledSet.resetLED(preset_id + ledPreset1);
                            if (_mc->_var.currentPresetNotSaved && (_preset_sm.writePreset == _mc->_settings.global.activePreset)){
                                // the current preset is still not saved, and the LED should return to the slow blinking mode
                                ledSet.startBlink(preset_id + ledPreset1, teslpSlowBlink);
                            }
                            else {
                                ledSet.setLED(preset_id + ledPreset1, (_preset_sm.writePreset == _mc->_settings.global.activePreset) );
                            }
                        }
                    }
                    break;
                    default:
                        SWER(swerGUI01);    // incorrect state
                }
                // clear the event to indicate it has been fully processed
                event->eventType = tesEmpty;
            }
            break;
        }
    }
    else if (event->buttonEvent == tesBeHoldReleased) {
        switch (event->buttonId) {
            case ctlButtonF1:
            case ctlButtonF2:
            case ctlButtonF3:
            case ctlButtonF4: {
                uint8_t fkey_id = event->buttonId - ctlButtonF1;
                switch ( _fkey_sm.state ){
                    case psNormal: {
                        // nothing to do (psNormal was set in tick())
                    }
                    break;
                    case psWriteConfirmation: {
                        // check if this button is for the F-key being processed
                        if (fkey_id == _fkey_sm.writeFKey){
                            // cancel the write confirmation mode, return to the normal mode
                            _fkey_sm.state = psNormal;
                            // stop blinking
                            ledSet.resetLED(ledSystem);
                        }
                    }
                    break;
                    default:
                        SWER(swerGUI07);    // incorrect state
                }
            }
            break;
        }
    }
    else if (event->buttonEvent == tesBeClick) {
        // "click" can be received only before "hold". So, a "click" is always a normal click.
        switch (event->buttonId) {
            case ctlButtonF1:
            case ctlButtonF2:
            case ctlButtonF3:
            case ctlButtonF4: {
                uint8_t fkey_id = event->buttonId - ctlButtonF1;
                uint8_t fkey_page = event->clicks - 1;
                // check if the page is not out of range
                if (fkey_page < NUMBER_OF_F_PAGES){
                    // check if F-keys can work in the current _editor_status.screenMode
                    if ((_editor_status.screenMode != screenModeDrumsEdit)
                            && (_editor_status.screenMode != screenModeMelodyEdit)){
                        // check if the F-key can be activated now
                        if((_mc->_settings.global.fkeyProfile[fkey_id][fkey_page].editorStatus.screenMode == screenModeInstrumentEdit)
                                && ((_mc->_settings.global.synthType != stProDX) && (_mc->_settings.global.synthType != stAtemp))){
                            // we cannot activate this F-key with the current synth type
                            break;
                        }
                        // activate F-key
                        _editor_status = _mc->_settings.global.fkeyProfile[fkey_id][fkey_page].editorStatus;
                        drawActiveScreen();
                        // clear the event to indicate it has been fully processed
                        event->eventType = tesEmpty;
                    }
                }
                else {
                    // just flash the LED to indicate an invalid condition
                    ledSet.flashLED( ledSystem );
                }
            }
            break;
        }
    }
    else if ( event->buttonEvent == tesBeHold ){
        switch (event->buttonId) {
            case ctlButtonPreset1:
            case ctlButtonPreset2:
            case ctlButtonPreset3:
            case ctlButtonPreset4:
            case ctlButtonPreset5:
            case ctlButtonPreset6: {
                uint8_t preset_id = event->buttonId - ctlButtonPreset1;
                // only one preset may be stored at a time
                if (_preset_sm.state == psNormal){
                    // start the confirmation timer
                    _preset_sm.writeTimer = millis();
                    // change the state to "write confirmation"
                    _preset_sm.state = psWriteConfirmation;
                    // remember the presetID being processed
                    _preset_sm.writePreset = preset_id;
                    // start blinking the LED
                    ledSet.startBlink( preset_id + ledPreset1 );
                }
                // clear the event to indicate it has been fully processed
                event->eventType = tesEmpty;
            }
            break;
            case ctlButtonF1:
            case ctlButtonF2:
            case ctlButtonF3:
            case ctlButtonF4: {
                // check if F-keys can work in the current _editor_status.screenMode
                if ((_editor_status.screenMode != screenModeDrumsEdit)
                        && (_editor_status.screenMode != screenModeMain)
                        && (_editor_status.screenMode != screenModeMelodyEdit)){
                    uint8_t fkey_id = event->buttonId - ctlButtonF1;
                    uint8_t fkey_page = event->clicks - 1;
                    // check if the page is within the range
                    if (fkey_page < NUMBER_OF_F_PAGES){
                        // only one F-key may be stored at a time
                        if (_preset_sm.state == psNormal){
                            // start the confirmation timer
                            _fkey_sm.writeTimer = millis();
                            // change the state to "write confirmation"
                            _fkey_sm.state = psWriteConfirmation;
                            // remember the presetID being processed
                            _fkey_sm.writeFKey = fkey_id;
                            _fkey_sm.writeFKeyPage = fkey_page;
                            // start blinking the LED
                            ledSet.setLED( ledSystem, true );
                            ledSet.startBlink( ledSystem );
                        }
                    }
                    else {
                        // just flash the LED to indicate an invalid condition
                        ledSet.flashLED( ledSystem );
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                }
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Processes Free Bass events
void TesMidiUI::processFreeBassEvent(tesEvent *event){
    // the Free Bass indicator is always visible in the header

    // just redraw the active screen (the _mc should have already taken care about setting its freebass parameter correctly)
    drawActiveScreen();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// This method processes system control events
void TesMidiUI::processSystemControlEvent(tesEvent *event){
    switch(event->command){
    case scEepromReset:
        // just re-draw the screen
        drawActiveScreen();
        break;
    case scEasterEgg:
        _editor_status.screenMode = screenModeEasterEgg;
        // *** ATTENTION! each time we change screens - we turn off the edit mode
        _editor_status.editMode = false;
        drawActiveScreen();
        // we use the inactivity timer for the Easter Egg screenMode
        _inactivity_timer = millis();
        // clear the event
        event->eventType = tesEmpty;
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// This method processes events from the Battery Sensor
void TesMidiUI::processBatteryEvent(tesEvent *event){
    // check if the battery level indicator is visible
    switch (_editor_status.screenMode){
    case screenModeMain:
    case screenModeMidiParamEdit:
        // just redraw the active screen, assuming that _mc->_var already contaings the new value of the battery level
        drawActiveScreen();
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Changes the state of the Drum Machine
void TesMidiUI::setDrumMachineState(uint8_t new_state){
    if (new_state == _mc->_var.drumsMode){
        // do nothing
        return;
    }

    // actions depend on the current state
    switch (_mc->_var.drumsMode){
    case dmOff:
        switch (new_state){
        case dmManual:
            // change the state
            _mc->_var.drumsMode = dmManual;
            // set the LED
            ledSet.setLED(ledDrums, true);
            // send settings of manual drums to the synth.
            _mc->sendDrumsSettings();
            // now - ready to play
            break;
        default:
            SWER(swerGUI05);
        }
        break;
    case dmManual:
        switch (new_state){
        case dmStandby:
            // change the state
            _mc->_var.drumsMode = dmStandby;
            // set the LED
            ledSet.resetLED(ledDrums);  // turn it off first
            ledSet.startBlink(ledDrums);
            // a melody is always ready to play
            break;
        case dmOff:
            // change the state
            _mc->_var.drumsMode = dmOff;
            // set the LED
            ledSet.resetLED(ledDrums);  // turn it off
            break;
        default:
            SWER(swerGUI05);
        }
        break;
    case dmStandby:
        switch (new_state){
        case dmPlaying:
            // change the state
            _mc->_var.drumsMode = dmPlaying;
            // set the LED
            ledSet.resetLED(ledDrums);       // reset possible blinking
            ledSet.setLED(ledDrums, true);   // turn it on, so the ti's the staring state of the LED
            ledSet.startBlink(ledDrums, teslpSlowBlink);
            // send settings of manual drums to the synth.
            _mc->sendDrumsSettings();
            // start the melody
            _mc->_auto_drums.start();
            break;
        case dmManual:
            // change the state
            _mc->_var.drumsMode = dmManual;
            // set the LED
            ledSet.resetLED(ledDrums);
            ledSet.setLED(ledDrums, true);
            // send settings of manual drums to the synth.
            _mc->sendDrumsSettings();
            break;
        default:
            SWER(swerGUI05);
        }
        break;
    case dmPlaying:
        // stop the melody (in any case)
        _mc->_auto_drums.stop();
        switch (new_state){
        case dmOff:
            // change the state
            _mc->_var.drumsMode = dmOff;
            // set the LED
            ledSet.resetLED(ledDrums);  // turn it off
            break;
        case dmManual:
            // change the state
            _mc->_var.drumsMode = dmManual;
            // set the LED
            ledSet.resetLED(ledDrums);
            ledSet.setLED(ledDrums, true);
            // send settings of manual drums to the synth.
            _mc->sendDrumsSettings();
            break;
        default:
            SWER(swerGUI05);
        }
        break;
    default:
        SWER(swerGUI04);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  auxiliary data
////////////////////////////////////////////////////////////////////////////////////////////////

#define symbolHeight    8       // pixels
#define symbolWidth     6       // pixels

#define maxFormatDescriptorLength   32
#define maxFormattedStringLength    32
const char numFormat3R[] = "%3d";       // 3 digits, right alignment
const char auxFormat1[]  = " (%3d)";    // 3 digits, right alignment

const char main_screen_footer[] = "MIDI баян v2.02b";

// for the header
#define headerStartPosition 6
#define headerIncrement     4
#define headerSections      4
const char * const  headerSection[] = {
    "Пр.",
    "Бас",
    "Акк",
    "Выб"
};

#define theNumberOfMidiParameters   NUMBER_OF_KBD_PARAMETERS
// labels for Kbd Parameters
const char * const  parLabel[theNumberOfMidiParameters] = {
    "Канал",
    "Банк",
    "Инстр.",
    "Громк.",
    "Вибр.",
    "Рез-с",
    "Атака",
    "Спад",
    "Тембр",
    "Затух.",
    "Порт.",
    "Ревер.",
    "Хорус",
    "Р. тип",
    "Х. тип",
    "Окт +-"
};
// Hints for Kbd Parameters
const char * const  parHint[theNumberOfMidiParameters] = {
    "MIDI канал",
    "Банк инструментов",
    "Номер инструмента",
    "Громкость канала",
    "Вибрато",
    "Резонанс",
    "Атака",
    "Спад",
    "Тембр",
    "Затухание",
    "Время Портаменто",
    "Ревербрация",
    "Хорус",
    "Тип ревербрации",
    "Тип хоруса",
    "Выше/ниже на октаву"
};
// Maximum possible values for Kbd Parameters
const uint8_t parMaxValue[theNumberOfMidiParameters] = {
    15, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 7, 7, 2
};
// Minimum possible values for Kbd Parameters
const int8_t parMinValue[theNumberOfMidiParameters] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -2
};

// definitions for printing the preset page ID on the main screen
#define ppY ((3<<3) + (symbolHeight/2))
const char ppText[] = "Пресеты #%1d";

const char * const  indGyroModeLabel[] = {
    "G:--",  // General MIDI
    "G:X",   // ATemp
    "G: Y",  // Atemp Pro.DX
    "G:XY"   // not used yet
};

// "Indicators" showing values of the MIDI controller local settings
#define indRow                  5
#define indNumber               4   // the number of implemented indicators for now

#define indBassOctavesPos       1
const char indBassOctavesLabel[] = "Окт";

#define indPressureSensorOnPos  7
const char indPressureSensorLabel[] = "Мех";

#define indSynthTypePos         13
const char * const  indSynthTypeLabel[] = {
    "GM",    // General MIDI
    "AT",    // ATemp
    "DX",    // Atemp Pro.DX
    "--"     // not used yet
};

#define indRunningStatusPos     18
const char indRunningStatusLabel[] = "RS";

#define editorFirstRow  1
#define editorRows      6           // the last row is reserved for hints

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the editor of system parameters

// 3 types of parameters:
// - boolean
// - number
// - fixed list of values
enum {parBoolean, parNumber, parList};

// universal data for boolean parameters
const char * const  parBooleanLabel[] = {
    "Нет",
    "Да"
};
#define parBooleanN             2       // the number of options

// list of parameter IDs
enum {parPresetPageId = 0, parRunningStatusId, parBassOctavesId, parGyroId, parSynthTypeId, parMasterVolumeId, parDrumsChannelId, parDrumsAdditionalChannelId,
    parInactivityPeriodId,
    parBuildId};    // <-- this parameter must be the last one

/////////////////////////////////////////////////////////////////
// specific data for the Master Volume parameter
const char parPresetPageName[]    = "Набор пресетов";
#define parPresetPageMaxValue 3     // 0 .. 3

/////////////////////////////////////////////////////////////////
// specific data forn the Running Status parameter
const char parRunningStatusName[]   = "Running Status";

/////////////////////////////////////////////////////////////////
// specific data forn the Bass Octaves parameter
const char parBassOctavesName[]   = "Октавы в басах";

/////////////////////////////////////////////////////////////////
// specific data forn the Running Status parameter
const char parGyroName[]   = "G-датчик Вкл   >";

/////////////////////////////////////////////////////////////////
// specific data for the Synth Type parameter
// It shares the same text constants with the respective indicator
const char * const * parSynthTypeLabel = indSynthTypeLabel;    // shares the same set of constants
const char parSynthTypeName[]       = "Тип синтезатора";
#define parSynthTypeN           3       // the number of options

/////////////////////////////////////////////////////////////////
// specific data for the Master Volume parameter
const char parMasterVolumeName[]    = "Общая громкость";
#define parMasterVolumeMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drums Channel parameter
const char parDrumsChannelName[]    = "Канал ударных";
#define parDrumsChannelMaxValue 15

const char parDrumsAdditionalChannelName[]    = "Канал мелодии";
#define parDrumsAdditionalChannelMaxValue 15

/////////////////////////////////////////////////////////////////
// specific data for the Inactivity Period parameter
const char parInactivityPeriodName[]    = "Автовыход (сек)";
#define parInactivityPeriodMaxValue 60

/////////////////////////////////////////////////////////////////
// specific data for the Inactivity Period parameter
//                              01234567
const char parBuildName[]    = "Сборка";
#define parBuildIdColumn    10

/////////////////////////////////////////////////////////////////
// common data

#define paramEditorFirstRow     0       // we start from this row
#define paramEditorRows         7       // 7 rows in the scroll area
#define paramEditorNameColumn   0       // where we start printing names
#define paramEditorValueColumn  17      // where we start printing values

#define parNumberOfParameters   10
const char * const  parParameterNames[parNumberOfParameters] = {
    parPresetPageName, parRunningStatusName, parBassOctavesName, parGyroName, parSynthTypeName, parMasterVolumeName, parDrumsChannelName,
    parDrumsAdditionalChannelName, parInactivityPeriodName,
    parBuildName    // <-- keep this one the last
};

const char sys_param_screen_footer[] = "Настройки системы";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the editor of drums parameters

/////////////////////////////////////////////////////////////////
// specific data for the Master Volume parameter
const char dpDrumsVolumeName[]    = "Громк. ударных";
#define dpDrumsVolumeMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drumset Number parameter
const char dpDrumsetNumberName[]    = "Набор звуков";
#define dpDrumsetNumberMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Bass parameter
const char dpDrumSoundForBassName[]    = "Звук для баса";
#define dpDrumSoundForBassMinValue F1      // this is a MIDI note
#define dpDrumSoundForBassMaxValue E7      // this is a MIDI note

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Bass Velocity parameter
const char dpDrumSoundForBassVelocityName[]    = "     его сила";
#define dpDrumSoundForBassVelocityMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Chord parameter
const char dpDrumSoundForChordName[]    = "Звук д. аккорда";
#define dpDrumSoundForChordMinValue F1      // this is a MIDI note
#define dpDrumSoundForChordMaxValue E7      // this is a MIDI note

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Chord Velocity parameter
const char dpDrumSoundForChordVelocityName[]    = "       его сила";
#define dpDrumSoundForChordVelocityMaxValue 127

/////////////////////////////////////////////////////////////////
// common data

#define dpEditorFirstRow     0       // we start from this row
#define dpEditorRows         6       // number of rows in the scroll area
#define dpEditorRowIncrement 1       // 0 empty row between parameters
#define dpEditorNameColumn   0       // where we start printing names
#define dpEditorValueColumn  17      // where we start printing values

#define dpNumberOfParameters   6
const char * const  dpParameterNames[dpNumberOfParameters] = {
    dpDrumsVolumeName, dpDrumsetNumberName, dpDrumSoundForBassName, dpDrumSoundForBassVelocityName, dpDrumSoundForChordName, dpDrumSoundForChordVelocityName
};

const char drums_param_screen_footer[] = "Настройки ударных";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the Synth-specific instrument editor

// list of parameter IDs
enum {ieGroupId = 0, ieInstrumentId};
#define ieNumberOfParameters 2

const char ieInstrumentGroupTitle[] = "Группа инструментов:";
#define ieInstrumentGroupTitlePos   0   // 1st row
#define ieInstrumentGroupNamePos    1   // just below the title

const char ieInstrumentTitle[] = "Инструмент:";
#define ieInstrumentTitlePos   3        // 4th row
#define ieInstrumentNamePos    4        // just below the title

//                                  123456789-123456789-1
const char ieInstrumentIDsLine[] = "Банк: XXX, Прог.: XXX";
#define ieInstrumentIDsLinePos  6
#define ieBankIdColumn      6
#define ieProgramIdColumn   18

const char instrument_edit_screen_footer[] = "Выбор инструмента";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for Melody Selector screen

#define msEditorFirstRow     0       // we start from this row
#define msEditorRows         7       // number of rows in the scroll area
#define msEditorNameColumn   0       // where we start printing names

//                                            123456789-123456789-1
const char melody_selector_screen_footer[] = "Выбор мелодии";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the editor of Gyro parameters

// list of parameter IDs
enum {gpModeId = 0, gpXCtrlId, gpYCtrlId, gpPitchDepthId};

/////////////////////////////////////////////////////////////////
// specific data for the Gyro Mode parameter
const char gpModeName[]    = "Активные оси";
#define gpModeMaxValue 2

const char * const  gpModeLabel[] = {
    " X ",
    " Y ",
    "X+Y",
    "-O-"     // not used
};

/////////////////////////////////////////////////////////////////
// specific data for the X Controller parameter
const char gpXCtrlName[]    = "Контроллер X";
#define gpXCtrlMaxValue 3

const char * const  gpCtrlLabel[] = {
    "Б ^",
    "Б v",
    "Виб",
    "Прт"
};
/////////////////////////////////////////////////////////////////
// specific data for the X Controller parameter
const char gpYCtrlName[]    = "Контроллер Y";
#define gpYCtrlMaxValue 3

/////////////////////////////////////////////////////////////////
// specific data for the Pitch Depth parameter
const char gpPitchDepthName[]    = "Глубина бенда";
#define gpPitchDepthMaxValue 6
#define gpPitchDepthMinValue 1

/////////////////////////////////////////////////////////////////
// common data

#define gpEditorFirstRow     1       // we start from this row
#define gpEditorRows         4       // number of rows in the scroll area
#define gpEditorRowIncrement 1       // 0 empty row between parameters
#define gpEditorNameColumn   0       // where we start printing names
#define gpEditorValueColumn  17      // where we start printing values

#define gpNumberOfParameters   4
const char * const  gpParameterNames[gpNumberOfParameters] = {
    gpModeName, gpXCtrlName, gpYCtrlName, gpPitchDepthName
};

const char gyro_param_screen_footer[] = "Настройки G-датчика";

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// private methods //////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// стырено отсюда: https://forum.amperka.ru/threads/Подсчёт-числа-символов-в-строке.19457/#post-237619
//
// Функция подсчёта количества символов в строке utf8,
// состоящей из букв английского и русского алфавитов, цифр, общепринятых символов...

//int mb_strlen_P(PGM_P _source)
int mb_strlen_P(const char * _source)
{
    const char * p = _source;

    int target = 0;
    unsigned char n;

    n = *p++;
    while (n != 0) {
        if (n >= 0xBF){
            switch (n) {
            case 0xD0:
            case 0xD1:
                n = *p++;
                break;
            }
        }
        target = target + 1;
        n = *p++;
    }
    return target;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns the specified sub-string from the large string stored in
// The large string contains many shorter strings ending with \0
PGM_P   getSubString_P(PGM_P storage, int16_t index){
    while (index-- > 0){
        // skip the current string
        while(*storage++ != 0);
    }
    return storage;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns a group_id as defined in group_map for program_id
uint8_t getInstrumentGroupId(const uint8_t * group_map, uint8_t program_id){
    uint8_t group_id = 0;

    while(group_map[group_id] < program_id){
        group_id++;
    }

    return group_id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns the global instrument index (as defined in )
uint16_t getInstrumentIndex(bool * need_to_reset_bank_id, const uint8_t * const * bank_map, uint8_t bank_id, uint8_t program_id){
    uint16_t instrument_index = 0;
    *need_to_reset_bank_id = false;

    // count all instruments BEFORE program_id
    for(int i=0; i<program_id; i++){
        // get a bank vector for program 'i'
        const uint8_t *ptr = bank_map[i];
        if (ptr == NULL){
            // There's only one variation for this program
            instrument_index += 1;  // counting the only variation of program 'i'
        }
        else{
            // There're variations for program 'i'. The number of variations is stored in the 1st element of the vector.
            instrument_index += ptr[0];
        }
    }

    // get a bank vector for program_id
    const uint8_t *ptr = bank_map[program_id];
    if (ptr == NULL){
        // there's only one variation for program_id
        // there's nothing to add; instrument_index is already pointing to {bank_id}:{program_id}
        if(bank_id != 0){
            // if ptr == NULL then the bank_id is expected to be 0
            *need_to_reset_bank_id = true;
        }
    }
    else {
        // count prior variations of program_id
        uint8_t N = ptr[0];
        uint16_t    saved_instrument_index = instrument_index;
        bool was_found = false;
        for(int i=1; i<=N; i++){
            if (ptr[i] != bank_id){
                // count this variation
                instrument_index += 1;
            }
            else {
                // we've got to the specified bank_id
                // there's nothing to add; instrument_index is already pointing to {bank_id}:{program_id}
                was_found = true;
                break;
            }
        }
        if ( !was_found ){
            // bank_id is not listed for program_id in bank_map

            // restore the saved value of the instrument_index
            instrument_index = saved_instrument_index;
            // raise the flag
            *need_to_reset_bank_id = true;
        }
    }

    return instrument_index;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function selects the next Instrument accoring to the provided bank_map and direction
void setNextInstrument(const uint8_t * const * bank_map, uint8_t * bank_id, uint8_t * program_id, bool forward = true){
    uint8_t cur_bank = *bank_id;
    uint8_t cur_program = *program_id;

    uint8_t bank_index = 0xFF;                  // out-of-range value; if it stays the same after search - that means we didn't find the entry in the vector
    // get a bank vector for program 'i'
    const uint8_t *ptr = bank_map[cur_program];
    // check if we can move to the next bank_id
    if (ptr != NULL){
        uint8_t N = ptr[0];             // the size of the vector
        for(int i=1; i<=N; i++){
            if (ptr[i] == cur_bank){
                // we've got to the specified bank_id
                bank_index = i;
                break;
            }
        }
        if (bank_index == 0xFF){
            // we didn't find a correct entry
            SWER(swerGUI13);
        }
        // *** now, we're sure that bank_index points to the current bank_id in the vector
        // check if there's a next/previous element in the vector
        if (forward && (bank_index < N)){
            // just move to the next bank_id
            bank_index++;
        }
        else if ( !forward && (bank_index > 1)){        // comparing to '1' because this is a vector and [0] contains the length!
            // just move to the previous bank_id
            bank_index--;
        }
        else {
            // indicate that we need to switch to another program_id
            bank_index = 0xFF;
        }
    }
    // check if we already have a new pair of bank_id:program_id
    if (bank_index != 0xFF){
        // just set the new bank_id
        *bank_id = ptr[bank_index];
        // program_id stays the same in this case
        // all done
        return;
    }
    // *** So, we have to switch to another program_id

    // first, go to the new program_id
    if (forward){
        if(cur_program++ == 127){
            cur_program = 0;
        }
    }
    else{
        if(cur_program-- == 0){
            cur_program = 127;
        }
    }
    // we can already set the new program_id
    *program_id = cur_program;
    // *** now, let's find the next bank_id
    // get a new bank vector
    ptr = bank_map[cur_program];
    if (ptr == NULL){
        // easy case: the next bank id is 0
        *bank_id = 0;
    }
    else{
        uint8_t N = ptr[0];             // the size of the vector
        // read either the 1st of the last element (depending on the direction)
        *bank_id = ptr[((forward)? 1 : N)];
    }
    // all done; new values were put into *bank_id and *program_id
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the top line of the screen (shared between Main screen & MIDI Param Editor
void TesMidiUI::drawKbdHeader(void){
    // prepare temporary data
    bool headerHighlight[headerSections] = {
        true,                                       // always highlight right keyboard
        ( ! _mc->_var.freeBassOn),             // bass keyboard - opposite to the Free Bass mode
        ( ! _mc->_var.freeBassOn),             // chord keyboard - opposite to the Free Bass mode
        (bool)_mc->_var.freeBassOn                   // free bass keyboard - same as the Free Bass mode
    };
    for(int i=0; i<headerSections; i++){
        setTextCursor(headerStartPosition + i*headerIncrement, 0);
        oled.invertText(headerHighlight[i]);
        oled.print(headerSection[i]);
    }
    // turn inversion off
    oled.invertText(false);
}

// Bitmaps for the battery level indicator
const uint8_t bat_000_17x8[] = { 0x80, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0x80, 0xE3, 0xE3 };
const uint8_t bat_025_17x8[] = { 0x80, 0xBE, 0xA2, 0xA2, 0xA2, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0x80, 0xE3, 0xE3 };
const uint8_t bat_050_17x8[] = { 0x80, 0xBE, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0xBE, 0x80, 0xE3, 0xE3 };
const uint8_t bat_075_17x8[] = { 0x80, 0xBE, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xBE, 0xBE, 0xBE, 0x80, 0xE3, 0xE3 };
const uint8_t bat_100_17x8[] = { 0x80, 0xBE, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xA2, 0xBE, 0x80, 0xE3, 0xE3 };
const uint8_t * const bat_bitmap[5] = {
    bat_000_17x8, bat_025_17x8, bat_050_17x8, bat_075_17x8, bat_100_17x8
};
enum    {battery_000 = 0, battery_025, battery_050, battery_075, battery_100};

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the battery level indicator (shared between Main screen & MIDI Param Editor)
void TesMidiUI::drawBatteryIndicator(void){
    int selected_bitmap = battery_000;

    // check if we need to select another bitmap
    if(_mc->_var.currentBatteryLevel > 90){
        selected_bitmap = battery_100;
    }
    else if(_mc->_var.currentBatteryLevel > 60){
        selected_bitmap = battery_075;
    }
    else if(_mc->_var.currentBatteryLevel > 40){
        selected_bitmap = battery_050;
    }
    else if (_mc->_var.currentBatteryLevel > 20){
        selected_bitmap = battery_025;
    }
    oled.drawBitmap(0, 0, bat_bitmap[selected_bitmap], 17, 8, BITMAP_INVERT, BUF_REPLACE);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the main screen
void TesMidiUI::drawMainScreen(void){
    oled.clear();

    // print the header
    drawKbdHeader();
    // print the battery level Indicator
    drawBatteryIndicator();
    // print 2 parameters: bank & instrument
    for(int idx=0; idx<2; idx++){
        setTextCursor(0, idx+1);
        uint8_t parameter_id = 0;
        switch(idx){
            case 0:
                parameter_id    = idxBank;
                break;
            case 1:
                parameter_id    = idxProgram;
                break;
        }
        oled.print(parLabel[parameter_id]);
        // print parameters here
        for(int kbd=0; kbd<4; kbd++){
            setTextCursor(6 + kbd*4, idx+1);
            printFormatted(numFormat3R, _mc->_settings.preset.kbdParameter[kbd][parameter_id], isSigned(parameter_id));
        }
    }
    // print the Id of the current preset page
    oled.setCursorXY( 2, ppY);
    printFormatted(ppText, _mc->_settings.global.activePresetPage + 1);     // we show preset page numbers starting from "1"
    // print a frame around the preset info
    // calculate graphical X and Y
    uint8_t frameX1 = 0;
    uint8_t frameX2 = 6*(mb_strlen_P(ppText)-2) + 3;
    uint8_t frameY1 = ppY - 2;
    uint8_t frameY2 = ppY + 8;
    oled.roundRect(frameX1, frameY1, frameX2, frameY2, OLED_STROKE);
    // print Indicators
    {
        // Indicator's logic is different, so, print them onr-by-one (not in a cycle)
        // *** G-sensor status -- on a separate line!
        oled.setCursorXY( 14*symbolWidth, ppY);
        oled.print(indGyroModeLabel[((!_mc->_settings.preset.gyroOn)? 0 : _mc->_settings.preset.gyroMode+1)]);
        // *** Bass Octaves
        setTextCursor(indBassOctavesPos, indRow);
        oled.invertText( _mc->_settings.preset.bassOctavesOn );
        oled.print(indBassOctavesLabel);
        oled.invertText(false);
        // *** Pressure Sensor
        setTextCursor(indPressureSensorOnPos, indRow);
        oled.invertText( _mc->_settings.preset.pressureSensorOn );
        oled.print(indPressureSensorLabel);
        oled.invertText(false);
        // *** Synthesizer Type
        setTextCursor(indSynthTypePos, indRow);
        oled.print(indSynthTypeLabel[_mc->_settings.global.synthType]);
        // *** Running Status
        setTextCursor(indRunningStatusPos, indRow);
        oled.invertText( _mc->_settings.global.runningStatus );
        oled.print(FPSTR(indRunningStatusLabel));
        oled.invertText(false);
    }
    // print the bottom
    oled.fastLineH(7*symbolHeight-2, 3, 124, OLED_FILL);
    // If a melody is not played - print a banner
    PGM_P   melodyName  = _mc->_auto_drums.getCurrentMelodyName();
    switch(_mc->_var.drumsMode){
    case dmOff:
    case dmManual:
        oled.setCursor( (128 - mb_strlen_P(FPSTR(main_screen_footer))*symbolWidth)/2, 7);
        oled.print(FPSTR(main_screen_footer));
        break;
    case dmStandby:
    case dmPlaying:
        oled.setCursor( (128 - (mb_strlen_P(FPSTR(melodyName)) + 6)*symbolWidth)/2, 7);
        oled.print(FPSTR(melodyName));
        printFormatted(auxFormat1, _mc->_auto_drums.getTempo());
        break;
    }

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Sets the cursor for text output
void TesMidiUI::setTextCursor(uint8_t X, uint8_t Y){
    // the original method uses pixels for X instead of sympols.
    // (6 pixels per symbol)
    oled.setCursor( X*symbolWidth , Y);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  prints a number according to the specified format string
void TesMidiUI::printFormatted(const char * format, uint16_t value, bool sgnd){
    char buf[maxFormattedStringLength];     // for a properly aligned number

    if (sgnd){
        sprintf(buf, format, (int8_t)value);
    }
    else{
        sprintf(buf, format, value);
    }
    oled.print(buf);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the active screen
void TesMidiUI::drawActiveScreen(void){
    switch (_editor_status.screenMode){
    case screenModeMain:
        drawMainScreen();
        break;
    case screenModeMidiParamEdit:
        drawMidiParamEditorScreen();
        break;
    case screenModeSysParamEdit:
        drawSystemParamEditorScreen();
        break;
    case screenModeDrumsEdit:
        drawDrumsParamEditorScreen();
        break;
    case screenModeInstrumentEdit:
        drawInstrumentEditorScreen();
        break;
    case screenModeMelodyEdit:
        drawMelodySelectorScreen();
        break;
    case screenModeGyroEdit:
        drawGyroEditorScreen();
        break;
    case screenModeEasterEgg:
        drawEasterEgg();
        break;
    default:
        SWER(swerGUI08);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the MIDI parameters editor screen
void TesMidiUI::drawMidiParamEditorScreen(void){
    // clear work area
    oled.clear();
    // print the header
    drawKbdHeader();
    // print the battery level Indicator
    drawBatteryIndicator();
    // print rows in the work area
    for (int editorRow=0; editorRow<editorRows; editorRow++ ){
        setTextCursor(0, editorRow+editorFirstRow);
        uint8_t currentParameterIndex = editorRow + _editor_status.topIndex;
        oled.print( parLabel[currentParameterIndex] );
        // print values
        for(int kbd=0; kbd<4; kbd++){
            // decide text inversion - select the value being edited
            oled.invertText(
                _editor_status.editMode
                && (_editor_status.selectorX == kbd)
                && (_editor_status.selectorY == editorRow)
            );
            setTextCursor(6 + kbd*4, editorRow+editorFirstRow);
            if(currentParameterIndex == idxChannel){
                printFormatted(numFormat3R, _mc->_settings.preset.kbdParameter[kbd][currentParameterIndex] + 1);
            }
            else{
                printFormatted(numFormat3R, _mc->_settings.preset.kbdParameter[kbd][currentParameterIndex], isSigned(currentParameterIndex));
            }
        }
        //reset text inversion (just in case...)
        oled.invertText(false);
    }
    // draw the selector (frame around the current value)
    // calculate graphical X and Y
    uint8_t selX1 = 6*symbolWidth +_editor_status.selectorX*4*symbolWidth -3;
    uint8_t selX2 = 6*symbolWidth +_editor_status.selectorX*4*symbolWidth +symbolWidth*3 +1;
    uint8_t selY1 = symbolHeight*editorFirstRow -1 +_editor_status.selectorY*symbolHeight;
    uint8_t selY2 = symbolHeight*editorFirstRow +7 +_editor_status.selectorY*symbolHeight;
    oled.roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    uint8_t selectedParameterIndex = _editor_status.topIndex + _editor_status.selectorY;
    oled.setCursor( (128 - mb_strlen_P(parHint[selectedParameterIndex])*symbolWidth)/2, 7);
    oled.print(parHint[selectedParameterIndex]);

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the System parameters editor screen
void TesMidiUI::drawSystemParamEditorScreen(void){
    // clear work area
    oled.clear();
    // print rows in the work area
    for (int editorRow=0; editorRow<paramEditorRows; editorRow++){
        setTextCursor(paramEditorNameColumn, editorRow + paramEditorFirstRow);
        uint8_t currentSystemParameterIndex = editorRow + _editor_status.sysParTopIndex;
        oled.print( parParameterNames [currentSystemParameterIndex] );
        // print the value
        setTextCursor(paramEditorValueColumn, editorRow + paramEditorFirstRow);
        // decide text inversion - select the value being edited
        oled.invertText(editorRow == _editor_status.sysParSelector);
        switch(currentSystemParameterIndex){
            case parPresetPageId:
                printFormatted(numFormat3R, _mc->_settings.global.activePresetPage + 1);
                break;
            case parRunningStatusId:
                oled.print( parBooleanLabel[_mc->_settings.global.runningStatus] );
                break;
            case parBassOctavesId:
                oled.print( parBooleanLabel[_mc->_settings.preset.bassOctavesOn] );
                break;
            case parGyroId:
                oled.print( parBooleanLabel[_mc->_settings.preset.gyroOn] );
                break;
            case parSynthTypeId:
                oled.print( parSynthTypeLabel[_mc->_settings.global.synthType] );
                break;
            case parMasterVolumeId:
                printFormatted(numFormat3R, _mc->_settings.preset.masterVolume);
                break;
            case parDrumsChannelId:
                printFormatted(numFormat3R, _mc->_settings.global.drumsMidiChannel + 1);
                break;
            case parDrumsAdditionalChannelId:
                printFormatted(numFormat3R, _mc->_settings.global.drumsAdditionalChannel + 1);
                break;
            case parInactivityPeriodId:
                printFormatted(numFormat3R, _mc->_settings.global.inactivityPeriod);
                break;
            case parBuildId:
                setTextCursor(parBuildIdColumn, editorRow + paramEditorFirstRow);
                oled.print(build_id);
                break;
            default:
                SWER(swerGUI02);
        }
        //reset text inversion (just in case...)
        oled.invertText(false);
    }

    if((_editor_status.sysParTopIndex + _editor_status.sysParSelector) != parBuildId){
        // draw the selector (frame around the current value)
        // calculate graphical X and Y
        uint8_t selX1 = paramEditorValueColumn*symbolWidth -3;
        uint8_t selX2 = paramEditorValueColumn*symbolWidth +symbolWidth*3 +1;
        uint8_t selY1 = symbolHeight*paramEditorFirstRow -1 +_editor_status.sysParSelector*symbolHeight;
        if (selY1 == 255){
            selY1 = 0;
        }
        uint8_t selY2 = symbolHeight*paramEditorFirstRow +7 +_editor_status.sysParSelector*symbolHeight;
        oled.roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);
    }

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    oled.setCursor( (128 - mb_strlen_P(FPSTR(sys_param_screen_footer))*symbolWidth)/2, 7);
    oled.print(FPSTR(sys_param_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Drums parameters editor screen
void TesMidiUI::drawDrumsParamEditorScreen(void){
    // clear work area
    oled.clear();
    // print rows in the work area
    for (int param_number=0; param_number < dpNumberOfParameters; param_number++){
        setTextCursor(dpEditorNameColumn, param_number * dpEditorRowIncrement + dpEditorFirstRow);
        oled.print( dpParameterNames[param_number] );
        // print the value
        setTextCursor(dpEditorValueColumn, param_number * dpEditorRowIncrement + dpEditorFirstRow);
        // decide text inversion - select the value being edited
        oled.invertText(param_number == _editor_status.drumsParSelector);
        switch(param_number){
            case dpDrumsVolume:
                printFormatted(numFormat3R, _mc->_settings.preset.drumsVolume);
                break;
            case dpDrumsetNumber:
                printFormatted(numFormat3R, _mc->_settings.preset.drumsetNumber);
                break;
            case dpSoundForBass:
                printFormatted(numFormat3R, _mc->_settings.preset.drumSoundForBass);
                break;
            case dpSoundForBassVelocity:
                printFormatted(numFormat3R, _mc->_settings.preset.drumSoundForBassVelocity);
                break;
            case dpSoundForChord:
                printFormatted(numFormat3R, _mc->_settings.preset.drumSoundForChord);
                break;
            case dpSoundForChordVelocity:
                printFormatted(numFormat3R, _mc->_settings.preset.drumSoundForChordVelocity);
                break;
            default:
                SWER(swerGUI03);
        }
        //reset text inversion (just in case...)
        oled.invertText(false);
    }

    // draw the selector (frame around the current value)
    // calculate graphical X and Y
    uint8_t selX1 = dpEditorValueColumn*symbolWidth -3;
    uint8_t selX2 = dpEditorValueColumn*symbolWidth +symbolWidth*3 +1;
    uint8_t selY1 = symbolHeight*dpEditorFirstRow -1 +_editor_status.drumsParSelector*dpEditorRowIncrement*symbolHeight;
    if (selY1 == 255){
        selY1 = 0;
    }
    uint8_t selY2 = symbolHeight*dpEditorFirstRow +7 +_editor_status.drumsParSelector*dpEditorRowIncrement*symbolHeight;
    oled.roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    oled.setCursor( (128 - mb_strlen_P(FPSTR(drums_param_screen_footer))*symbolWidth)/2, 7);
    oled.print(FPSTR(drums_param_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Instrument selector/editor screen
void TesMidiUI::drawInstrumentEditorScreen(void){
    // This function can be called in the ATEMP mode only
    // set aux valiables up
    const char * const * _GroupName = NULL;
    const uint8_t * _GroupMap       = NULL;
    const char * _InstrumentNames   = NULL;
    const uint8_t * const * _Banks  = NULL;
    switch(_mc->_settings.global.synthType){
        case stProDX:
            _GroupName          = dxGroupName;
            _GroupMap           = dxGroupMap;
            _InstrumentNames    = dxInstrumentNames;
            _Banks              = dxBanks;
            break;
        case stAtemp:
            _GroupName          = atGroupName;
            _GroupMap           = atGroupMap;
            _InstrumentNames    = atInstrumentNames;
            _Banks              = atBanks;
            break;
        default:
            SWER(swerGUI09);
    }
    // Info: the ID of the current Keyboard is in _editor_status.selectorX (because this screen was called from Parameters Editor)
    // determine the instrument group and instrument index
    uint8_t     bank_id = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxBank];
    uint8_t     program_id = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram];
    uint8_t     instrument_group = getInstrumentGroupId(_GroupMap, program_id);
    bool        need_to_reset_bank_id = false;
    uint16_t    instrument_index = getInstrumentIndex(&need_to_reset_bank_id, _Banks, bank_id, program_id);
    if (need_to_reset_bank_id){
        // the specified instrument variation (AKA "bank") was not found.
        // Let's reset it to 0
        bank_id = 0;
        _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxBank] = 0;
        // notify the MIDI controller about this change
        _mc->processChangedKbdParameter(_editor_status.selectorX, idxBank);
        // mark the current presed and "changed and not saved"
        indicatePresetChange();
    }

    // clear work area
    oled.clear();

    // Print the title of Instrument group
    oled.setCursor((128 - mb_strlen_P(ieInstrumentGroupTitle)*symbolWidth)/2, ieInstrumentGroupTitlePos);
    oled.print(FPSTR(ieInstrumentGroupTitle));
    // Print the Instrument group name
    oled.setCursor((128 - mb_strlen_P(_GroupName[instrument_group])*symbolWidth)/2, ieInstrumentGroupNamePos);
    oled.print(_GroupName[instrument_group]);

    // Print the title of Instrument
    oled.setCursor((128 - mb_strlen_P(ieInstrumentTitle)*symbolWidth)/2, ieInstrumentTitlePos);
    oled.print(ieInstrumentTitle);
    // Print the Instrument name
    PGM_P   str = getSubString_P(_InstrumentNames, instrument_index);
    oled.setCursor((128 - mb_strlen_P(str)*symbolWidth)/2, ieInstrumentNamePos);
    oled.print(FPSTR(str));

    // draw the selector
    // calculate graphical X and Y
    uint8_t selX1 = 3*symbolWidth -5;                       // 3 - first column of the text
    uint8_t selX2 = 17*symbolWidth + symbolWidth +4;        // 17 - last column of the text
    uint8_t selRow = 0;
    switch (_editor_status.instrumentSelector ){
    case ieGroupId:
        selRow = ieInstrumentGroupNamePos;
        break;
    case ieInstrumentId:
        selRow = ieInstrumentNamePos;
        break;
    }
    uint8_t selY1 = symbolHeight*selRow -1;
    uint8_t selY2 = symbolHeight*selRow +7;
    oled.roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // Print instrument IDs
    oled.setCursor(0, ieInstrumentIDsLinePos);    // to the very beginning of the line
    oled.print(ieInstrumentIDsLine);
    oled.setCursor(ieBankIdColumn*symbolWidth, ieInstrumentIDsLinePos);
    // print the Bank ID
    printFormatted(numFormat3R, bank_id);
    oled.setCursor(ieProgramIdColumn*symbolWidth, ieInstrumentIDsLinePos);
    // print the Program ID
    printFormatted(numFormat3R, program_id);

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    oled.setCursor( (128 - mb_strlen_P(instrument_edit_screen_footer)*symbolWidth)/2, 7);
    oled.print(instrument_edit_screen_footer);

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Instrument selector/editor screen
void TesMidiUI::drawMelodySelectorScreen(void){
    // clear work area
    oled.clear();

    // See how many rows we need to print
    uint8_t rows_for_printing   = min(msEditorRows, _mc->_auto_drums.getNumberOfMelodies());
    // print rows in the work area
    for (int editorRow=0; editorRow<rows_for_printing; editorRow++ ){
        // set the cursor
        setTextCursor(msEditorNameColumn, editorRow+msEditorFirstRow);
        // calculate the index of a melody to be printed here
        uint8_t currentMelodyIndex  = editorRow + _editor_status.msTopIndex;
        // decide if we need to inverse text for this melody name
        oled.invertText(editorRow == _editor_status.msSelector);
        // print the melody name
        oled.print(_mc->_auto_drums.getMelodyName(currentMelodyIndex));
        //reset text inversion (just in case...)
        oled.invertText(false);
    }

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    oled.setCursor( (128 - mb_strlen_P(melody_selector_screen_footer)*symbolWidth)/2, 7);
    oled.print(FPSTR(melody_selector_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Instrument selector/editor screen
void TesMidiUI::drawGyroEditorScreen(void){
    // clear work area
    oled.clear();

    // print rows in the work area
    for (int param_number=0; param_number < gpNumberOfParameters; param_number++){
        setTextCursor(gpEditorNameColumn, param_number * gpEditorRowIncrement + gpEditorFirstRow);
        oled.print( gpParameterNames[param_number] );
        // print the value
        setTextCursor(gpEditorValueColumn, param_number * gpEditorRowIncrement + gpEditorFirstRow);
        // decide text inversion - select the value being edited
        oled.invertText(param_number == _editor_status.gyroParSelector);
        switch(param_number){
            case gpModeId:
                oled.print(gpModeLabel[_mc->_settings.preset.gyroMode]);
                break;
            case gpXCtrlId:
                oled.print(gpCtrlLabel[_mc->_settings.preset.gyroXCtrl]);
                break;
            case gpYCtrlId:
                oled.print(gpCtrlLabel[_mc->_settings.preset.gyroYCtrl]);
                break;
            case gpPitchDepthId:
                printFormatted(numFormat3R, _mc->_settings.preset.gyroPitchDepth);
                break;
            default:
                SWER(swerGUI15);
        }
        //reset text inversion (just in case...)
        oled.invertText(false);
    }

    // draw the selector (frame around the current value)
    // calculate graphical X and Y
    uint8_t selX1 = gpEditorValueColumn*symbolWidth -3;
    uint8_t selX2 = gpEditorValueColumn*symbolWidth +symbolWidth*3 +1;
    uint8_t selY1 = symbolHeight*gpEditorFirstRow -1 +_editor_status.gyroParSelector*gpEditorRowIncrement*symbolHeight;
    if (selY1 == 255){
        selY1 = 0;
    }
    uint8_t selY2 = symbolHeight*gpEditorFirstRow +7 +_editor_status.gyroParSelector*gpEditorRowIncrement*symbolHeight;
    oled.roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    oled.fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    oled.setCursor( (128 - mb_strlen_P(gyro_param_screen_footer)*symbolWidth)/2, 7);
    oled.print(gyro_param_screen_footer);

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler to Midi Param Editor mode
void TesMidiUI::processCtlButtonEventMidiParamEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonUp:
            if (_editor_status.editMode){
                // we don't move up/down in the edit mode
            }
            else {
                // move to the previous row
                if( _editor_status.selectorY == 0){
                    // the last editor Row
                    // Scroll the list down until topIndex == 0
                    if((_editor_status.topIndex) > 0){
                        // scrolling is done by decrementing the topIndex value
                        _editor_status.topIndex--;
                    }
                    else{
                        // The beginning of the parameter list is reached.
                        // Navigate to the very end of the list:
                        // - set topIndex to theNumberOfMidiParameters - editorRows
                        // - set selectorY to editorRows-1 as well
                        _editor_status.topIndex = theNumberOfMidiParameters - editorRows;
                        _editor_status.selectorY = editorRows-1;
                    }
                }
                else{
                    _editor_status.selectorY--;
                }
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            if (_editor_status.editMode){
                // we don't move up/down in the edit mode
            }
            else {
                // move to the next row
                if( _editor_status.selectorY == editorRows-1){
                    // the last editor Row
                    // Scroll the list up until topIndex+selectorY == theNumberOfMidiParameters-1
                    if((_editor_status.selectorY + _editor_status.topIndex) < theNumberOfMidiParameters-1){
                        // scrolling is done by incrementing the topIndex value
                        _editor_status.topIndex++;
                    }
                    else{
                        // The end of the parameter list is reached.
                        // Navigate to the beginning of the list:
                        // - set topIndex to 0 in order to start from the 1st parameter
                        // - set selectorY to 0 as well
                        _editor_status.topIndex = 0;
                        _editor_status.selectorY = 0;
                    }
                }
                else{
                    _editor_status.selectorY++;
                }
            }
            drawActiveScreen();
            break;
        case ctlButtonRight:
        case ctlButtonLeft:
            if (_editor_status.editMode){
                bool    newValueIsOk;   // exit condition from the loop below
                // Parameter to be changed: _mc->_settings.preset.kbdParameter[selectorX][topIndex +selectorY]
                uint8_t parameterIndex = _editor_status.topIndex +_editor_status.selectorY;
                if ((parameterIndex == idxBank)
                        && ((_mc->_settings.global.synthType == stAtemp) || (_mc->_settings.global.synthType == stProDX))){
                    // Bank ID for Atemp synth - is a special case
                    _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] = getNextAtempBankId(_editor_status.selectorX, (event->buttonId == ctlButtonRight));
                }
                else {
                    //  other parameters
                    do {
                        if(event->buttonId == ctlButtonLeft){
                            newValueIsOk = true;
                            if (*(int8_t *)(&_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]) == parMinValue[parameterIndex]){
                                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] = parMaxValue[parameterIndex];
                            }
                            else {
                                if(isSigned(parameterIndex)){
                                    *(int8_t *)(&_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]) -= 1;
                                }
                                else {
                                    _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]--;
                                }
                            }
                        }
                        else {      // ctlButtonRight
                            newValueIsOk = true;
                            if (*(int8_t *)(&_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]) == parMaxValue[parameterIndex]){
                                *(int8_t *)(&_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]) = parMinValue[parameterIndex];
                            }
                            else {
                                if(isSigned(parameterIndex)){
                                    *(int8_t *)(&_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]) += 1;
                                }
                                else {
                                    _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]++;
                                }
                            }
                        }
                        // check the special case: each keyboard should play into a unique MIDI channel
                        // keep changing the Channel ID until it's unique
                        if (parameterIndex == idxChannel){
                            // yes, this is the Channel parameter; let's check the condition
                            uint8_t new_channel = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxChannel];
                            for(int i=0; i<4; i++){
                                if ( (i != _editor_status.selectorX) && (_mc->_settings.preset.kbdParameter[i][idxChannel] == new_channel) ){
                                    newValueIsOk = false;
                                    break;
                                }
                            }
                            if (newValueIsOk){
                                // check another Channels
                                newValueIsOk = (new_channel != _mc->_settings.global.drumsMidiChannel)
                                    && (new_channel != _mc->_settings.global.drumsAdditionalChannel);
                            }
                        }
                    } while ( ! newValueIsOk );
                }
                // *** and now the common part for Left and Right
                // notify the MIDI controller about the change
                _mc->processChangedKbdParameter(_editor_status.selectorX, parameterIndex);
                // adjust bank id if required
                if ((parameterIndex == idxProgram) && adjustBankId(_editor_status.selectorX)){
                    // notify the MIDI controller about this another change
                    _mc->processChangedKbdParameter(_editor_status.selectorX, idxBank);
                }
                // start indicating a "not saved preset", if required
                indicatePresetChange();
            }
            else {
                if(event->buttonId == ctlButtonLeft){
                    // move to the previous column
                    if( _editor_status.selectorX == 0){
                        _editor_status.selectorX = 3;
                    }
                    else{
                        _editor_status.selectorX--;
                    }
                }
                else {      // ctlButtonRight
                    // move to the next column
                    if( _editor_status.selectorX == 3){
                        _editor_status.selectorX = 0;
                    }
                    else{
                        _editor_status.selectorX++;
                    }
                }
            }
            drawActiveScreen();
            break;
        case ctlButtonOk:
            {
                // Parameter to be changed: _mc->_settings.preset.kbdParameter[selectorX][topIndex +selectorY]
                uint8_t parameterIndex = _editor_status.topIndex +_editor_status.selectorY;
                // There's a special case: Bank & Program for the stProDX synth.
                if (((_mc->_settings.global.synthType == stProDX) || (_mc->_settings.global.synthType == stAtemp))
                        && ((parameterIndex == idxBank) || (parameterIndex == idxProgram))){
                    // invoke a special editor screen
                    _editor_status.screenMode = screenModeInstrumentEdit;
                }
                else {
                    // Toggle edit mode
                    _editor_status.editMode = ! _editor_status.editMode;
                }
                drawActiveScreen();
                break;
            }
        case ctlButtonMenu:
            // return back to the main screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler to System Param Editor mode
void TesMidiUI::processCtlButtonEventSystemParamEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonOk:
            if((_editor_status.sysParSelector + _editor_status.sysParTopIndex) == parGyroId){
                // enter Gyro Editor
                _editor_status.screenMode = screenModeGyroEdit;
                // *** ATTENTION! each time we change screens - we turn off the edit mode
                _editor_status.editMode = false;
                drawActiveScreen();
            }
            break;
        case ctlButtonUp:
            // move up
            if(_editor_status.sysParSelector == 0){
                // the first editor row
                // Scroll the list down until topIndex == 0
                if(_editor_status.sysParTopIndex > 0){
                    // scrolling is done by decrementing the topIndex value
                    _editor_status.sysParTopIndex--;
                }
                else{
                    // The beginning of the parameter list is reached.
                    // Navigate to the very end of the list:
                    // - set sysParTopIndex to parNumberOfParameters  - paramEditorRows
                    // - set sysParSelector to paramEditorRows-1 as well
                    _editor_status.sysParTopIndex = parNumberOfParameters - paramEditorRows;
                    _editor_status.sysParSelector = paramEditorRows - 1;
                }
            }
            else{
                _editor_status.sysParSelector--;
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            // move down
            if(_editor_status.sysParSelector == (paramEditorRows - 1)){
                // the last editor row
                // Scroll the list up until sysParTopIndex+sysParSelector == parNumberOfParameters-1
                if((_editor_status.sysParTopIndex + _editor_status.sysParSelector) < (parNumberOfParameters - 1)){
                    // scrolling is done by incrementing the topIndex value
                    _editor_status.sysParTopIndex++;
                }
                else{
                    // The end of the parameter list is reached.
                    // Navigate to the beginning of the list:
                    // - set sysParTopIndex to 0 in order to start from the 1st parameter
                    // - set sysParSelector to 0 as well
                    _editor_status.sysParTopIndex = 0;
                    _editor_status.sysParSelector = 0;
                }
            }
            else{
                _editor_status.sysParSelector++;
            }
            drawActiveScreen();
            break;
        case ctlButtonLeft:
        case ctlButtonRight:
            switch(_editor_status.sysParSelector + _editor_status.sysParTopIndex){
            case parPresetPageId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.global.activePresetPage-- == 0){
                        _mc->_settings.global.activePresetPage = parPresetPageMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.global.activePresetPage++ == parPresetPageMaxValue){
                        _mc->_settings.global.activePresetPage = 0;
                    }
                }
                // activate the current preset from the new preset page
                _mc->_var.activePresetPageChanged = true;                   // this will make the MIDI Controller reload
                                                                            // the current preset
                activatePreset(_mc->_settings.global.activePreset);
                _mc->_var.activePresetPageChanged = false;                  // reset the flag
                // function _mc->globalSettingsChangeNotification() was already called from activatePreset()
                break;
            case parRunningStatusId:
                // toggle the value
                _mc->_settings.global.runningStatus = ! _mc->_settings.global.runningStatus;
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parBassOctavesId:
                // toggle the value
                _mc->_settings.preset.bassOctavesOn = ! _mc->_settings.preset.bassOctavesOn;
                // notify the controller about preset change
                indicatePresetChange();
                break;
            case parGyroId:
                // toggle the value
                _mc->_settings.preset.gyroOn = ! _mc->_settings.preset.gyroOn;
                // turn the Gyro on/off
                _mc->turnGyroOnOff(_mc->_settings.preset, _mc->_settings.preset.gyroOn);
                // notify the controller about preset change
                indicatePresetChange();
                break;
            case parSynthTypeId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.global.synthType-- == 0){
                        _mc->_settings.global.synthType = parSynthTypeN - 1;
                    }
                }
                else {      // ctlButtonRight
                    if (++_mc->_settings.global.synthType == parSynthTypeN){
                        _mc->_settings.global.synthType = 0;
                    }
                }
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parMasterVolumeId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.masterVolume-- == 0){
                        _mc->_settings.preset.masterVolume = parMasterVolumeMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.masterVolume++ == parMasterVolumeMaxValue){
                        _mc->_settings.preset.masterVolume = 0;
                    }
                }
                // set the new Volume
                _mc->setVolume(_mc->_settings.preset.masterVolume);
                // set new Volume for Drums in any case (drums do not depend on Pressure Sensor)
                _mc->setDrumsVolume();
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case parDrumsChannelId:
                {
                    bool new_value_is_ok;
                    do {
                        new_value_is_ok = true;
                        if (event->buttonId == ctlButtonLeft){
                            if (_mc->_settings.global.drumsMidiChannel-- == 0){
                                _mc->_settings.global.drumsMidiChannel = parDrumsChannelMaxValue;
                            }
                        }
                        else {      // ctlButtonRight
                            if (_mc->_settings.global.drumsMidiChannel++ == parDrumsChannelMaxValue){
                                _mc->_settings.global.drumsMidiChannel = 0;
                            }
                        }
                        // check if the new value is not the same as any other configured channel
                        for(int i=0; i<NUMBER_OF_KEYBOARDS; i++){
                            if(_mc->_settings.global.drumsMidiChannel == _mc->_settings.preset.kbdParameter[i][idxChannel]){
                                new_value_is_ok = false;
                                break;
                            }
                        }
                        if(new_value_is_ok){
                            new_value_is_ok = (_mc->_settings.global.drumsMidiChannel != _mc->_settings.global.drumsAdditionalChannel);
                        }
                    }while (!new_value_is_ok);
                }
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parDrumsAdditionalChannelId:
                {
                    bool new_value_is_ok;
                    do {
                        new_value_is_ok = true;
                        if (event->buttonId == ctlButtonLeft){
                            if (_mc->_settings.global.drumsAdditionalChannel-- == 0){
                                _mc->_settings.global.drumsAdditionalChannel = parDrumsAdditionalChannelMaxValue;
                            }
                        }
                        else {      // ctlButtonRight
                            if (_mc->_settings.global.drumsAdditionalChannel++ == parDrumsAdditionalChannelMaxValue){
                                _mc->_settings.global.drumsAdditionalChannel = 0;
                            }
                        }
                        // check if the new value is not the same as any other configured channel
                        for(int i=0; i<NUMBER_OF_KEYBOARDS; i++){
                            if(_mc->_settings.global.drumsAdditionalChannel == _mc->_settings.preset.kbdParameter[i][idxChannel]){
                                new_value_is_ok = false;
                                break;
                            }
                        }
                        if(new_value_is_ok){
                            new_value_is_ok = (_mc->_settings.global.drumsMidiChannel != _mc->_settings.global.drumsAdditionalChannel);
                        }
                    }while (!new_value_is_ok);
                }
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parInactivityPeriodId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.global.inactivityPeriod-- == 0){
                        _mc->_settings.global.inactivityPeriod = parInactivityPeriodMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.global.inactivityPeriod++ == parInactivityPeriodMaxValue){
                        _mc->_settings.global.inactivityPeriod = 0;
                    }
                }
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parBuildId:
                // do nothing
                break;
            default:
                SWER(swerGUI14);
            }
            drawActiveScreen();
            break;
        case ctlButtonMenu:
            // return back to the main screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler to Drums Param Editor mode
void TesMidiUI::processCtlButtonEventDrumsParamEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonUp:
            // move up
            if( _editor_status.drumsParSelector-- == 0){
                _editor_status.drumsParSelector = dpEditorRows - 1;
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            // move down
            if( ++_editor_status.drumsParSelector == dpEditorRows){
                _editor_status.drumsParSelector = 0;
            }
            drawActiveScreen();
            break;
        case ctlButtonLeft:
        case ctlButtonRight:
            switch(_editor_status.drumsParSelector){
            case dpDrumsVolume:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.drumsVolume-- == 0){
                        _mc->_settings.preset.drumsVolume = dpDrumsVolumeMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.drumsVolume++ == dpDrumsVolumeMaxValue){
                        _mc->_settings.preset.drumsVolume = 0;
                    }
                }
                _mc->processChangedDrumsParameter(dpDrumsVolume);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case dpDrumsetNumber:
                if((_mc->_settings.global.synthType == stAtemp) || (_mc->_settings.global.synthType == stProDX)) {
                    // Drumset ID for Atemp synth - is a special case
                    _mc->_settings.preset.drumsetNumber = getNextAtempDrumsetId( event->buttonId == ctlButtonRight );
                }
                else {
                    if (event->buttonId == ctlButtonLeft){
                        if (_mc->_settings.preset.drumsetNumber-- == 0){
                            _mc->_settings.preset.drumsetNumber = dpDrumsetNumberMaxValue;
                        }
                    }
                    else {      // ctlButtonRight
                        if (_mc->_settings.preset.drumsetNumber++ == dpDrumsetNumberMaxValue){
                            _mc->_settings.preset.drumsetNumber = 0;
                        }
                    }
                }
                _mc->processChangedDrumsParameter(dpDrumsetNumber);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case dpSoundForBass:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.drumSoundForBass-- == dpDrumSoundForBassMinValue){
                        _mc->_settings.preset.drumSoundForBass = dpDrumSoundForBassMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.drumSoundForBass++ == dpDrumSoundForBassMaxValue){
                        _mc->_settings.preset.drumSoundForBass = dpDrumSoundForBassMinValue;
                    }
                }
                _mc->processChangedDrumsParameter(dpSoundForBass);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case dpSoundForBassVelocity:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.drumSoundForBassVelocity-- == 0){
                        _mc->_settings.preset.drumSoundForBassVelocity = dpDrumSoundForBassVelocityMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.drumSoundForBassVelocity++ == dpDrumSoundForBassVelocityMaxValue){
                        _mc->_settings.preset.drumSoundForBassVelocity = 0;
                    }
                }
                _mc->processChangedDrumsParameter(dpSoundForBassVelocity);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case dpSoundForChord:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.drumSoundForChord-- == dpDrumSoundForChordMinValue){
                        _mc->_settings.preset.drumSoundForChord = dpDrumSoundForChordMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.drumSoundForChord++ == dpDrumSoundForChordMaxValue){
                        _mc->_settings.preset.drumSoundForChord = dpDrumSoundForChordMinValue;
                    }
                }
                _mc->processChangedDrumsParameter(dpSoundForChord);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case dpSoundForChordVelocity:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.drumSoundForChordVelocity-- == 0){
                        _mc->_settings.preset.drumSoundForChordVelocity = dpDrumSoundForChordVelocityMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.drumSoundForChordVelocity++ == dpDrumSoundForChordVelocityMaxValue){
                        _mc->_settings.preset.drumSoundForChordVelocity = 0;
                    }
                }
                _mc->processChangedDrumsParameter(dpSoundForChordVelocity);
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            }
            drawActiveScreen();
            break;
        case ctlButtonMenu:
            // return back to the main screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler to Instrument Editor mode
void TesMidiUI::processCtlButtonEventInstrumentEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event
                                        //
    // set aux valiables up
    const uint8_t * _GroupMap       = NULL;
    const uint8_t * const * _Banks  = NULL;
    uint8_t _NumberOfGroups         = 0;
    switch(_mc->_settings.global.synthType){
        case stProDX:
            _GroupMap           = dxGroupMap;
            _Banks              = dxBanks;
            _NumberOfGroups     = dxNumberOfGroups;
            break;
        case stAtemp:
            _GroupMap           = atGroupMap;
            _Banks              = atBanks;
            _NumberOfGroups     = atNumberOfGroups;
            break;
        default:
            SWER(swerGUI12);
    }

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonUp:
            // move up
            if (_editor_status.instrumentSelector-- == 0){
                _editor_status.instrumentSelector = ieNumberOfParameters - 1;
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            // move down
            if (++_editor_status.instrumentSelector == ieNumberOfParameters){
                _editor_status.instrumentSelector = 0;
            }
            drawActiveScreen();
            break;
        case ctlButtonLeft:
        case ctlButtonRight:
            switch(_editor_status.instrumentSelector){
            case ieGroupId:
              {
                // changing the group (moving to the 1st instrument in the new group)
                // get the id of the current group
                uint8_t program_id = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram];
                uint8_t group_id = getInstrumentGroupId(_GroupMap, program_id);
                if (event->buttonId == ctlButtonLeft){
                    // go to the previous group
                    if (group_id-- == 0){
                        group_id = _NumberOfGroups - 1;
                    }
                }
                else {      // ctlButtonRight
                    // go to the next group
                    if (++group_id == _NumberOfGroups){
                        group_id = 0;
                    }
                }
                // get the 1st program_id for the new group_id
                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram] =
                    (group_id == 0) ? 0 : (_GroupMap[group_id -1] + 1);
                // idxBank for a new group_id is always 0
                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxBank] = 0;
                // notify the Midi Controller properly about the change
                _mc->processChangedKbdParameter(
                        _editor_status.selectorX,
                        idxBank                     // this will cause sending both idxBank & idxProgram
                        );
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
              }
            case ieInstrumentId:
                setNextInstrument(
                        _Banks,
                        &(_mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxBank]),
                        &(_mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram]),
                        (event->buttonId == ctlButtonRight)
                        );
                // notify the MIDI controller about the change
                _mc->processChangedKbdParameter(
                        _editor_status.selectorX,
                        idxBank                     // this will cause sending both idxBank & idxProgram
                        );
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            }
            drawActiveScreen();
            break;
        case ctlButtonOk:
            // return back to the Param Editor screen
            _editor_status.screenMode = screenModeMidiParamEdit;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        case ctlButtonMenu:
            // return back to the main screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler for Melody Selector mode
void TesMidiUI::processCtlButtonEventMelodySelector(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    uint8_t actual_editor_rows  = min(msEditorRows, _mc->_auto_drums.getNumberOfMelodies());

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonUp:
            // move up
            // move to the previous row
            if( _editor_status.msSelector == 0){
                // the first editor Row
                // Scroll the list down until topIndex == 0
                if((_editor_status.msTopIndex) > 0){
                    // scrolling is done by decrementing the topIndex value
                    _editor_status.msTopIndex--;
                }
                else{
                    // The beginning of the parameter list is reached.
                    // Navigate to the very end of the list:
                    // - set topIndex to theNumberOfMidiParameters - editorRows
                    // - set selectorY to editorRows-1 as well
                    _editor_status.msTopIndex = _mc->_auto_drums.getNumberOfMelodies() - actual_editor_rows;
                    _editor_status.msSelector = actual_editor_rows-1;
                }
            }
            else{
                _editor_status.msSelector--;
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            // move down
            // move to the next row
            if( _editor_status.msSelector == actual_editor_rows-1){
                // the last editor Row
                // Scroll the list up until topIndex+msSelector == theNumberOfMidiParameters-1
                if((_editor_status.msSelector + _editor_status.msTopIndex) < _mc->_auto_drums.getNumberOfMelodies()-1){
                    // scrolling is done by incrementing the topIndex value
                    _editor_status.msTopIndex++;
                }
                else{
                    // The end of the parameter list is reached.
                    // Navigate to the beginning of the list:
                    // - set topIndex to 0 in order to start from the 1st parameter
                    // - set selectorY to 0 as well
                    _editor_status.msTopIndex = 0;
                    _editor_status.msSelector = 0;
                }
            }
            else{
                _editor_status.msSelector++;
            }
            drawActiveScreen();
            break;
        case ctlButtonOk:
            {
                bool was_playing = _mc->_auto_drums.isPlaying();
                if ( was_playing ){
                    _mc->_auto_drums.stop();
                }
                _mc->_auto_drums.setMelodyId(_editor_status.msTopIndex + _editor_status.msSelector);
                if ( was_playing ){
                    _mc->_auto_drums.start();
                }
            }
            // *** ATTENTION! Control goes to the case below! Don't put "break" here!
        case ctlButtonMenu:
            // return back to the main screen
            _editor_status.screenMode = screenModeMain;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
        break;
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

static bool twoCtrlsAreOk(uint8_t ctrl1, uint8_t ctrl2){
    bool pitch_clash = ((ctrl1 == gyroCtrlPitchUp) || (ctrl1 == gyroCtrlPitchDown)) 
        && ((ctrl2 == gyroCtrlPitchUp) || (ctrl2 == gyroCtrlPitchDown));
    return (!pitch_clash) && (ctrl1 != ctrl2);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler for Melody Selector mode
void TesMidiUI::processCtlButtonEventGyroEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
    case tesBeStep:
        switch (event->buttonId){
        case ctlButtonUp:
            // move up
            if( _editor_status.gyroParSelector-- == 0){
                _editor_status.gyroParSelector = gpEditorRows - 1;
            }
            drawActiveScreen();
            break;
        case ctlButtonDown:
            // move down
            if( ++_editor_status.gyroParSelector == gpEditorRows){
                _editor_status.gyroParSelector = 0;
            }
            drawActiveScreen();
            break;
        case ctlButtonLeft:
        case ctlButtonRight:
            // *** the approach is simpified: we just:
            //  - turn Gyro off, if required
            //  - change the Gyro parameter
            //  - turn Gyro on again with new settings (if required)
            if(_mc->_settings.preset.gyroOn){
                _mc->turnGyroOnOff(_mc->_settings.preset, false);
            }
            switch(_editor_status.gyroParSelector){
            case gpModeId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.gyroMode-- == 0){
                        _mc->_settings.preset.gyroMode = gpModeMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.gyroMode++ == gpModeMaxValue){
                        _mc->_settings.preset.gyroMode = 0;
                    }
                }
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case gpXCtrlId:
                do {
                    if (event->buttonId == ctlButtonLeft){
                        if (_mc->_settings.preset.gyroXCtrl-- == 0){
                            _mc->_settings.preset.gyroXCtrl = gpXCtrlMaxValue;
                        }
                    }
                    else {      // ctlButtonRight
                        if (_mc->_settings.preset.gyroXCtrl++ == gpXCtrlMaxValue){
                            _mc->_settings.preset.gyroXCtrl = 0;
                        }
                    }
                } while ( !twoCtrlsAreOk(_mc->_settings.preset.gyroXCtrl, _mc->_settings.preset.gyroYCtrl) );
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case gpYCtrlId:
                do {
                    if (event->buttonId == ctlButtonLeft){
                        if (_mc->_settings.preset.gyroYCtrl-- == 0){
                            _mc->_settings.preset.gyroYCtrl = gpYCtrlMaxValue;
                        }
                    }
                    else {      // ctlButtonRight
                        if (_mc->_settings.preset.gyroYCtrl++ == gpYCtrlMaxValue){
                            _mc->_settings.preset.gyroYCtrl = 0;
                        }
                    }
                } while ( !twoCtrlsAreOk(_mc->_settings.preset.gyroXCtrl, _mc->_settings.preset.gyroYCtrl) );
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case gpPitchDepthId:
                if (event->buttonId == ctlButtonLeft){
                    if (_mc->_settings.preset.gyroPitchDepth-- == gpPitchDepthMinValue){
                        _mc->_settings.preset.gyroPitchDepth = gpPitchDepthMaxValue;
                    }
                }
                else {      // ctlButtonRight
                    if (_mc->_settings.preset.gyroPitchDepth++ == gpPitchDepthMaxValue){
                        _mc->_settings.preset.gyroPitchDepth = gpPitchDepthMinValue;
                    }
                }
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            default:
                SWER(swerGUI16);
            }
            // and turn Gyro on again, if required
            if(_mc->_settings.preset.gyroOn){
                _mc->turnGyroOnOff(_mc->_settings.preset, true);
            }
            drawActiveScreen();
            break;
        case ctlButtonMenu:
            // return back to the System Parameter Editor screen
            _editor_status.screenMode = screenModeSysParamEdit;
            // *** ATTENTION! each time we change screens - we turn off the edit mode
            _editor_status.editMode = false;
            drawActiveScreen();
            break;
        default:
            // not handled ctl button; don't clear the event
            needToClearTheEvent = false;
            break;
        }
    default:
        // not handled ctl button event; don't clear the event
        needToClearTheEvent = false;
        break;
    }
    if (needToClearTheEvent){
        event->eventType = tesEmpty;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Activates the specified preset
void TesMidiUI::activatePreset(uint8_t preset_id){
    if (((preset_id == _mc->_settings.global.activePreset) && ( ! _mc->_var.currentPresetNotSaved))
            && (! _mc->_var.activePresetPageChanged)){
        // there's nothonig to do
        return;
    }

    // clear indication of the old preset
    ledSet.resetLED( _mc->_settings.global.activePreset + ledPreset1);    // the assumption is that Led IDs are sorted (like 0, 1, 2, etc.)
    // activate the new preset
    _mc->activatePreset(preset_id);
    // indicate the active preset
    ledSet.setLED( _mc->_settings.global.activePreset + ledPreset1, 1);   // the assumption is that Led IDs are sorted (like 0, 1, 2, etc.)
    // redraw the active screen
    drawActiveScreen();
    // Active preset ID - is a part of global settings
    _mc->globalSettingsChangeNotification();
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the next/previous value for the bank ID
uint8_t TesMidiUI::getNextAtempBankId(uint8_t kbd_id, bool forward){
    uint8_t curBankId = _mc->_settings.preset.kbdParameter[kbd_id][idxBank];
    uint8_t curProgram = _mc->_settings.preset.kbdParameter[kbd_id][idxProgram];

    // get a pointer to the list of available bank IDs for the current program
    const uint8_t * ptr = NULL;
    switch (_mc->_settings.global.synthType){
    case stAtemp:
        ptr = atBanks[curProgram];
        break;
    case stProDX:
        ptr = dxBanks[curProgram];
        break;
    }
    if (ptr == NULL){
        // no other banks available; return the current one
        return curBankId;
    }
    // *** let's find the next bank id

    // get the total number of bank IDs
    uint8_t N = ptr[0];
    // find the index of the current bank
    uint8_t curIndex = 1;
    while ((curIndex <= N) && (ptr[curIndex] != curBankId)){
        curIndex++;
    }
    // now, the curIndex points to the current bank ID
    // get the new index
    if ( forward ){
        if (curIndex++ == N){
            curIndex = 1;
        }
    }
    else {
        if (--curIndex == 0){
            curIndex = N;
        }
    }
    // return the new bank id
    return ptr[curIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the next/previous value for the drumset ID
// (relative to the current value in _mc->_settings.preset.drumsetNumber)
uint8_t TesMidiUI::getNextAtempDrumsetId(bool forward){
    const uint8_t * map = NULL;
    switch(_mc->_settings.global.synthType){
    case stAtemp:
        map = atDrumSets;
        break;
    case stProDX:
        map = dxDrumSets;
        break;
    }
    // get the total number of drumsets
    uint8_t N = map[0];
    // find the index of the current bank
    uint8_t curIndex = 1;
    while ((curIndex <= N) && (map[curIndex] != _mc->_settings.preset.drumsetNumber)){
        curIndex++;
    }
    // now, the curIndex points to the current bank ID
    // get the new index
    if ( forward ){
        if (curIndex++ == N){
            curIndex = 1;
        }
    }
    else {
        if (--curIndex == 0){
            curIndex = N;
        }
    }
    // return the new bank id
    return map[curIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Adjusts bank ID to be is allowed for the specified Program and to be the closest to the current
// bank ID.
// - Returns True if the bank ID was changed.
bool TesMidiUI::adjustBankId(uint8_t kbd_id){
    // we do this only for ATEMP synth
    if ((_mc->_settings.global.synthType != stAtemp)&&(_mc->_settings.global.synthType != stProDX)){
        // in this case, we never adjust bank id
        return false;
    }

    uint8_t curBankId = _mc->_settings.preset.kbdParameter[kbd_id][idxBank];
    uint8_t curProgram = _mc->_settings.preset.kbdParameter[kbd_id][idxProgram];
    bool    bank_id_was_changed = false;
    uint8_t new_bank_id = 0;

    // get a pointer to the list of available bank IDs for the current program
    const uint8_t * ptr = NULL;
    switch (_mc->_settings.global.synthType){
    case stAtemp:
        ptr = atBanks[curProgram];
        break;
    case stProDX:
        ptr = dxBanks[curProgram];
        break;
    }
    if (ptr == NULL){
        // There's just one available bank id: 0
        // new_bank_id is already set to 0 (above)
    }
    else {
        // *** let's find the best new_bank_id
        // get the total number of bank IDs
        uint8_t N = ptr[0];
        // find a bank id closest to curBankId
        uint8_t diff = 200;
        for(int i = 1; i<=N; i++){
            int b = ptr[i];
            uint8_t d = abs(curBankId - b);
            if (d == 0){
                // the best possible match
                new_bank_id = b;
                break;
            }
            else if (d < diff){
                diff = d;
                new_bank_id = b;
            }
        }
    }
    // *** Here we have new_bank_id set to the best value
    // check if it differs from the current one
    if (new_bank_id != curBankId){
        // adjust the bank id
        _mc->_settings.preset.kbdParameter[kbd_id][idxBank] = new_bank_id;
        // set the flag
        bank_id_was_changed = true;
    }

    return bank_id_was_changed;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Starts indicating a not saved preset
void TesMidiUI::indicatePresetChange(void){
    // check if this is the 1st change of the current preset
    if ( ! _mc->_var.currentPresetNotSaved ){
        // make the respective LED blinking
        ledSet.startBlink(_mc->_settings.global.activePreset + ledPreset1, teslpSlowBlink);
        // set the "preset not saved" flag
        _mc->_var.currentPresetNotSaved = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Sets Melody Selector's variables up, so that they point to the current melody.
void TesMidiUI::initMelodySelector(void){
    uint8_t actual_editor_rows = min(msEditorRows, _mc->_auto_drums.getNumberOfMelodies());
    uint8_t melody_index = _mc->_auto_drums.getCurrentMelodyId();

    if(melody_index < actual_editor_rows){
        _editor_status.msTopIndex = 0;
        _editor_status.msSelector = melody_index;
    }
    else if(melody_index >= (_mc->_auto_drums.getNumberOfMelodies() - actual_editor_rows)){
        _editor_status.msTopIndex = _mc->_auto_drums.getNumberOfMelodies() - actual_editor_rows;
        _editor_status.msSelector = melody_index - _editor_status.msTopIndex;
    }
    else {
        _editor_status.msSelector = actual_editor_rows / 2;
        _editor_status.msTopIndex = melody_index - _editor_status.msSelector;
    }
}


#define EASTER_EGG_WIDTH    64
static const uint8_t easter_egg_64x64[] = {
	0x95, 0x6B, 0x15, 0x42, 0xD5, 0x08, 0xD5, 0x22, 0xCD, 0x20, 0xAB, 0x54, 0xA2, 0x5C, 0xA0, 0x1F, 0x40, 0xAF, 0x50, 0xA5, 0x12, 0xA9, 0x44, 0x91, 0x24, 0x4B, 0x00, 0x94, 0x2B, 0x44, 0xAA, 0x11, 0xEE, 0x10, 0xA7, 0x58, 0xA5, 0x5A, 0x85, 0x78, 0xA7, 0xC8, 0xB5, 0xC0, 0x15, 0x82, 0x2D, 0xD2, 0x2D, 0x72, 0xCD, 0x12, 0xCD, 0x12, 0x54, 0xA5, 0x08, 0xB5, 0x4A, 0x10, 0x85, 0x28, 0x86, 0x50, 
	0x08, 0x33, 0x84, 0x49, 0x36, 0x49, 0x16, 0xC9, 0x32, 0x45, 0x9A, 0x65, 0x0A, 0x50, 0x02, 0x95, 0x40, 0x8A, 0x30, 0xC6, 0x18, 0x66, 0x99, 0x62, 0x9D, 0x22, 0x50, 0x84, 0x10, 0xA5, 0x08, 0xA3, 0x4C, 0x91, 0x66, 0x91, 0x6E, 0xD0, 0x6D, 0xBB, 0xFE, 0xB7, 0xFE, 0xEF, 0xBD, 0xFA, 0xE5, 0x12, 0xED, 0x13, 0xAE, 0x51, 0x6A, 0xD0, 0x25, 0xCA, 0x30, 0xC6, 0x99, 0x26, 0xD9, 0x56, 0xAB, 0x56, 
	0x09, 0xE6, 0x19, 0xC6, 0x18, 0xA3, 0x54, 0x28, 0x25, 0x4A, 0xF0, 0x0B, 0x54, 0xA1, 0x08, 0x52, 0x84, 0x30, 0x45, 0x9A, 0x21, 0x4A, 0x94, 0x22, 0x54, 0x03, 0x54, 0x89, 0x12, 0xA4, 0x49, 0x92, 0x24, 0x4A, 0xB5, 0x4A, 0x95, 0x6A, 0x9D, 0x63, 0x9F, 0xEA, 0x37, 0xFA, 0xAF, 0xFA, 0x5F, 0xF7, 0xA8, 0x55, 0x2A, 0xE9, 0xBE, 0x7B, 0xDD, 0x3B, 0xB6, 0xC9, 0x56, 0xE9, 0x45, 0xD2, 0xA9, 0x46, 
	0x4A, 0x90, 0x26, 0xD9, 0x04, 0x73, 0x94, 0x69, 0x87, 0x78, 0x85, 0x2B, 0x80, 0x1A, 0xE1, 0x1E, 0xE1, 0x1A, 0xA5, 0x58, 0x25, 0x0A, 0x54, 0x01, 0x56, 0x01, 0x96, 0x29, 0x82, 0x0C, 0x33, 0x44, 0x9B, 0x24, 0x4A, 0x95, 0x2A, 0x55, 0x8E, 0x3B, 0x46, 0xBD, 0x2B, 0x57, 0xAE, 0x7D, 0xAB, 0xEF, 0xBA, 0x05, 0xDA, 0xB5, 0xB2, 0x5C, 0xE2, 0xAD, 0x76, 0xED, 0x93, 0x6D, 0x93, 0x6C, 0x93, 0x24, 
	0x44, 0x49, 0x92, 0xAC, 0x91, 0xE6, 0x49, 0xF4, 0x23, 0xD4, 0x32, 0x0E, 0xB0, 0x47, 0xB8, 0x57, 0x28, 0x56, 0x00, 0x54, 0x01, 0x54, 0x01, 0x44, 0x90, 0x02, 0x88, 0x20, 0x44, 0x90, 0x21, 0x8A, 0x64, 0x58, 0xE5, 0x58, 0xA0, 0xC0, 0x2A, 0xC0, 0x14, 0x80, 0x2D, 0xD2, 0xAC, 0x5D, 0xAA, 0xFD, 0xAB, 0x5F, 0x2A, 0x9B, 0x7A, 0xAB, 0x75, 0xAD, 0xF7, 0x6C, 0xD3, 0x7E, 0xC1, 0xBD, 0xE6, 0x9D, 
	0x2F, 0xD5, 0x2B, 0x16, 0x6B, 0x96, 0x29, 0x45, 0x0A, 0x10, 0x44, 0x09, 0x22, 0x04, 0x19, 0x67, 0x90, 0x25, 0x92, 0x25, 0x48, 0x15, 0x82, 0x28, 0x02, 0x54, 0x00, 0x21, 0x54, 0x8A, 0x04, 0x29, 0x42, 0x0D, 0x53, 0xAF, 0xDC, 0x29, 0xD2, 0x25, 0xC9, 0x17, 0x69, 0x96, 0x7B, 0xCC, 0x1B, 0x76, 0x89, 0x25, 0x40, 0x0A, 0x01, 0x52, 0x05, 0xAA, 0x05, 0x5B, 0x27, 0x9D, 0x2F, 0x5A, 0xB7, 0x5D, 
	0x05, 0x00, 0x55, 0x00, 0x09, 0xA0, 0x02, 0x08, 0x21, 0x84, 0x10, 0x01, 0x24, 0x00, 0x11, 0x80, 0x02, 0x24, 0x8A, 0x10, 0xA5, 0x00, 0xAA, 0x00, 0x49, 0x10, 0xA5, 0x08, 0xA2, 0x08, 0x41, 0x14, 0x80, 0x51, 0x86, 0x18, 0xE3, 0x1C, 0x61, 0x88, 0x52, 0x21, 0x4C, 0x13, 0x4C, 0x11, 0xA7, 0x08, 0x00, 0x2D, 0x40, 0x15, 0x80, 0x09, 0x20, 0x8A, 0x40, 0x11, 0x04, 0x10, 0x42, 0x14, 0x01, 0x54, 
	0x00, 0x49, 0x00, 0x02, 0x10, 0x44, 0x00, 0x09, 0x40, 0x04, 0x20, 0x00, 0x89, 0x00, 0x00, 0x20, 0x04, 0x00, 0x90, 0x02, 0x04, 0x08, 0xA2, 0x10, 0x85, 0x20, 0x8A, 0x00, 0xAA, 0x00, 0x55, 0x00, 0x8A, 0x20, 0x83, 0x2C, 0x83, 0x24, 0x4B, 0x10, 0x85, 0x28, 0x02, 0x80, 0x10, 0x42, 0x14, 0x81, 0x20, 0x04, 0x11, 0x44, 0x00, 0x22, 0x88, 0x00, 0x92, 0x00, 0x41, 0x14, 0x00, 0x29, 0x82, 0x51, 
};

const uint8_t easter_egg_128x64[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xE0, 0x80, 0x00, 0x00, 0x00, 0xE0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0xE0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x78, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x01, 0x0F, 0x3E, 0xF0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0x80, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xF0, 0xF0, 0xC0, 0x80, 0x83, 0x0F, 0x1E, 0x38, 0x70, 0xE0, 0xC0, 0x00, 0x00, 0x03, 0x0F, 0x3C, 0x78, 0xE0, 0x80, 0x00, 0x01, 0x0F, 0x7C, 0xF0, 0x80, 0x00, 0x00, 0x1F, 0xFF, 0xC0, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xE0, 0x93, 0x0F, 0x07, 0xC3, 0x67, 0x37, 0xBF, 0xDF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF8, 0xF8, 0xF0, 0xC1, 0xC7, 0x8E, 0x9C, 0x38, 0x70, 0xE0, 0x81, 0x07, 0x0E, 0x3C, 0x70, 0x40, 0x01, 0x0F, 0x1E, 0x10, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x01, 0x03, 0x06, 0x0C, 0x18, 0x31, 0x63, 0xC1, 0x88, 0x0E, 0x67, 0x33, 0x3F, 0x1F, 0x0F, 0x9F, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0x7D, 0x33, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x04, 0x06, 0x06, 0x03, 0x01, 0x01, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Easter Egg
void TesMidiUI::drawEasterEgg(void){
    oled.clear();
    // just draw the picture to the whole screen
    oled.drawBitmap((128/2 - EASTER_EGG_WIDTH/2 - 1), 0, easter_egg_64x64, EASTER_EGG_WIDTH, 64, BITMAP_NORMAL, BUF_REPLACE);
    oled.update();
}
