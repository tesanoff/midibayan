#include <Arduino.h>
#include <GyverOLED.h>
#include "TesLedSet.h"
#include "TesEventQueue.h"
#include "TesKeyboard.h"
#include "TesMidiController.h"
#include "swer.h"
#include <EEPROM.h>

//////////////////////////////////////////
// Mapping between buttons and notes.
/*
107 music buttons are distributed as:
- right keyboard:   55 keys from Bb (big octave) to E (4th octave)
                    It requires a cascade of 7 shift registers
                    (1 bit is not used).
- left keyboard:    52 keys from F (Contractave) to G# (2nd octave)
                    It Required s cascade of 7 shift registers
                    (4 bits are not used)
*/

// configure music buttons
// This class (TesMidiController) receives "music button IDs" from 0 to 106; this class coesn't care about physical implementation, it deals with just music button IDs.
#include "midi_note_names.h"    // the list is too large to put it right here
const PROGMEM uint8_t noteMap[TES_NUMBER_OF_ALL_MUSIC_BUTTONS] = {
// Left keyboard
// #0-----------Contr.----------------#6    #7--------------------------------Big------------------------#18
    _F1, _Gb1, _G1, _Ab1, _A1, _Bb1, _B1,    _C2, _Db2, _D2, _Eb2, _E2, _F2, _Gb2, _G2, _Ab2, _A2, _Bb2, _B2,
// #19----------------------------Small-------------------------#30     #31---------------------------1st----------------------------#42
    _C3, _Db3, _D3, _Eb3, _E3, _F3, _Gb3, _G3, _Ab3, _A3, _Bb3, _B3,     _C4, _Db4, _D4, _Eb4, _E4, _F4, _Gb4, _G4, _Ab4, _A4, _Bb4, _B4,
// #43----------------------2nd------------------#51
    _C5, _Db5, _D5, _Eb5, _E5, _F5, _Gb5, _G5, _Ab5,
// Right keyboard
// #52-Big-#53   #54-------------------------Small----------------------------#65   #66---------------------------1st----------------------------#77
    _Bb2, _B2,   _C3, _Db3, _D3, _Eb3, _E3, _F3, _Gb3, _G3, _Ab3, _A3, _Bb3, _B3,   _C4, _Db4, _D4, _Eb4, _E4, _F4, _Gb4, _G4, _Ab4, _A4, _Bb4, _B4,
// #78--------------------2nd-----------------------------------#89     #90------------------------3rd------------------------------#101
    _C5, _Db5, _D5, _Eb5, _E5, _F5, _Gb5, _G5, _Ab5, _A5, _Bb5, _B5,     _C6, _Db6, _D6, _Eb6, _E6, _F6, _Gb6, _G6, _Ab6, _A6, _Bb6, _B6,
// #102--------4th-------#106
    _C7, _Db7, _D7, _Eb7, _E7
};


// default values for MIDI parameters
const uint8_t   kbdParameterDefaultValue[NUMBER_OF_KBD_PARAMETERS] PROGMEM = {
    0,      // MIDI Channel
    0,      // Bank             - default
    0,      // Program          - Grand Piano
    127,    // Volume           - highest (it will be corrected by the "master volume" setting)
    0,      // Vibrato
    64,      // Resonance
    64,      // Attak
    64,      // Release
    64,      // Timbre
    64,      // Decay
    40,      // Reverbration
    0,      // Chorus
    4,      // Reverb Type
    2       // Chorus Type
};

// sequence of sending Kbd Parameters
// It looks like the order matters (PC cannot be sent right after "CC 0", i.e. bank change)
const uint8_t   kbdParameterOrder[NUMBER_OF_KBD_PARAMETERS] PROGMEM = {
    idxBank, idxChannel, idxVolume, idxVibrato, idxResonance, idxAttak, idxRelease, idxProgram, idxTimbre, idxDecay, idxReverbration, idxChorus, idxReverbType, idxChorusType
};


///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// TesMidiControllerSettings ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

TesMIDIControllerSettings::TesMIDIControllerSettings (void){
    // set default values for global parameters
    global.pressureSensorOn = false;
    global.runningStatus = false;   // ATEMP synth. doesn't work well with RS
    global.synthType = stAtemp;
    global.activePreset = 0;
    global.bassOctavesOn = true;
    global.drumsMidiChannel = 9;    // I let this hardcode be here, because it's very unlikely that anybody will want to change this default value

    // set default values for preset values
    preset.masterVolume = TES_DEFAULT_MASTER_VOLUME;
    preset.drumsVolume                  = 127;  // it's relative to the master volume
    preset.drumsetNumber                = 0;    // the default drumset
    preset.drumSoundForBass             = _C2;  // std kick drum
    preset.drumSoundForBassVelocity     = 127;  // max velocity
    preset.drumSoundForChord            = _Ab2; // pedal Hi-Hat
    preset.drumSoundForChordVelocity    = 127;  // max velocity
    // set default values for everything except MIDI channels
    for(int i=1; i<NUMBER_OF_KBD_PARAMETERS; i++){
        preset.kbdParameter[0][i] = preset.kbdParameter[1][i] = preset.kbdParameter[2][i] = preset.kbdParameter[3][i] = pgm_read_byte(kbdParameterDefaultValue + i);
    }
    // set default MIDI channels
    preset.kbdParameter[0][0] = 0;
    preset.kbdParameter[1][0] = 1;
    preset.kbdParameter[2][0] = 2;
    preset.kbdParameter[3][0] = 3;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// TesMidiController ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////
// Event handler
// It should be called from loop() after all tick() functions.
void TesMIDIController::processEvent(tesEvent *event){
    switch (event->eventType) {
    case tesEvButton:
        processButtonEvent(event);  // call the specific handler of music button-related events
        break;
    case tesEvControlButton:
        processControlButtonEvent(event);  // call the specific handler of control button-related events
        break;
    case tesEvPressure:
        processPressureEvent(event);  // call the specific handler of pressure-related events
        break;
    case tesEvFreeBass:
        processFreeBassEvent(event);  // call the specific handler of FreeBass switch-related events
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  EEPROM-specific constants and variables

#define signatureAddress    1022    // the last 2 bytes of the EEPROM (1022 and 1023)
#define eepromBaseAddress   0


///////////////////////////////////////////////////////////////////////////
// Initializes the instance.
// Should be called from setup()
void    TesMIDIController::init(void){
    // check if we have data in the EEPROM
    uint16_t    signature;
    EEPROM.get(signatureAddress, signature);
    if (signature == TES_EEPROM_SIGNATURE){
        // we have data in EEPROM; now read it
        readGlobalSettingsFromEEPROM();

        // read the active preset
        readPresetFromEEPROM( _settings.global.activePreset );
    }
    else {
        initializeEEPROM();
    }
    // indicate the active preset
    _led_set->setLED( _settings.global.activePreset + ledPreset1, 1);   // the assumption is that Led IDs are sorted (like 0, 1, 2, etc.)

    // initialize the GUI
    _ui.init();

}


///////////////////////////////////////////////////////////////////////////
// the tick() method
void    TesMIDIController::tick(void){
    // check if we're sending parameters to the synthesizer
    if ( _var.sendingParameters ){
        if (millis() - _var.sendParamTimer > sendParamInterval){
            // send the current parameter
            sendKbdParameter(_var.currentKbd, pgm_read_byte(kbdParameterOrder + _var.currentParameter));
            // check if it was the last parameter
            if ((_var.currentKbd == (NUMBER_OF_KEYBOARDS - 1)) && (_var.currentParameter == (NUMBER_OF_KBD_PARAMETERS - 1))){
                // stop the process
                _var.sendingParameters = 0;
            }
            else {
                // select the next parameter
                bool    parameter_is_ok_for_sending = true;     // this value must be overwritten below anyway; but I feel bad when
                                                                // I declare a variable without initialization
                do {
                    if (_var.currentParameter++ == (NUMBER_OF_KBD_PARAMETERS - 1)){
                        // need to switch to the next marked kbd
                        if (_var.currentKbd == (NUMBER_OF_KEYBOARDS - 1)){
                            // *** but it was the last keyboard
                            // so, stop senting parameters
                            _var.sendingParameters = 0;
                            // stop selecting the next parameter for sending
                            break;
                        }
                        _var.currentParameter = 0;      // we start from the 1st parameter for each keyboard
                        // find the next marked kbd (if it exists)
                        while (((_var.currentKbdMap >> ++_var.currentKbd) & 1) == 0){
                            // check if it was the last keyboard
                            if (_var.currentKbd == (NUMBER_OF_KEYBOARDS - 1)){
                                // stop sending parameters
                                _var.sendingParameters = 0;
                                // stop finding a marked kbd
                                break;
                            }
                        }
                    }
                    // if we haven't finished with sending...
                    if (_var.sendingParameters){
                        // check if the selected parameter is marked for sending in the case of optimized update
                        if (_var.optimizedUpdate) {
                            uint8_t parId = pgm_read_byte(kbdParameterOrder + _var.currentParameter);
                            uint8_t byteIndex   = (_var.currentKbd * NUMBER_OF_KBD_PARAMETERS + parId) / 8;
                            uint8_t bitIndex    = (_var.currentKbd * NUMBER_OF_KBD_PARAMETERS + parId) % 8;
                            // get the flag
                            parameter_is_ok_for_sending = (_var.presetUpdateBitmap[byteIndex] >> bitIndex) & 1;
                        }
                        else {
                            // "not optimized" mode; we're sending everything
                            parameter_is_ok_for_sending = true;
                        }
                    }
                } while ( (_var.sendingParameters) && ( ! parameter_is_ok_for_sending) );
            }
            // start the timer for the next parameter
            _var.sendParamTimer   = millis();
        }
    }

    // check if global settings were changed but not yet saved to EEPROM
    if( _var.globalSettingsNotSaved ){
        // check the write timeout
        if (millis() - _var.globalSettingsSavingTimer > GLOBAL_SETTINGS_SAVING_TIMEOUT){
            // it's time to save global settings to EEPROM
            writeGlobalSettingsToEEPROM();
        }
    }

    // send MIDI commands out while the queue is not empty
    // We send only 1 command in each "tick", to allow other components to do their job
    if ( ! _midi_queue.isEmpty() ){
        TesMIDICommand  *midiCommand = _midi_queue.pullCommand();
        sendMIDICommand(midiCommand);
    }

    _ui.tick();
    _auto_drums.tick();
}

///////////////////////////////////////////////////////////////////////////
// constructor
TesMIDIController::TesMIDIController(TesLedSet *led_panel, GyverOLED<SSH1106_128x64> *oled)
    :_ui(oled, this, led_panel),
    _auto_drums(&_midi_queue)
{
    // store parameters
    _led_set      = led_panel;
}

//////////////////////////////////////////////////////////////////////////
// This method processes button events
void TesMIDIController::processButtonEvent(tesEvent *event){
    bool    first_bass_pressed  = false;    // true when we detect the first lower bass note played
    bool    last_bass_released  = false;    // true when we detect the last lower bass note released

    // check if this is a lower-bass note (0 .. 11)
    if (event->buttonId < 12){
        // check if this is the first lower bass note
        first_bass_pressed = ((_var.bassNotesStatus == 0) && (event->buttonEvent == tesBePressed));
        // set the respective bit in the bitmap
        _var.bassNotesStatus = _var.bassNotesStatus & ~((uint16_t)1 << event->buttonId) | ((uint16_t)(event->buttonEvent == tesBePressed ? 1 : 0) << event->buttonId);
        // check if this was the last lower bass note
        last_bass_released = ((_var.bassNotesStatus == 0) && (event->buttonEvent == tesBeReleased));
    }

    // check if bass octaves are disabled
    if(  ! _settings.global.bassOctavesOn){
        // check if this is a PRESSED high-bass note
        if ((event->buttonEvent == tesBePressed) && (event->buttonId >= 12) && (event->buttonId < 24)){
            // calculate the respective low-bass note
            uint8_t lbass = event->buttonId - 12;
            // get the status of the low-bass note
            uint8_t lbstatus = (_var.bassNotesStatus >> lbass) & 1;
            // check if bass octaves are disabled and the low-bass note is pressed
            if( lbstatus ){
                // *** we don't play the high-bass note in this case
                // clear the event (it's fully processed now)
                event->eventType = tesEmpty;
                return;
            }
        }
    }

    // Play the note
    TesMIDICommand  cmd;            // compose a MIDI command
    cmd.midiCommand = mcNoteOn;     // we use only this command for playing notes
    cmd.channelId = _settings.preset.kbdParameter[getKbdId(event->buttonId)][idxChannel];
    cmd.data1       = pgm_read_byte(noteMap + event->buttonId); // take a MIDI note from the mapping table
    cmd.data2       = (event->buttonEvent == tesBePressed)  // assume that events can be
                      ? TES_DEFAULT_VELOCITY : 0;           // either "Pressed" or "Released"
    _midi_queue.pushCommand(&cmd);

    // register chord notes
    bool    first_chord_pressed = false;
    bool    last_chord_released = false;
    if (kbdChord == getKbdId(event->buttonId)){
        if (event->buttonEvent == tesBePressed){
            first_chord_pressed = (_var.numberOfActiveChordNotes++ == 0);
        }
        else {
            last_chord_released = (--_var.numberOfActiveChordNotes == 0);
        }
    }

    // *** handle "manual" Drums
    // They should work only for kbdBass
    if (_var.drumsMode == dmManual){
        if ((kbdBass == getKbdId(event->buttonId)) && first_bass_pressed){
            // playing the "drum for bass" note
            cmd.midiCommand = mcNoteOn;
            cmd.channelId   = _settings.global.drumsMidiChannel;
            cmd.data1       = _settings.preset.drumSoundForBass;
            cmd.data2       = _settings.preset.drumSoundForBassVelocity;
            _midi_queue.pushCommand(&cmd);
        }
        else if ((kbdBass == getKbdId(event->buttonId)) && last_bass_released){
            // stopping the "drum for bass" note
            cmd.midiCommand = mcNoteOn;
            cmd.channelId   = _settings.global.drumsMidiChannel;
            cmd.data1       = _settings.preset.drumSoundForBass;
            cmd.data2       = 0;
            _midi_queue.pushCommand(&cmd);
        }
        else if (first_chord_pressed){
            // playing the "drum for chord" note
            cmd.midiCommand = mcNoteOn;
            cmd.channelId   = _settings.global.drumsMidiChannel;
            cmd.data1       = _settings.preset.drumSoundForChord;
            cmd.data2       = _settings.preset.drumSoundForChordVelocity;
            _midi_queue.pushCommand(&cmd);
        }
        else if (last_chord_released){
            // stopping the "drum for chord" note
            cmd.midiCommand = mcNoteOn;
            cmd.channelId   = _settings.global.drumsMidiChannel;
            cmd.data1       = _settings.preset.drumSoundForChord;
            cmd.data2       = 0;
            _midi_queue.pushCommand(&cmd);
        }
    }
    else if (_var.drumsMode == dmStandby){
        // the drum machine is waiting until a bass note is played
        if (first_bass_pressed){
            // start playing drums
            _ui.setDrumMachineState(dmPlaying);
        }
    }

    // Clear the event. This means that the current event was fully processed and other event handlers
    // should ignore it.
    event->eventType = tesEmpty;
}

//////////////////////////////////////////////////////////////////////////
// This method processes control button events
void TesMIDIController::processControlButtonEvent(tesEvent *event){
    // a control button event

    // just call a dedicated handler, because processing of control buttons may be very complex
    _ui.processCtlButtonEvent(event);
    // we don't clear the event, assuming that GUI did that if the event was fully processed
}

//////////////////////////////////////////////////////////////////////////
// This method processes pressure events
void TesMIDIController::processPressureEvent(tesEvent *event){
    // we cache these values even if the Pressure Sensor is not active at the moment
    _var.lastPressureValue = event->newPressureValue;

    // ignore these events if the pressure sensor is turned off globally
    if( ! _settings.global.pressureSensorOn){
        return; // nothing to do here
    }
    // ok, let's process the new value

    setVolume(_var.lastPressureValue);

    // Clear the event. This means that the current event was fully processed and other event handlers
    // should ignore it.
    event->eventType = tesEmpty;
}

//////////////////////////////////////////////////////////////////////////
// This method processes Free Bass events
void TesMIDIController::processFreeBassEvent(tesEvent *event){
    // change the free bass mode
    _var.freeBassOn = event->freeBassOn;

    // this event requires update of screens/indicators
    _ui.processFreeBassEvent(event);

    // Another keybord has became active. So, we need to re-send the current preset parameters to the new active left keyboard(s),
    // because we didn't send parameters to inactive keyboard(s).
    // Note: we send full sets of parameters here (without "optimization")
    sendCurrentPresetToSynthesizer( _var.freeBassOn ? 0b00001000 : 0b00000110, false );    // either to FreeBass kbd or to Bass+Chord kbds

    // Clear the event. This means that the current event was fully processed and other event handlers
    // should ignore it.
    event->eventType = tesEmpty;
}

//////////////////////////////////////////////////////////////////////////
// the class gets to know about changes of Keyboard-specific Parameters via this method
// This method also implements a bit of "business logic" required in the case when a single parameter is changed
void TesMIDIController::processChangedKbdParameter(uint8_t kbdId, uint8_t    changedParameter){

    switch (changedParameter){
    case idxBank:
        sendKbdParameter(kbdId, idxBank);
        // PC is required after changing the bank, because some synthesizers reset the program when the bank changes
        sendKbdParameter(kbdId, idxProgram);
        break;
    case idxChannel:
    case idxProgram:
    case idxVolume:
    case idxVibrato:
    case idxResonance:
    case idxAttak:
    case idxRelease:
    case idxTimbre:
    case idxDecay:
    case idxReverbration:
    case idxChorus:
    case idxReverbType:
    case idxChorusType:
        sendKbdParameter(kbdId, changedParameter);
        break;
    default:
        SWER(swerMidiController01);
    }
}

//////////////////////////////////////////////////////////////////////////
// the class gets to know about changes of Drums-specific Parameters via this method
void TesMIDIController::processChangedDrumsParameter(uint8_t changedParameter){
    switch (changedParameter){
    case dpDrumsVolume:
        // _settings.preset.drumsVolume
        setDrumsVolume();
        break;
    case dpDrumsetNumber:
        {
            // prepare a MIDI command
            TesMIDICommand  cmd;
            cmd.midiCommand     = mcProgramChange;
            cmd.channelId       = _settings.global.drumsMidiChannel;
            cmd.data1           = _settings.preset.drumsetNumber;
            _midi_queue.pushCommand(&cmd);
        }
        // _settings.preset.drumsetNumber
        break;
    case dpSoundForBass:
    case dpSoundForBassVelocity:
    case dpSoundForChord:
    case dpSoundForChordVelocity:
        break;
    default:
        SWER(swerMidiController06);
    }
}

//////////////////////////////////////////////////////////////////////////
// Sends the specified parameter to the synthesizer.
// No any other business logic.
void TesMIDIController::sendKbdParameter(uint8_t kbdId, uint8_t parameterIndex){
    TesMIDICommand  cmd;
    bool    sendCommand = true; // the flag tells that we should send the MIDI command we're preparing here

    cmd.channelId = _settings.preset.kbdParameter[kbdId][idxChannel];  // common for all commands
    switch (parameterIndex){
    case idxChannel:
        // nothing to do
        sendCommand = false;
        break;
    case idxBank:
        // send CC 0 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 0;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxProgram:
        // send PC <val>
        cmd.midiCommand = mcProgramChange;
        cmd.data1 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxVolume:
        // send CC 7 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 7;  // controller number
        {
            // the master volume should be taken either from the Pressure Sensor or from the global setting
            uint8_t master_volume = (_settings.global.pressureSensorOn) ? _var.lastPressureValue : _settings.preset.masterVolume;
            cmd.data2 = map(master_volume, 0, 127, 0, _settings.preset.kbdParameter[kbdId][parameterIndex]);
        }
        break;
    case idxVibrato:
        // send CC 1 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 1;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxResonance:
        // send CC 71 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 71;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxAttak:
        // send CC 73 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 73;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxRelease:
        // send CC 72 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 72;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxTimbre:
        // send CC 74 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 74;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxDecay:
        // send CC 75 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 75;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxReverbration:
        // send CC 91 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 91;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxChorus:
        // send CC 93 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 93;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxReverbType:
        // send CC 80 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 80;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    case idxChorusType:
        // send CC 81 <val>
        cmd.midiCommand = mcControlChange;
        cmd.data1 = 81;  // controller number
        cmd.data2 = _settings.preset.kbdParameter[kbdId][parameterIndex];
        break;
    default:
        SWER(swerMidiController04);
    }
    // send the prepared command if required
    if(sendCommand){
        _midi_queue.pushCommand(&cmd);
    }
}

//////////////////////////////////////////////////////////////////////////
// Sends drums settings to the synth
void TesMIDIController::sendDrumsSettings(void){
    // *** send the volume
    setDrumsVolume();

    // *** send the drumset
    TesMIDICommand  cmd;
    cmd.midiCommand     = mcProgramChange;
    cmd.channelId       = _settings.global.drumsMidiChannel;
    cmd.data1           = _settings.preset.drumsetNumber;
    _midi_queue.pushCommand(&cmd);
}

/////////////////////////////////////////////////////////////////////////
// Sends a MIDI command to MIDI-out port
void TesMIDIController::sendMIDICommand(TesMIDICommand *cmd){
    static uint8_t  previousStatusByte  = 0;
#ifndef TES_REAL_MIDI_PORT
    Serial.println("=====");
#endif
    if ( !_settings.global.runningStatus || (previousStatusByte != cmd->statusByte) ){
#ifdef TES_REAL_MIDI_PORT
        Serial.write(cmd->statusByte);
#else
        Serial.println(cmd->statusByte, HEX);
#endif
        previousStatusByte = cmd->statusByte;
    }
#ifdef TES_REAL_MIDI_PORT
    Serial.write(cmd->payload[1]);
#else
    Serial.println(cmd->payload[1], HEX);
#endif
    if ( (cmd->midiCommand != mcProgramChange) && (cmd->midiCommand != mcChannelPressure) ) {
#ifdef TES_REAL_MIDI_PORT
        Serial.write(cmd->payload[2]);
#else
        Serial.println(cmd->payload[2], HEX);
#endif
    }
#ifndef TES_REAL_MIDI_PORT
    Serial.flush();
#endif

    // blink the LED
    _led_set->flashLED(ledMidiOut);
}

#define firstRightButton    52
#define lastBassButton      23  // the first 12 bass buttons also use octaves (i.e. they also press buttons from 11 to 23 mechanically in order to make the bass sound richer)

//////////////////////////////////////////////////////////////////////////
// returns a keyboard ID for the specified global button ID
uint8_t TesMIDIController::getKbdId(uint8_t button_id){
    // Keyboard ID depends on which range the specified global button ID belongs to, and also on the status of the Free Bass switch.

    // first, check if a button from the right keyboard was pressed
    if (button_id >= firstRightButton){
        return  kbdRight;
    }
    else if ( _var.freeBassOn ){
        // if the FreeBass mode is turned on, then all button IDs lower than 'firstRightButton' are from the Free Bass keyboard
        return  kbdFreeBass;
    } else {
        // need to check whether it's from Bass or Chord
        if (button_id <= lastBassButton){
            // this is a Bass button
            return  kbdBass;
        } else {
            // this is a Chord button
            return  kbdChord;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Sets the volume of all actuve MIDI channels according to the specified Master Volume value
// (puts respective MIDI commands into the MIDI-out queue)
void TesMIDIController::setVolume(uint8_t new_master_volume){
    // prepare a MIDI command
    TesMIDICommand  cmd;
    cmd.midiCommand     = mcControlChange;
    cmd.data1           = 7;        // Volume controller
    /////////// and send it to all active channels
    // the right keyboard is always active
    cmd.channelId       = _settings.preset.kbdParameter[kbdRight][idxChannel];
    // make a proportional change (according to the fixed volume setting for the channel)
    cmd.data2           = map(new_master_volume, 0, 127, 0, _settings.preset.kbdParameter[kbdRight][idxVolume]);
    _midi_queue.pushCommand(&cmd);
    if ( _var.freeBassOn ){
        cmd.channelId       = _settings.preset.kbdParameter[kbdFreeBass][idxChannel];
        // make a proportional change (according to the fixed volume setting for the channel)
        cmd.data2           = map(new_master_volume, 0, 127, 0, _settings.preset.kbdParameter[kbdFreeBass][idxVolume]);
        _midi_queue.pushCommand(&cmd);
    }
    else {
        cmd.channelId       = _settings.preset.kbdParameter[kbdBass][idxChannel];
        // make a proportional change (according to the fixed volume setting for the channel)
        cmd.data2           = map(new_master_volume, 0, 127, 0, _settings.preset.kbdParameter[kbdBass][idxVolume]);
        _midi_queue.pushCommand(&cmd);
        // make a proportional change (according to the fixed volume setting for the channel)
        cmd.data2           = map(new_master_volume, 0, 127, 0, _settings.preset.kbdParameter[kbdChord][idxVolume]);
        cmd.channelId       = _settings.preset.kbdParameter[kbdChord][idxChannel];
        _midi_queue.pushCommand(&cmd);
    }
}

//////////////////////////////////////////////////////////////////////////
// Sets the volume for the Drums channel (basing on current values of MasterVolume and DrumsVolume
// (puts respective MIDI commands into the MIDI-out queue)
void TesMIDIController::setDrumsVolume(void){
    // prepare a MIDI command
    TesMIDICommand  cmd;
    cmd.midiCommand     = mcControlChange;
    cmd.channelId       = _settings.global.drumsMidiChannel;
    cmd.data1           = 7;        // Volume controller
    // make a proportional change (according to the fixed volume setting for the channel)
    cmd.data2           = map(_settings.preset.masterVolume, 0, 127, 0, _settings.preset.drumsVolume);
    _midi_queue.pushCommand(&cmd);
}

//////////////////////////////////////////////////////////////////////////
// Toggles the pressure sensor mode (on/off)
void TesMIDIController::togglePressureSensor(void){
  // toggle the setting
  _settings.global.pressureSensorOn = ! _settings.global.pressureSensorOn;
  // The "source of the Master Volume value" has been changed.
  // So, we need to update active channel's volume according to the new source.

  setVolume( _settings.global.pressureSensorOn ? _var.lastPressureValue : _settings.preset.masterVolume );

  // this is part of global settings
  globalSettingsChangeNotification();
}



//////////////////////////////////////////////////////////////////////////
// Initializes the EEPROM
void TesMIDIController::initializeEEPROM(void){

    // write the global settings
    writeGlobalSettingsToEEPROM();

    // write 6 presets with the same default values
    for (int i = 0; i<6; i++){
        // just write the current settings to the respective slot
        writePresetToEEPROM(i);
    }

    // write the signature (into the pre-defined address)
    uint16_t    signature = TES_EEPROM_SIGNATURE;
    EEPROM.put(signatureAddress, signature);
}

//////////////////////////////////////////////////////////////////////////
// Reads Global Settings from the EEPROM
void TesMIDIController::readGlobalSettingsFromEEPROM(void){
    // read the global settings
    int address = eepromBaseAddress;
    EEPROM.get(address, _settings.global);
}

//////////////////////////////////////////////////////////////////////////
// Writes Global Settings to the EEPROM
void TesMIDIController::writeGlobalSettingsToEEPROM(void){
    // write the global settings
    int address = eepromBaseAddress;
    EEPROM.put(address, _settings.global);
    // reset the "not saved" flag
    _var.globalSettingsNotSaved = 0;
}

//////////////////////////////////////////////////////////////////////////
// Reads the specified preset from the EEPROM
void TesMIDIController::readPresetFromEEPROM(uint8_t preset_id){
    TesMIDIControllerSettings::TesMIDIControllerPreset     new_preset;      // yes, heavy stack usage
    // calculate the EEPROM address
    int address = eepromBaseAddress + sizeof(_settings.global) + preset_id * sizeof(_settings.preset);
    // read the preset settings
    EEPROM.get(address, new_preset);
    // Special case: if the Master Volume values differ, then we need to send volume parameters anyway.
    if (_settings.preset.masterVolume != new_preset.masterVolume) {
        // set "volume" values in the old preset to "impossible" 0xFF value, so that
        // the loop below catches the difference and marks Volume parameters for sending
        _settings.preset.kbdParameter[0][idxVolume]
            = _settings.preset.kbdParameter[1][idxVolume]
            = _settings.preset.kbdParameter[2][idxVolume]
            = _settings.preset.kbdParameter[3][idxVolume]
            = 0xFF;
    }
    // save the difference between the old and new presets in the bitmap
    for(int kbd=0; kbd<NUMBER_OF_KEYBOARDS; kbd++){
        for(int i=0; i<NUMBER_OF_KBD_PARAMETERS; i++){
            uint8_t byteIndex   = (kbd * NUMBER_OF_KBD_PARAMETERS + i) / 8;
            uint8_t bitIndex    = (kbd * NUMBER_OF_KBD_PARAMETERS + i) % 8;
            // set the bit value
            uint8_t value = (new_preset.kbdParameter[kbd][i] != _settings.preset.kbdParameter[kbd][i]);
            _var.presetUpdateBitmap[byteIndex] = _var.presetUpdateBitmap[byteIndex] & ~((uint8_t)1 << bitIndex) | (value << bitIndex);
        }
    }
    // replace the old preset with the new one
    memcpy(&_settings.preset, &new_preset, sizeof(new_preset));
}

//////////////////////////////////////////////////////////////////////////
// Writes the specified preset to the EEPROM
void TesMIDIController::writePresetToEEPROM(uint8_t preset_id){
    // calculate the EEPROM address
    int address = eepromBaseAddress + sizeof(_settings.global) + preset_id * sizeof(_settings.preset);
    // write the preset settings
    EEPROM.put(address, _settings.preset);
    // reset the "not saved" flag
    _var.currentPresetNotSaved = 0;
}

//////////////////////////////////////////////////////////////////////////
// Activates the specified preset
void TesMIDIController::activatePreset(uint8_t preset_id){
    // read the preset settings from EEPROM
    readPresetFromEEPROM(preset_id);

    // change the active preset ID
    _settings.global.activePreset = preset_id;
    // clear the "preset not saved" flag
    _var.currentPresetNotSaved = 0;

    // send the parameters to the synthesizer
    sendCurrentPresetToSynthesizer( _var.freeBassOn ? 0b00001001 : 0b00000111 );
}

//////////////////////////////////////////////////////////////////////////
// Sends all Kbd Parameters from the current preset to the synthesizer
void TesMIDIController::sendCurrentPresetToSynthesizer(uint8_t kbd_map, bool optimized){
    // start sending kbd parameters

    // check if we're already sending parameters
    if ( _var.sendingParameters ){
        // this process cannot overlap
        SWER(swerMidiController03);
    }

    // Always set the drums volume right away (it's just one MIDI cmd)
    setDrumsVolume();

    // *** prepare for sending parameters for kbd channels

    // save the kbdMap
    _var.currentKbdMap  = kbd_map;
    if (_var.currentKbdMap == 0){
        // the method was called with an incorrect kbd map
        SWER(swerMidiController05);
    }

    // set the flag
    _var.sendingParameters = 1;
    _var.optimizedUpdate = optimized;
    // initialize aux variables
    _var.currentKbd       = 0;
    // find the first marked kbd
    while ( ((_var.currentKbdMap >> _var.currentKbd) & 1) == 0){
        _var.currentKbd++;
    }
    _var.currentParameter = 0;      // in the reality, we won't send a MIDI command for this parameter, because it's "the MIDI channel"
                                    // So, this initial selection will not spoil the "optimized" update if it is requested.
                                    // Proper selection of parameters is done in tick().
    _var.sendParamTimer   = millis();

    // actual work will be done in tick()
}

//////////////////////////////////////////////////////////////////////////
// This menthod should be called each time whe global settings are changed
void TesMIDIController::globalSettingsChangeNotification(void){
    // set a flag of not saved global settings
    _var.globalSettingsNotSaved = true;
    // start the timer for saving the global settings to EEPROM
    _var.globalSettingsSavingTimer = millis();
}

