#include <Arduino.h>
#include "TesMidiController.h"
#include "TesEventQueue.h"
#include "TesKeyboard.h"
#include "TesMidiUI.h"
#include "swer.h"
#include "midi_note_names.h"
#include "prodx.h"
//#include "atemp_hw.h"

#define FPSTR(pstr) (const __FlashStringHelper*)(pstr)

// period for updating the display (it takes too much time to update it in each work cycle)
#define TES_DISPLAY_UPDATE_PERIOD   (1000 / 3)

#define INACTIVITY_PERIOD       10000    // 10 seconds

#define screenModeMain              0
#define screenModeMidiParamEdit     1
#define screenModeSysParamEdit      2
#define screenModeDrumsEdit         3
#define screenModeInstrumentEdit    4

///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesMidiUI::TesMidiUI(GyverOLED<SSH1106_128x64> * oled, TesMIDIController * mc, TesLedSet * led){
    _oled = oled;
    _mc = mc;
    _led_set = led;
    
    memset(&_editor_status, 0, sizeof(_editor_status));
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the instance.
// Should be called from setup()
void    TesMidiUI::init(void){
    // initialize the display
    _oled->init();
    Wire.setClock(800000L); // Maximum speed

    // draw the active screen
    drawActiveScreen();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// standard tick() method
void TesMidiUI::tick(void){
    uint16_t    millis_snapshot = millis();

    // check the display update timer
    if (millis_snapshot - _display_update_timer > TES_DISPLAY_UPDATE_PERIOD){
        // update the display if required
        if ( _flags.display_update_is_required ){
            _oled->update();    // partial update doesn't work for SSH1106 yet :(
            // reset the display update request
            _flags.display_update_is_required = false;
        }
        // restart the timer
        _display_update_timer = millis();
    }

    // check user activity
    if (_editor_status.screenMode != screenModeMain){
        if (millis_snapshot - _inactivity_timer > INACTIVITY_PERIOD){
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
            _led_set->resetLED(_preset_sm.writePreset + ledPreset1);
            _led_set->setLED(_preset_sm.writePreset + ledPreset1, (_preset_sm.writePreset == _mc->_settings.global.activePreset) );
        }
    }

    // check F-key write confirmation timer
    if (_fkey_sm.state == psWriteConfirmation){
        if (millis_snapshot - _fkey_sm.writeTimer > fKeyWriteConfirmationPeriod){
            // the user has confirmed writing of the F-key

            // return back to the normal mode
            _fkey_sm.state = psNormal;
            // save the F-key settings
            memcpy(&_mc->_settings.global.fkeyProfile[_fkey_sm.writeFKey].editorStatus, &_editor_status, sizeof(_editor_status));
            // notify the controller about global settings change
            _mc->globalSettingsChangeNotification();

            // stop blinking
            _led_set->resetLED(ledSystem);
            // and flash once
            _led_set->flashLED(ledSystem);
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
            if (event->buttonEvent == tesBeReleased){
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
                    // check if this was a long press
                    if (_flags.menu_hold_detected){
                        // long press starts system parameters editor
                        _editor_status.screenMode = screenModeSysParamEdit;
                    }
                    else {
                        _editor_status.screenMode = screenModeMidiParamEdit;
                        // *** ATTENTION! each time we change screens - we turn off the edit mode
                        _editor_status.editMode = false;
                    }
                    _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    drawActiveScreen();
                    // reset the long press flag (because we have just received the "released" event)
                    _flags.menu_hold_detected = false;
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonDrumsControl:
                    if (_flags.drums_control_hold_detected){
                        // switch to the Manual mode
                        setDrumMachineState(dmManual);
                        // start drums parameters editing
                        _editor_status.screenMode = screenModeDrumsEdit;
                        // *** ATTENTION! each time we change screens - we turn off the edit mode
                        _editor_status.editMode = false;
                        _inactivity_timer = millis();   // set inactivity timer for a non-Main screen
                    }
                    else {
                        switch(_mc->_var.drumsMode){
                        case dmOff:
                            setDrumMachineState(dmManual);
                            break;
                        case dmManual:
                            setDrumMachineState(dmStandby);
                            break;
                        case dmStandby:
                            setDrumMachineState(dmPlaying);
                            break;
                        case dmPlaying:
                            setDrumMachineState(dmOff);
                            break;
                        }
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    _flags.drums_control_hold_detected = false;
                    drawActiveScreen();
                    break;
                }
            }
            else if (event->buttonEvent == tesBeHold){
                switch(event->buttonId){
                case ctlButtonMenu:
                    // register the long pressed key
                    _flags.menu_hold_detected = true;
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
                    break;
                case ctlButtonDrumsControl:
                    // register the long pressed key
                    _flags.drums_control_hold_detected = true;
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
            case ctlButtonDrumsStyle: {
                bool was_playing = _mc->_auto_drums.isPlaying();
                if ( was_playing ){
                    _mc->_auto_drums.stop();
                }
                _mc->_auto_drums.setNextMelodyId();
                if ( was_playing ){
                    _mc->_auto_drums.start();
                }
                drawActiveScreen();
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
                            _led_set->resetLED(preset_id + ledPreset1);
                            if (_mc->_var.currentPresetNotSaved && (_preset_sm.writePreset == _mc->_settings.global.activePreset)){
                                // the current preset is still not saved, and the LED should return to the slow blinking mode
                                _led_set->startBlink(preset_id + ledPreset1, teslpSlowBlink);
                            }
                            else {
                                _led_set->setLED(preset_id + ledPreset1, (_preset_sm.writePreset == _mc->_settings.global.activePreset) );
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
            case ctlButtonF1:
            case ctlButtonF2:
            case ctlButtonF3:
            case ctlButtonF4: {
                // check if F-keys can work in the current _editor_status.screenMode
                if (_editor_status.screenMode != screenModeDrumsEdit){
                    uint8_t fkey_id = event->buttonId - ctlButtonF1;
                    switch ( _fkey_sm.state ){
                        case psNormal: {
                            // check if the F-key can be activated now
                            if((_mc->_settings.global.fkeyProfile[fkey_id].editorStatus.screenMode == screenModeInstrumentEdit)
                                    && (_mc->_settings.global.synthType != stProDX)){
                                // we cannot activate this F-key with the current synth type
                                break;
                            }
                            // activate F-key
                            memcpy(&_editor_status, &_mc->_settings.global.fkeyProfile[fkey_id].editorStatus, sizeof(_editor_status));
                            drawActiveScreen();
                        }
                        break;
                        case psWriteConfirmation: {
                            // check if this button is for the F-key being processed
                            if (fkey_id == _fkey_sm.writeFKey){
                                // cancel the write confirmation mode, return to the normal mode
                                _fkey_sm.state = psNormal;
                                // stop blinking
                                _led_set->resetLED(ledSystem);
                            }
                        }
                        break;
                        default:
                            SWER(swerGUI07);    // incorrect state
                    }
                    // clear the event to indicate it has been fully processed
                    event->eventType = tesEmpty;
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
                    _led_set->startBlink( preset_id + ledPreset1 );
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
                if ((_editor_status.screenMode != screenModeDrumsEdit) && (_editor_status.screenMode != screenModeMain)){
                    uint8_t fkey_id = event->buttonId - ctlButtonF1;
                    // only one F-key may be stored at a time
                    if (_preset_sm.state == psNormal){
                        // start the confirmation timer
                        _fkey_sm.writeTimer = millis();
                        // change the state to "write confirmation"
                        _fkey_sm.state = psWriteConfirmation;
                        // remember the presetID being processed
                        _fkey_sm.writeFKey = fkey_id;
                        // start blinking the LED
                        _led_set->setLED( ledSystem, true );
                        _led_set->startBlink( ledSystem );
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
            _led_set->setLED(ledDrums, true);
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
            _led_set->resetLED(ledDrums);  // turn it off first
            _led_set->startBlink(ledDrums);
            // a melody is always ready to play
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
            _led_set->resetLED(ledDrums);       // reset possible blinking
            _led_set->setLED(ledDrums, true);   // turn it on, so the ti's the staring state of the LED
            _led_set->startBlink(ledDrums, teslpSlowBlink);
            // send settings of manual drums to the synth.
            _mc->sendDrumsSettings();
            // start the melody
            _mc->_auto_drums.start();
            break;
        case dmManual:
            // change the state
            _mc->_var.drumsMode = dmManual;
            // set the LED
            _led_set->resetLED(ledDrums);
            _led_set->setLED(ledDrums, true);
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
            _led_set->resetLED(ledDrums);  // turn it off
            break;
        case dmManual:
            // change the state
            _mc->_var.drumsMode = dmManual;
            // set the LED
            _led_set->resetLED(ledDrums);
            _led_set->setLED(ledDrums, true);
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

#define maxFormatDescriptorLength   8
#define maxFormattedStringLength    8
const char numFormat3R[] PROGMEM = "%3d";    // 3 digits, right alignment
const char auxFormat1[] PROGMEM = " (%3d)";  // 3 digits, right alignment

const char main_screen_footer[] PROGMEM = "MIDI баян v1.01";

// for the header
#define headerStartPosition 6
#define headerIncrement     4
#define headerSections      4
const char headerSection0[]  PROGMEM  = "Пр.";
const char headerSection1[]  PROGMEM  = "Бас";
const char headerSection2[]  PROGMEM  = "Акк";
const char headerSection3[]  PROGMEM  = "Выб";
const char * const  headerSection[] PROGMEM = {
    headerSection0, headerSection1, headerSection2, headerSection3
};

#define theNumberOfMidiParameters   14
// labels for Kbd Parameters
const char parLabel0[]  PROGMEM  = "Канал";
const char parLabel1[]  PROGMEM  = "Банк";
const char parLabel2[]  PROGMEM  = "Инстр.";
const char parLabel3[]  PROGMEM  = "Громк.";
const char parLabel4[]  PROGMEM  = "Вибр.";
const char parLabel5[]  PROGMEM  = "Рез-с";
const char parLabel6[]  PROGMEM  = "Атака";
const char parLabel7[]  PROGMEM  = "Спад";
const char parLabel8[]  PROGMEM  = "Тембр";
const char parLabel9[]  PROGMEM  = "Затух.";
const char parLabel10[] PROGMEM  = "Ревер.";
const char parLabel11[] PROGMEM  = "Хорус";
const char parLabel12[] PROGMEM  = "Р. тип";
const char parLabel13[] PROGMEM  = "Х. тип";
const char * const  parLabel[theNumberOfMidiParameters] PROGMEM = {
    parLabel0,  parLabel1,  parLabel2,  parLabel3,  parLabel4,
    parLabel5,  parLabel6,  parLabel7,  parLabel8,  parLabel9, 
    parLabel10, parLabel11, parLabel12, parLabel13
};
// Hints for Kbd Parameters
const char parHint0[]  PROGMEM  = "MIDI канал";
const char parHint1[]  PROGMEM  = "Банк инструментов";
const char parHint2[]  PROGMEM  = "Номер инструмента";
const char parHint3[]  PROGMEM  = "Громкость канала";
const char parHint4[]  PROGMEM  = "Вибрато";
const char parHint5[]  PROGMEM  = "Резонанс";
const char parHint6[]  PROGMEM  = "Атака";
const char parHint7[]  PROGMEM  = "Спад";
const char parHint8[]  PROGMEM  = "Тембр";
const char parHint9[]  PROGMEM  = "Затухание";
const char parHint10[] PROGMEM  = "Ревербрация";
const char parHint11[] PROGMEM  = "Хорус";
const char parHint12[] PROGMEM  = "Тип ревербрации";
const char parHint13[] PROGMEM  = "Тип хоруса";
const char * const  parHint[theNumberOfMidiParameters] PROGMEM = {
    parHint0,  parHint1,  parHint2,  parHint3,  parHint4,
    parHint5,  parHint6,  parHint7,  parHint8,  parHint9, 
    parHint10, parHint11, parHint12, parHint13
};
// Maximum possible values for Kbd Parameters
const uint8_t parMaxValue[theNumberOfMidiParameters] PROGMEM = {
    15, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 7, 7
};

// "Indicators" showing values of the MIDI controller local settings
#define indRow                  5
#define indNumber               4   // the number of implemented indicators for now

#define indBassOctavesPos       1
const char indBassOctavesLabel[] PROGMEM = "Окт";

#define indPressureSensorOnPos  7
const char indPressureSensorLabel[] PROGMEM = "Мех";

#define indSynthTypePos         13
const char indSynthTypeLabel0[]     PROGMEM  = "GM";    // General MIDI
const char indSynthTypeLabel1[]     PROGMEM  = "AT";    // ATemp
const char indSynthTypeLabel2[]     PROGMEM  = "DX";    // Atemp Pro.DX
const char indSynthTypeLabel3[]     PROGMEM  = "--";    // not used yet
const char * const  indSynthTypeLabel[] PROGMEM = {
    indSynthTypeLabel0, indSynthTypeLabel1, indSynthTypeLabel2, indSynthTypeLabel3 
};

#define indRunningStatusPos     18
const char indRunningStatusLabel[] PROGMEM = "RS";

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
const char parBooleanLabel0[]     PROGMEM  = "Нет";
const char parBooleanLabel1[]     PROGMEM  = "Да";
const char * const  parBooleanLabel[] PROGMEM = {
    parBooleanLabel0, parBooleanLabel1
};
#define parBooleanN             2       // the number of options

// list of parameter IDs
enum {parRunningStatusId = 0, parBassOctavesId, parSynthTypeId, parMasterVolumeId, parDrumsChannelId, parTest1, parTest2};

/////////////////////////////////////////////////////////////////
// specific data forn the Running Status parameter
const char parRunningStatusName[]   PROGMEM  = "Running Status";

/////////////////////////////////////////////////////////////////
// specific data forn the Bass Octaves parameter
const char parBassOctavesName[]   PROGMEM  = "Октавы в басах";

/////////////////////////////////////////////////////////////////
// specific data for the Synth Type parameter
// It shares the same text constants with the respective indicator
const char * const  parSynthTypeLabel[] PROGMEM = {
    indSynthTypeLabel0, indSynthTypeLabel1, indSynthTypeLabel2
};
const char parSynthTypeName[]       PROGMEM  = "Тип синтезатора";
#define parSynthTypeN           3       // the number of options

/////////////////////////////////////////////////////////////////
// specific data for the Master Volume parameter
const char parMasterVolumeName[]    PROGMEM  = "Общая громкость";
#define parMasterVolumeMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drums Channel parameter
const char parDrumsChannelName[]    PROGMEM  = "Канал ударных";
#define parDrumsChannelMaxValue 15

/////////////////////////////////////////////////////////////////
// common data

#define paramEditorFirstRow     1       // we start from this row
#define paramEditorRows         5       // 5 parameters in total
#define paramEditorNameColumn   0       // where we start printing names
#define paramEditorValueColumn  17      // where we start printing values

#define parNumberOfParameters   7
const char parTestName1[]    PROGMEM  = "T1";
const char parTestName2[]    PROGMEM  = "T2";
const char * const  parParameterNames[parNumberOfParameters] PROGMEM = {
    parRunningStatusName, parBassOctavesName, parSynthTypeName, parMasterVolumeName, parDrumsChannelName,
    parTestName1, parTestName2
};

const char sys_param_screen_footer[] PROGMEM = "Настройки системы";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the editor of drums parameters

/////////////////////////////////////////////////////////////////
// specific data for the Master Volume parameter
const char dpDrumsVolumeName[]    PROGMEM  = "Громк. ударных";
#define dpDrumsVolumeMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drumset Number parameter
const char dpDrumsetNumberName[]    PROGMEM  = "Набор звуков";
#define dpDrumsetNumberMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Bass parameter
const char dpDrumSoundForBassName[]    PROGMEM  = "Звук для баса";
#define dpDrumSoundForBassMinValue _F1      // this is a MIDI note
#define dpDrumSoundForBassMaxValue _E7      // this is a MIDI note

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Bass Velocity parameter
const char dpDrumSoundForBassVelocityName[]    PROGMEM  = "     его сила";
#define dpDrumSoundForBassVelocityMaxValue 127

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Chord parameter
const char dpDrumSoundForChordName[]    PROGMEM  = "Звук д. аккорда";
#define dpDrumSoundForChordMinValue _F1      // this is a MIDI note
#define dpDrumSoundForChordMaxValue _E7      // this is a MIDI note

/////////////////////////////////////////////////////////////////
// specific data for the Drum Sound for Chord Velocity parameter
const char dpDrumSoundForChordVelocityName[]    PROGMEM  = "       его сила";
#define dpDrumSoundForChordVelocityMaxValue 127

/////////////////////////////////////////////////////////////////
// common data

#define dpEditorFirstRow     0       // we start from this row
#define dpEditorRows         6       // parameters in total
#define dpEditorRowIncrement 1       // 0 empty row between parameters
#define dpEditorNameColumn   0       // where we start printing names
#define dpEditorValueColumn  17      // where we start printing values

#define dpNumberOfParameters   6
const char * const  dpParameterNames[dpNumberOfParameters] PROGMEM = {
    dpDrumsVolumeName, dpDrumsetNumberName, dpDrumSoundForBassName, dpDrumSoundForBassVelocityName, dpDrumSoundForChordName, dpDrumSoundForChordVelocityName
};

const char drums_param_screen_footer[] PROGMEM = "Настройки ударных";

///////////////////////////////////////////////////////////////////////////////////////////////////
// section for the Synth-specific instrument editor

// list of parameter IDs
enum {ieGroupId = 0, ieInstrumentId};
#define ieNumberOfParameters 2

const char ieInstrumentGroupTitle[] PROGMEM = "Группа инструментов:";
#define ieInstrumentGroupTitlePos   0   // 1st row
#define ieInstrumentGroupNamePos    1   // just below the title

const char ieInstrumentTitle[] PROGMEM = "Инструмент:";
#define ieInstrumentTitlePos   3        // 4th row
#define ieInstrumentNamePos    4        // just below the title

//                                          123456789-123456789-1
const char ieInstrumentIDsLine[] PROGMEM = "Банк: XXX, Прог.: XXX";
#define ieInstrumentIDsLinePos  6
#define ieBankIdColumn      6
#define ieProgramIdColumn   18

const char instrument_edit_screen_footer[] PROGMEM = "Выбор инструмента";

///////////////////////////////////////////////////////////////////////////////////////////////////
// *** section for ATEMP Synthesizer

// Mapping of banks. Sounds are distributed among banks pretty chaotically, without any visible pattern.
// We want to be able to skip banks which do not contain sounds. So, this is a mapping where we have a list of non-empty banks for each program (PC 0..127)
// If only "General MIDI" sound is available for a program, then a respective pointer is NULL. Otherwise, it points to a vector of bank IDs.
const uint8_t atBanks4[] PROGMEM  = {7, 0, 8, 9, 16, 24, 25, 126};
const uint8_t atBanks5[] PROGMEM  = {6, 0, 8, 9, 16, 24, 126};
const uint8_t atBanks6[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks12[] PROGMEM  = {2, 0, 16};
const uint8_t atBanks14[] PROGMEM  = {3, 0, 8, 9};
const uint8_t atBanks15[] PROGMEM  = {2, 0, 1};
const uint8_t atBanks16[] PROGMEM  = {8, 0, 1, 8, 9, 16, 33, 40, 126};
const uint8_t atBanks17[] PROGMEM  = {5, 0, 1, 8, 9, 126};
const uint8_t atBanks18[] PROGMEM  = {4, 0, 8, 16, 24};
const uint8_t atBanks19[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks21[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks24[] PROGMEM  = {4, 0, 8, 16, 24};
const uint8_t atBanks25[] PROGMEM  = {7, 0, 7, 8, 9, 16, 32, 126};
const uint8_t atBanks26[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks27[] PROGMEM  = {4, 0, 8, 9, 126};
const uint8_t atBanks28[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks29[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks30[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks31[] PROGMEM  = {3, 0, 8, 16};
const uint8_t atBanks36[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks38[] PROGMEM  = {6, 0, 8, 9, 10, 16, 126};
const uint8_t atBanks39[] PROGMEM  = {10, 0, 1, 2, 3, 8, 16, 17, 18, 19, 126};
const uint8_t atBanks48[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks49[] PROGMEM  = {2, 0, 10};
const uint8_t atBanks50[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks55[] PROGMEM  = {3, 0, 8, 9};
const uint8_t atBanks56[] PROGMEM  = {5, 0, 1, 8, 24, 126};
const uint8_t atBanks59[] PROGMEM  = {3, 0, 1, 126};
const uint8_t atBanks61[] PROGMEM  = {3, 0, 8, 16};
const uint8_t atBanks62[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks63[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks64[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks65[] PROGMEM  = {3, 0, 8, 126};
const uint8_t atBanks66[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks67[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks69[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks70[] PROGMEM  = {2, 0, 126};
const uint8_t atBanks71[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks80[] PROGMEM  = {3, 0, 1, 8};
const uint8_t atBanks81[] PROGMEM  = {2, 0, 1};
const uint8_t atBanks89[] PROGMEM  = {2, 0, 3};
const uint8_t atBanks102[] PROGMEM  = {2, 0, 2};
const uint8_t atBanks107[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks115[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks116[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks117[] PROGMEM  = {2, 0, 8};
const uint8_t atBanks118[] PROGMEM  = {3, 0, 8, 9};
const uint8_t atBanks120[] PROGMEM  = {5, 0, 1, 2, 5, 6};
const uint8_t atBanks121[] PROGMEM  = {2, 0, 1};
const uint8_t atBanks122[] PROGMEM  = {6, 0, 1, 2, 3, 4, 5};
const uint8_t atBanks123[] PROGMEM  = {4, 0, 1, 2, 3};
const uint8_t atBanks124[] PROGMEM  = {6, 0, 1, 2, 3, 4, 5};
const uint8_t atBanks125[] PROGMEM  = {10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t atBanks126[] PROGMEM  = {6, 0, 1, 2, 3, 4, 5};
const uint8_t atBanks127[] PROGMEM  = {4, 0, 1, 2, 3};
const uint8_t * const  atBanks[128] PROGMEM = {
//  0     1     2     3     4         5         6         7     8     9     10    11    12         13    14         15
    NULL, NULL, NULL, NULL, atBanks4, atBanks5, atBanks6, NULL, NULL, NULL, NULL, NULL, atBanks12, NULL, atBanks14, atBanks15,
//  16         17         18         19         20    21         22    23    24         25         26         27         28         29         30         31
    atBanks16, atBanks17, atBanks18, atBanks19, NULL, atBanks21, NULL, NULL, atBanks24, atBanks25, atBanks26, atBanks27, atBanks28, atBanks29, atBanks30, atBanks31,
//  32    33    34    35    36         37    38         39         40    41    42    43    44    45    46    47
    NULL, NULL, NULL, NULL, atBanks36, NULL, atBanks38, atBanks39, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//  48         49         50         51    52    53    54    55         56         57    58    59         60    61         62         63
    atBanks48, atBanks49, atBanks50, NULL, NULL, NULL, NULL, atBanks55, atBanks56, NULL, NULL, atBanks59, NULL, atBanks61, atBanks62, atBanks63, 
//  64         65         66         67         68    69         70         71         72    73    74    75    76    77    78    79
    atBanks64, atBanks65, atBanks66, atBanks67, NULL, atBanks69, atBanks70, atBanks71, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
//  80         81         82    83    84    85    86    87    88    89         90    91    92    93    94    95    
    atBanks80, atBanks81, NULL, NULL, NULL, NULL, NULL, NULL, NULL, atBanks89, NULL, NULL, NULL, NULL, NULL, NULL,
//  96    97    98    99    100   101   102         103   104   105   106   107         108   109   110   111
    NULL, NULL, NULL, NULL, NULL, NULL, atBanks102, NULL, NULL, NULL, NULL, atBanks107, NULL, NULL, NULL, NULL, 
//  112   113   114   115         116         117         118         119   120         121         122         123         124         125         126         127
    NULL, NULL, NULL, atBanks115, atBanks116, atBanks117, atBanks118, NULL, atBanks120, atBanks121, atBanks122, atBanks123, atBanks124, atBanks125, atBanks126, atBanks127
};

// A list of drumsets supproted by ATEMP
const uint8_t atDrumSets[] PROGMEM  = {10, 0, 8, 16, 24, 25, 32, 40, 48, 56, 127};

///////////////////////////////////////////////////////////////////////////////////////////////////
// *** section for Pro.DX Synthesizer

const uint8_t dxBanks0[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks1[] PROGMEM = {2, 0, 8};
const uint8_t dxBanks3[] PROGMEM = {2, 0, 8};
const uint8_t dxBanks4[] PROGMEM = {7, 0, 1, 2, 3, 4, 8, 9};
const uint8_t dxBanks5[] PROGMEM = {5, 0, 1, 2, 8, 9};
const uint8_t dxBanks6[] PROGMEM = {3, 0, 1, 8};
const uint8_t dxBanks7[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks12[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks14[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks15[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks16[] PROGMEM = {8, 0, 1, 2, 8, 9, 16, 33, 40};
const uint8_t dxBanks17[] PROGMEM = {6, 0, 1, 2, 8, 9, 16};
const uint8_t dxBanks18[] PROGMEM = {4, 0, 8, 16, 24};
const uint8_t dxBanks19[] PROGMEM = {7, 0, 1, 19, 3, 4, 5, 8};
const uint8_t dxBanks21[] PROGMEM = {5, 0, 1, 2, 3, 8};
const uint8_t dxBanks22[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks24[] PROGMEM = {8, 0, 1, 2, 8, 16, 24, 32, 40};
const uint8_t dxBanks25[] PROGMEM = {7, 0, 1, 8, 16, 17, 18, 32};
const uint8_t dxBanks26[] PROGMEM = {4, 0, 1, 2, 8};
const uint8_t dxBanks27[] PROGMEM = {4, 0, 1, 2, 3};
const uint8_t dxBanks29[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks30[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks31[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks32[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks33[] PROGMEM = {5, 0, 1, 2, 3, 4};
const uint8_t dxBanks38[] PROGMEM = {9, 0, 1, 2, 3, 4, 5, 6, 7, 8};
const uint8_t dxBanks39[] PROGMEM = {11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const uint8_t dxBanks40[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks45[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks47[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks48[] PROGMEM = {4, 0, 1, 2, 3};
const uint8_t dxBanks49[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks50[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks51[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks52[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks53[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks55[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks56[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks58[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks59[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks61[] PROGMEM = {6, 0, 1, 2, 3, 8, 16};
const uint8_t dxBanks62[] PROGMEM = {5, 0, 1, 2, 8, 16};
const uint8_t dxBanks63[] PROGMEM = {5, 0, 1, 2, 3, 8};
const uint8_t dxBanks65[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks66[] PROGMEM = {4, 0, 1, 2, 3};
const uint8_t dxBanks71[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks73[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks74[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks80[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks81[] PROGMEM = {3, 0, 1, 8};
const uint8_t dxBanks88[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks89[] PROGMEM = {5, 0, 1, 2, 3, 8};
const uint8_t dxBanks90[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks91[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks102[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks107[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks114[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks115[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks116[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks117[] PROGMEM = {2, 0, 1};
const uint8_t dxBanks118[] PROGMEM = {3, 0, 1, 2};
const uint8_t dxBanks120[] PROGMEM = {5, 0, 1, 2, 3, 4};
const uint8_t dxBanks121[] PROGMEM = {3, 0, 1, 5};
const uint8_t dxBanks122[] PROGMEM = {5, 0, 1, 2, 3, 4};
const uint8_t dxBanks123[] PROGMEM = {4, 0, 1, 2, 3};
const uint8_t dxBanks124[] PROGMEM = {6, 0, 1, 2, 3, 4, 5};
const uint8_t dxBanks125[] PROGMEM = {10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const uint8_t dxBanks126[] PROGMEM = {6, 0, 1, 2, 3, 4, 5};
const uint8_t dxBanks127[] PROGMEM = {4, 0, 1, 2, 3};
const uint8_t * const dxBanks[128] PROGMEM = {
    dxBanks0, dxBanks1, NULL, dxBanks3, dxBanks4, dxBanks5, dxBanks6, dxBanks7, NULL, NULL, NULL, NULL, dxBanks12, NULL, dxBanks14,
    dxBanks15, dxBanks16, dxBanks17, dxBanks18, dxBanks19, NULL, dxBanks21, dxBanks22, NULL, dxBanks24, dxBanks25, dxBanks26, dxBanks27,
    NULL, dxBanks29, dxBanks30, dxBanks31, dxBanks32, dxBanks33, NULL, NULL, NULL, NULL, dxBanks38, dxBanks39, dxBanks40, NULL, NULL, NULL,
    NULL, dxBanks45, NULL, dxBanks47, dxBanks48, dxBanks49, dxBanks50, dxBanks51, dxBanks52, dxBanks53, NULL, dxBanks55, dxBanks56, NULL,
    dxBanks58, dxBanks59, NULL, dxBanks61, dxBanks62, dxBanks63, NULL, dxBanks65, dxBanks66, NULL, NULL, NULL, NULL, dxBanks71, NULL,
    dxBanks73, dxBanks74, NULL, NULL, NULL, NULL, NULL, dxBanks80, dxBanks81, NULL, NULL, NULL, NULL, NULL, NULL, dxBanks88, dxBanks89,
    dxBanks90, dxBanks91, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, dxBanks102, NULL, NULL, NULL, NULL, dxBanks107,
    NULL, NULL, NULL, NULL, NULL, NULL, dxBanks114, dxBanks115, dxBanks116, dxBanks117, dxBanks118, NULL, dxBanks120, dxBanks121, dxBanks122,
    dxBanks123, dxBanks124, dxBanks125, dxBanks126, dxBanks127
};

// A list of drumsets supproted by Pro.DX
const uint8_t dxDrumSets[] PROGMEM  = {15, 0, 1, 2, 3, 4, 5, 6, 8, 11, 16, 17, 24, 25, 40, 48};

////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// private methods //////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// стырено отсюда: https://forum.amperka.ru/threads/Подсчёт-числа-символов-в-строке.19457/#post-237619
//
// Функция подсчёта количества символов в строке utf8,
// состоящей из букв английского и русского алфавитов, цифр, общепринятых символов...

//int mb_strlen_P(PGM_P _source)
int mb_strlen_P(const __FlashStringHelper * _source)
{
    PGM_P p = reinterpret_cast<PGM_P>(_source);

    int target = 0;
    unsigned char n;

    n = pgm_read_byte(p++);
    while (n != 0) {
        if (n >= 0xBF){
            switch (n) {
            case 0xD0:
            case 0xD1:
                n = pgm_read_byte(p++);
                break;
            }
        }
        target = target + 1;
        n = pgm_read_byte(p++);
    }
    return target;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns the specified sub-string from the large string stored in PROGMEM
// The large string contains many shorter strings ending with \0
PGM_P   getSubString_P(PGM_P storage, int16_t index){
    while (index-- > 0){
        // skip the current string
        while(pgm_read_byte(storage++) != 0);
    }
    return storage;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns a group_id as defined in group_map for program_id
uint8_t getInstrumentGroupId(const uint8_t * group_map, uint8_t program_id){
    uint8_t group_id = 0;

    while(pgm_read_byte(group_map + group_id) < program_id){
        group_id++;
    }

    return group_id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function returns the global instrument index (as defined in )
uint16_t getInstrumentIndex(const uint8_t * const * bank_map, uint8_t bank_id, uint8_t program_id){
    uint16_t instrument_index = 0;

    // count all instruments BEFORE program_id
    for(int i=0; i<program_id; i++){
        // get a bank vector for program 'i'
        const uint8_t *ptr = pgm_read_ptr(bank_map + i);
        if (ptr == NULL){
            // There's only one variation for this program
            instrument_index += 1;  // counting the only variation of program 'i'
        }
        else{
            // There're variations for program 'i'. The number of variations is stored in the 1st element of the vector.
            instrument_index += pgm_read_byte(ptr);
        }
    }

    // get a bank vector for program_id
    const uint8_t *ptr = pgm_read_ptr(bank_map + program_id);
    if (ptr == NULL){
        // there's only one variation for program_id
        // there's nothing to add; instrument_index is already pointing to {bank_id}:{program_id}
        if(bank_id != 0){
            // if ptr == NULL then the bank_id is expected to be 0
            SWER(swerGUI10);
        }
    }
    else {
        // count prior variations of program_id
        uint8_t N = pgm_read_byte(ptr);
        bool was_found = false;
        for(int i=1; i<=N; i++){
            if (pgm_read_byte(ptr + i) != bank_id){
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
            SWER(swerGUI11);
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
    const uint8_t *ptr = pgm_read_ptr(bank_map + cur_program);
    // check if we can move to the next bank_id
    if (ptr != NULL){
        uint8_t N = pgm_read_byte(ptr);             // the size of the vector
        for(int i=1; i<=N; i++){
            if (pgm_read_byte(ptr + i) == cur_bank){
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
        *bank_id = pgm_read_byte(ptr + bank_index);
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
    ptr = pgm_read_ptr(bank_map + cur_program);
    if (ptr == NULL){
        // easy case: the next bank id is 0
        *bank_id = 0;
    }
    else{
        uint8_t N = pgm_read_byte(ptr);             // the size of the vector
        // read either the 1st of the last element (depending on the direction)
        *bank_id = pgm_read_byte(ptr + ((forward)? 1 : N));
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
        _mc->_var.freeBassOn,                  // free bass keyboard - same as the Free Bass mode
    };
    for(int i=0; i<headerSections; i++){
        setTextCursor(headerStartPosition + i*headerIncrement, 0);
        _oled->invertText(headerHighlight[i]);
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(headerSection + i)) );
    }
    // turn inversion off
    _oled->invertText(false);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the main screen
void TesMidiUI::drawMainScreen(void){
    _oled->clear();

    // print the header
    drawKbdHeader();
    // print 3 top parameters
    for(int idx=0; idx<3; idx++){
        setTextCursor(0, idx+1);
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(parLabel + idx)) );
        // print parameters here
        for(int kbd=0; kbd<4; kbd++){
            setTextCursor(6 + kbd*4, idx+1);
            printFormatted(numFormat3R, _mc->_settings.preset.kbdParameter[kbd][idx]);
        }
    }
    // print Indicators
    {
        // Indicator's logic is different, so, print them onr-by-one (not in a cycle)
        // *** Bass Octaves
        setTextCursor(indBassOctavesPos, indRow);
        _oled->invertText( _mc->_settings.global.bassOctavesOn );
        _oled->print(FPSTR(indBassOctavesLabel));
        _oled->invertText(false);
        // *** Pressure Sensor
        setTextCursor(indPressureSensorOnPos, indRow);
        _oled->invertText( _mc->_settings.global.pressureSensorOn );
        _oled->print(FPSTR(indPressureSensorLabel));
        _oled->invertText(false);
        // *** Synthesizer Type
        setTextCursor(indSynthTypePos, indRow);
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(indSynthTypeLabel + _mc->_settings.global.synthType)) );
        // *** Running Status
        setTextCursor(indRunningStatusPos, indRow);
        _oled->invertText( _mc->_settings.global.runningStatus );
        _oled->print(FPSTR(indRunningStatusLabel));
        _oled->invertText(false);
    }
    // print the bottom
    _oled->fastLineH(7*symbolHeight-2, 3, 124, OLED_FILL);
    // If a melody is not played - print a banner
    PGM_P   melodyName  = _mc->_auto_drums.getCurrentMelodyName();
    switch(_mc->_var.drumsMode){
    case dmOff:
    case dmManual:
        _oled->setCursor( (128 - mb_strlen_P(FPSTR(main_screen_footer))*symbolWidth)/2, 7);
        _oled->print(FPSTR(main_screen_footer));
        break;
    case dmStandby:
    case dmPlaying:
        _oled->setCursor( (128 - (mb_strlen_P(FPSTR(melodyName)) + 6)*symbolWidth)/2, 7);
        _oled->print(FPSTR(melodyName));
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
    _oled->setCursor( X*symbolWidth , Y);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  prints a number according to the specified format string located in PROGMEM
void TesMidiUI::printFormatted(PGM_P format, uint8_t value){
    char buf[maxFormattedStringLength];     // for a properly aligned number
    char fmt[maxFormatDescriptorLength];    // for a format descriptor string
    sprintf(buf, strcpy_P(fmt, format), value);
    _oled->print(buf);
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
    default:
        SWER(swerGUI08);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the MIDI parameters editor screen
void TesMidiUI::drawMidiParamEditorScreen(void){
    // clear work area
    _oled->clear();
    // print the header
    drawKbdHeader();
    // print rows in the work area
    for (int editorRow=0; editorRow<editorRows; editorRow++ ){
        setTextCursor(0, editorRow+editorFirstRow);
        uint8_t currentParameterIndex = editorRow + _editor_status.topIndex;
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(parLabel + currentParameterIndex)) );
        // print values
        for(int kbd=0; kbd<4; kbd++){
            // decide text inversion - select the value being edited
            _oled->invertText(
                _editor_status.editMode
                && (_editor_status.selectorX == kbd) 
                && (_editor_status.selectorY == editorRow)
            );
            setTextCursor(6 + kbd*4, editorRow+editorFirstRow);
            printFormatted(numFormat3R, _mc->_settings.preset.kbdParameter[kbd][currentParameterIndex]);
        }
        //reset text inversion (just in case...)
        _oled->invertText(false);
    }
    // draw the selector (frame around the current value)
    // calculate graphical X and Y
    uint8_t selX1 = 6*symbolWidth +_editor_status.selectorX*4*symbolWidth -3;
    uint8_t selX2 = 6*symbolWidth +_editor_status.selectorX*4*symbolWidth +symbolWidth*3 +1;
    uint8_t selY1 = symbolHeight*editorFirstRow -1 +_editor_status.selectorY*symbolHeight;
    uint8_t selY2 = symbolHeight*editorFirstRow +7 +_editor_status.selectorY*symbolHeight;
    _oled->roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    _oled->fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    uint8_t selectedParameterIndex = _editor_status.topIndex + _editor_status.selectorY;
    _oled->setCursor( (128 - mb_strlen_P(FPSTR((PGM_P)pgm_read_ptr(parHint + selectedParameterIndex)))*symbolWidth)/2, 7);
    _oled->print(FPSTR((PGM_P)pgm_read_ptr(parHint + selectedParameterIndex)));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the System parameters editor screen
void TesMidiUI::drawSystemParamEditorScreen(void){
    // clear work area
    _oled->clear();
    // print rows in the work area
    for (int editorRow=0; editorRow<paramEditorRows; editorRow++){
        setTextCursor(paramEditorNameColumn, editorRow + paramEditorFirstRow);
        uint8_t currentSystemParameterIndex = editorRow + _editor_status.sysParTopIndex;
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(parParameterNames + currentSystemParameterIndex)) );
        // print the value
        setTextCursor(paramEditorValueColumn, editorRow + paramEditorFirstRow);
        // decide text inversion - select the value being edited
        _oled->invertText(editorRow == _editor_status.sysParSelector);
        switch(currentSystemParameterIndex){
            case parRunningStatusId:
                _oled->print( FPSTR((PGM_P)pgm_read_ptr(parBooleanLabel + _mc->_settings.global.runningStatus)) );
                break;
            case parBassOctavesId:
                _oled->print( FPSTR((PGM_P)pgm_read_ptr(parBooleanLabel + _mc->_settings.global.bassOctavesOn)) );
                break;
            case parSynthTypeId:
                _oled->print( FPSTR((PGM_P)pgm_read_ptr(parSynthTypeLabel + _mc->_settings.global.synthType)) );
                break;
            case parMasterVolumeId:
                printFormatted(numFormat3R, _mc->_settings.preset.masterVolume);
                break;
            case parDrumsChannelId:
                printFormatted(numFormat3R, _mc->_settings.global.drumsMidiChannel);
                break;
            case parTest1:
                // TODO the same value as RS
                _oled->print( FPSTR((PGM_P)pgm_read_ptr(parBooleanLabel + _mc->_settings.global.runningStatus)) );
                break;
            case parTest2:
                // TODO the same as "bass octaves"
                _oled->print( FPSTR((PGM_P)pgm_read_ptr(parBooleanLabel + _mc->_settings.global.bassOctavesOn)) );
                break;
            default:
                SWER(swerGUI02);
        }
        //reset text inversion (just in case...)
        _oled->invertText(false);
    }

    // draw the selector (frame around the current value)
    // calculate graphical X and Y
    uint8_t selX1 = paramEditorValueColumn*symbolWidth -3;
    uint8_t selX2 = paramEditorValueColumn*symbolWidth +symbolWidth*3 +1;
    uint8_t selY1 = symbolHeight*paramEditorFirstRow -1 +_editor_status.sysParSelector*symbolHeight;
    uint8_t selY2 = symbolHeight*paramEditorFirstRow +7 +_editor_status.sysParSelector*symbolHeight;
    _oled->roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    _oled->fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    _oled->setCursor( (128 - mb_strlen_P(FPSTR(sys_param_screen_footer))*symbolWidth)/2, 7);
    _oled->print(FPSTR(sys_param_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Drums parameters editor screen
void TesMidiUI::drawDrumsParamEditorScreen(void){
    // clear work area
    _oled->clear();
    // print rows in the work area
    for (int param_number=0; param_number < dpNumberOfParameters; param_number++){
        setTextCursor(dpEditorNameColumn, param_number * dpEditorRowIncrement + dpEditorFirstRow);
        _oled->print( FPSTR((PGM_P)pgm_read_ptr(dpParameterNames + param_number)) );
        // print the value
        setTextCursor(dpEditorValueColumn, param_number * dpEditorRowIncrement + dpEditorFirstRow);
        // decide text inversion - select the value being edited
        _oled->invertText(param_number == _editor_status.drumsParSelector);
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
        _oled->invertText(false);
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
    _oled->roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // print the bottom
    _oled->fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    _oled->setCursor( (128 - mb_strlen_P(FPSTR(drums_param_screen_footer))*symbolWidth)/2, 7);
    _oled->print(FPSTR(drums_param_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  Draws the Instrument selector/editor screen
void TesMidiUI::drawInstrumentEditorScreen(void){
    // This function can be called in the ATEMP mode only
    if (_mc->_settings.global.synthType != stProDX){
        SWER(swerGUI09);
    }
    // Info: the ID of the current Keyboard is in _editor_status.selectorX (because this screen was called from Parameters Editor)
    // determine the instrument group and instrument index
    uint8_t     bank_id = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxBank];
    uint8_t     program_id = _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram];
    uint8_t     instrument_group = getInstrumentGroupId(dxGroupMap, program_id);
    uint16_t    instrument_index = getInstrumentIndex(dxBanks, bank_id, program_id);

    // clear work area
    _oled->clear();

    // Print the title of Instrument group
    _oled->setCursor((128 - mb_strlen_P(FPSTR(ieInstrumentGroupTitle))*symbolWidth)/2, ieInstrumentGroupTitlePos);
    _oled->print(FPSTR(ieInstrumentGroupTitle));
    // Print the Instrument group name
    _oled->setCursor((128 - mb_strlen_P(FPSTR((PGM_P)pgm_read_ptr(dxGroupName + instrument_group)))*symbolWidth)/2, ieInstrumentGroupNamePos);
    _oled->print(FPSTR((PGM_P)pgm_read_ptr(dxGroupName + instrument_group)));

    // Print the title of Instrument
    _oled->setCursor((128 - mb_strlen_P(FPSTR(ieInstrumentTitle))*symbolWidth)/2, ieInstrumentTitlePos);
    _oled->print(FPSTR(ieInstrumentTitle));
    // Print the Instrument name
    PGM_P   str = getSubString_P(dxInstrumentNames, instrument_index);
    _oled->setCursor((128 - mb_strlen_P(FPSTR(str))*symbolWidth)/2, ieInstrumentNamePos);
    _oled->print(FPSTR(str));

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
    _oled->roundRect(selX1, selY1, selX2, selY2, OLED_STROKE);

    // Print instrument IDs
    _oled->setCursor(0, ieInstrumentIDsLinePos);    // to the very beginning of the line
    _oled->print(FPSTR(ieInstrumentIDsLine));
    _oled->setCursor(ieBankIdColumn*symbolWidth, ieInstrumentIDsLinePos);
    // print the Bank ID
    printFormatted(numFormat3R, bank_id);
    _oled->setCursor(ieProgramIdColumn*symbolWidth, ieInstrumentIDsLinePos);
    // print the Program ID
    printFormatted(numFormat3R, program_id);

    // print the bottom
    _oled->fastLineH(7*symbolHeight-1, 3, 124, OLED_FILL);
    _oled->setCursor( (128 - mb_strlen_P(FPSTR(instrument_edit_screen_footer))*symbolWidth)/2, 7);
    _oled->print(FPSTR(instrument_edit_screen_footer));

    _flags.display_update_is_required = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//  control button event handler to Midi Param Editor mode
void TesMidiUI::processCtlButtonEventMidiParamEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
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
                            if (_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] == 0){
                                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] = pgm_read_byte(parMaxValue +parameterIndex);
                            }
                            else {
                                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]--;
                            }
                        }
                        else {      // ctlButtonRight
                            newValueIsOk = true;
                            if (_mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] == pgm_read_byte(parMaxValue +parameterIndex)){
                                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex] = 0;
                            }
                            else {
                                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][parameterIndex]++;
                            }
                        }
                        // check the special case: each keyboard should play into a unique MIDI channel
                        // keep changing the Channel ID until it's unique
                        if (parameterIndex == idxChannel){
                            // yes, this is the Channel parameter; let's check the condition
                            for(int i=0; i<4; i++){
                                if ( (i != _editor_status.selectorX) && (_mc->_settings.preset.kbdParameter[i][idxChannel] == _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxChannel]) ){
                                    newValueIsOk = false;
                                    break;
                                }
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
                if ((_mc->_settings.global.synthType == stProDX)
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
        switch (event->buttonId){
        case ctlButtonUp:
            // move up
            /*
             *if( _editor_status.sysParSelector-- == 0){
             *    _editor_status.sysParSelector = paramEditorRows - 1;
             *}
             */
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
            /*
             *if( ++_editor_status.sysParSelector == paramEditorRows){
             *    _editor_status.sysParSelector = 0;
             *}
             */
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
            case parRunningStatusId:
            case parTest1:  // TODO remove it after debugging
                // toggle the value
                _mc->_settings.global.runningStatus = ! _mc->_settings.global.runningStatus;
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
                break;
            case parBassOctavesId:
            case parTest2: // TODO remove it after debugging
                // toggle the value
                _mc->_settings.global.bassOctavesOn = ! _mc->_settings.global.bassOctavesOn;
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
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
                // set the new Volume if the pressure sensor is off
                if ( ! _mc->_settings.global.pressureSensorOn ){
                    _mc->setVolume(_mc->_settings.preset.masterVolume);
                }
                // set new Volume for Drums in any case (drums do not depend on Pressure Sensor)
                _mc->setDrumsVolume();
                // start indicating a "not saved preset", if required
                indicatePresetChange();
                break;
            case parDrumsChannelId:
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
                // notify the controller about global settings change
                _mc->globalSettingsChangeNotification();
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
//  control button event handler to Drums Param Editor mode
void TesMidiUI::processCtlButtonEventDrumsParamEditor(tesEvent *event){
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
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
    if (_mc->_settings.global.synthType != stProDX){
        // this function can be called only for the ATEMP synthesizer
        SWER(swerGUI12);
    }
    // as this is a Control Button event, event->buttonId already contains Ctl Button ID
    bool    needToClearTheEvent = true; // by default we clear the event

    switch (event->buttonEvent){
    case tesBeReleased:
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
                uint8_t group_id = getInstrumentGroupId(dxGroupMap, program_id);
                if (event->buttonId == ctlButtonLeft){
                    // go to the previous group
                    if (group_id-- == 0){
                        group_id = dxNumberOfGroups - 1;
                    }
                }
                else {      // ctlButtonRight
                    // go to the next group
                    if (++group_id == dxNumberOfGroups){
                        group_id = 0;
                    }
                }
                // get the 1st program_id for the new group_id
                _mc->_settings.preset.kbdParameter[_editor_status.selectorX][idxProgram] =
                    (group_id == 0) ? 0 : (pgm_read_byte(dxGroupMap + group_id -1) + 1);
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
                        dxBanks, 
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
//  Activates the specified preset
void TesMidiUI::activatePreset(uint8_t preset_id){
    if ((preset_id == _mc->_settings.global.activePreset) && ( ! _mc->_var.currentPresetNotSaved)){
        // there's nothonig to do
        return;
    }
   
    // clear indication of the old preset
    _led_set->resetLED( _mc->_settings.global.activePreset + ledPreset1);    // the assumption is that Led IDs are sorted (like 0, 1, 2, etc.)
    // activate the new preset
    _mc->activatePreset(preset_id);
    // indicate the active preset
    _led_set->setLED( _mc->_settings.global.activePreset + ledPreset1, 1);   // the assumption is that Led IDs are sorted (like 0, 1, 2, etc.)
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
        ptr = pgm_read_ptr(atBanks + curProgram);
        break;
    case stProDX:
        ptr = pgm_read_ptr(dxBanks + curProgram);
        break;
    }
    if (ptr == NULL){
        // no other banks available; return the current one
        return curBankId;
    }
    // *** let's find the next bank id

    // get the total number of bank IDs
    uint8_t N = pgm_read_byte(ptr);
    // find the index of the current bank
    uint8_t curIndex = 1;
    while ((curIndex <= N) && (pgm_read_byte(ptr + curIndex) != curBankId)){
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
    return pgm_read_byte(ptr + curIndex);
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
    uint8_t N = pgm_read_byte(map);
    // find the index of the current bank
    uint8_t curIndex = 1;
    while ((curIndex <= N) && (pgm_read_byte(map + curIndex) != _mc->_settings.preset.drumsetNumber)){
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
    return pgm_read_byte(map + curIndex);
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
        ptr = pgm_read_ptr(atBanks + curProgram);
        break;
    case stProDX:
        ptr = pgm_read_ptr(dxBanks + curProgram);
        break;
    }
    if (ptr == NULL){
        // There's just one available bank id: 0
        // new_bank_id is already set to 0 (above)
    }
    else {
        // *** let's find the best new_bank_id
        // get the total number of bank IDs
        uint8_t N = pgm_read_byte(ptr);
        // find a bank id closest to curBankId
        uint8_t diff = 200;
        for(int i = 1; i<=N; i++){
            int b = pgm_read_byte(ptr + i);
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
        _led_set->startBlink(_mc->_settings.global.activePreset + ledPreset1, teslpSlowBlink);
        // set the "preset not saved" flag
        _mc->_var.currentPresetNotSaved = 1;
    }
}

