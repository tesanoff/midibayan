////////////////////////////////////////////////////////////////////////////////////////
//      MIDI controller
////////////////////////////////////////////////////////////////////////////////////////
#ifndef _TES_MIDI_CONTROLLER_H
#define _TES_MIDI_CONTROLLER_H
#include "TesEventQueue.h"
#include "TesMidiUI.h"
#include "TesAutoDrums.h"
#include "swer.h"

#define TES_REAL_MIDI_PORT

/////////////////////////////////////////////////////////////
// auxiliary stuff

// My bayan keyboard is not pressure sensitive. So, we're using the default velocity for all notes.
#define TES_DEFAULT_VELOCITY 127

// The default master volume
#define TES_DEFAULT_MASTER_VOLUME   70     // middle level

//////////////////////////////////////////
// MIDI command definitions
#define mcNoteOff                   0x8
#define mcNoteOn                    0x9
#define mcPolyphonicKeyPressure     0xA
#define mcControlChange             0xB
#define mcProgramChange             0xC
#define mcChannelPressure           0xD
#define mcPitchBandChange           0xE


//////////////////////////////////////////
// MIDI command type
#define TES_MIDI_COMMAND_MAX_LENGTH 3
struct TesMIDICommand {
    union {
        // byte representation; to be used for sending
        uint8_t payload[ TES_MIDI_COMMAND_MAX_LENGTH ];
        // structure representation; to be used for operations
        struct {
            // status byte
            union {
                // byte representation; for fast comparing (see "Running Status" chapter in the MIDI documentation)
                uint8_t statusByte;
                // structure representation; to be used for operations
                struct {
                    uint8_t channelId: 4;   // MIDI Channel ID (0 .. 15)
                    uint8_t midiCommand: 4; // MIDI Command (0x8 .. 0xE)
                };
            };
            // Data 1 byte
            struct {
                uint8_t data1: 7;       // data 1 value (0 .. 127)
                uint8_t data1Bit: 1;    // must always be 0
            };
            // Data 2 byte
            struct {
                uint8_t data2: 7;       // data 1 value (0 .. 127)
                uint8_t data2Bit: 1;    // must always be 0
            };
        };
    };
    // the constructor sets all bytes to 0 in order to ensure data bits are set to 0
    TesMIDICommand(void){
        for(int i=0; i<TES_MIDI_COMMAND_MAX_LENGTH; i++){
            payload[i] = 0;
        }
    }
};

//////////////////////////////////////////
// MIDI-out queue
#define TES_MIDI_QUEUE_SIZE     15
class TesMIDIOutQueue {
private:
    TesMIDICommand  _command[ TES_MIDI_QUEUE_SIZE  ];
    uint8_t         _head;
    uint8_t         _tail;
    uint8_t         _size;

public:
    // construstor
    TesMIDIOutQueue (){
        _head = 0;
        _tail = 0;
        _size = 0;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // puts a command to the head of the queue
    //
    // Returns:
    // - true - successfully done
    // - false - couldn't put a message to the queue, because the queue is full
    bool pushCommand(TesMIDICommand *command){
        // check if the queue is full (i.e. there's no space for another event)
        if ( _size == TES_MIDI_QUEUE_SIZE ){
            // go to SWER
            SWER(swerMidiController02);
        }
        // ok, let's continue

        // advance the head
        if( ++_head == TES_MIDI_QUEUE_SIZE ){
            // correct the head, if required
            _head = 0;
        }
        // put the command to the head
        memcpy((_command + _head), command, sizeof(TesMIDICommand));
        _size++;    // don't forget it
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // returns True if the queue is empty
    bool isEmpty(void){
        return (_size == 0);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // returns the current size of the queue
    uint8_t getSize(void){
        return _size;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // returns an event from the tail of the queue
    TesMIDICommand* pullCommand(void){
        // return NULL if the queue is empty;
        if ( isEmpty() ){
            return NULL;
        }
        // advance the tail
        if( ++_tail == TES_MIDI_QUEUE_SIZE ){
            // correct the tail, if required
            _tail = 0;
        }
        _size--;    // don't forget it
        return (_command + _tail);
    }
};

//////////////////////////////////////////
// a configuration of MIDI controller
#define NUMBER_OF_KEYBOARDS 4
#define kbdRight        0
#define kbdBass         1
#define kbdChord        2
#define kbdFreeBass     3

// define Indices for parameters
#define idxChannel          0
#define idxBank             1
#define idxProgram          2
#define idxVolume           3
#define idxVibrato          4
#define idxResonance        5
#define idxAttak            6
#define idxRelease          7
#define idxTimbre           8
#define idxDecay            9
#define idxReverbration     10
#define idxChorus           11
#define idxReverbType       12
#define idxChorusType       13

#define NUMBER_OF_KBD_PARAMETERS    14
#define TES_EEPROM_SIGNATURE 0x5A0F  // 0x5Avv, where: 5A - signature, vv - version
#define GLOBAL_SETTINGS_SAVING_TIMEOUT  8000    // changes are saved to EEPROM in 8 seconds after the last change

#define SIZE_OF_PRESET_UPDATE_BITMAP    (((NUMBER_OF_KEYBOARDS * NUMBER_OF_KBD_PARAMETERS) -  1) / 8 + 1)

// synthesizer types
enum    {stGeneralMidi = 0, stAtemp, stProDX};

struct TesMIDIControllerSettings {
   struct TesMIDIControllerGlobalSettings { 
        struct {
            uint8_t     pressureSensorOn: 1;    // true - the pressure sensor is enabled; false - disabled
            uint8_t     runningStatus: 1;       // true - running status enabled; false = disabled
            uint8_t     synthType: 2;           // let's reserve range 0 .. 3
            uint8_t     bassOctavesOn: 1;       // true - if octaves are allowed for bass keyboard
            uint8_t     activePreset: 3;        // 0 .. 5
            uint8_t     drumsMidiChannel: 4;    // a MIDI channel for drums
        };
        TesFKeyProfile  fkeyProfile[4];       // move it to system data that can be stored in EEPROM
    } global;

    struct TesMIDIControllerPreset {
        uint8_t         masterVolume;           // the master volume setting
        uint8_t         drumsVolume;            // volume for drums
        uint8_t         drumsetNumber;          // drumset ID (0..127)
        uint8_t         drumSoundForBass;
        uint8_t         drumSoundForBassVelocity;
        uint8_t         drumSoundForChord;
        uint8_t         drumSoundForChordVelocity;
        uint8_t kbdParameter[NUMBER_OF_KEYBOARDS][NUMBER_OF_KBD_PARAMETERS];  // yeah, that's a lot :(
    } preset;

    TesMIDIControllerSettings (void);
};

// interval between sending preset parameters to the synthesizer
#define sendParamInterval   5       // in milliseconds

// drums mode IDs
enum {dmOff = 0, dmManual, dmStandby, dmPlaying};

// Some parameters stored in MIDI controller, but not stored in EEPROM
struct TesMIDIControllerRunTimeData {
    uint8_t     lastPressureValue;          // the last value received from the Pressure Sensor
    struct {
        uint8_t     currentPresetNotSaved: 1;   // true - if the current preset has changes not saved in the EEPROM
        uint8_t     freeBassOn: 1;              // the current mode of the left keyboard
        uint8_t     sendingParameters: 1;       // true - when we're sending parameters to the synthesizer
        uint8_t     optimizedUpdate: 1;         // true - when we need to send a parameter only if it differs from the same parameter in the previous preset
        uint8_t     currentKbdMap: 4;           // - bitmap of Kbds to be updated (0001 - right, 0010 - bass, 0100 - chord, 1000 - freebass)
        uint8_t     currentKbd: 2;              // - the kbd being processed (0..3)
        uint8_t     currentParameter: 5;        // - the parameter being processed (up to 32)
        uint8_t     globalSettingsNotSaved: 1;  // true - if global settings were changed, but not yet saved to EEPROM
        uint8_t     numberOfActiveChordNotes: 4;    // this is for correct playing "drums for chords"
        uint8_t     drumsMode: 2;               // 0 - off, 1 - manual, 2 - standby, 3 - playing
    };
    uint16_t        sendParamTimer;             // to measure intwervals between sending each parameter of the preset
    uint16_t        globalSettingsSavingTimer;  // for saving global settings to EEPROM
    uint16_t        bassNotesStatus;            // bitmap stores status of each lower-bass notes: 1 - on, 0 - off
    uint8_t         presetUpdateBitmap[SIZE_OF_PRESET_UPDATE_BITMAP];   // each bit represents 1 preset parameter
                                                // used when preset parameters are sent to the synth to minimize 
                                                // the number of MIDI commands

    TesMIDIControllerRunTimeData(void){
        currentPresetNotSaved = false;
        sendingParameters = false;
        globalSettingsNotSaved = false;
        bassNotesStatus = 0;
        numberOfActiveChordNotes = 0;
        drumsMode = dmOff;
    }
};

class TesAutoDrums;
/////////////////////////////////////////////////////////////////////
// the MIDI controller itself
class TesMIDIController  {
    friend class TesMidiUI;
public:
    ///////////////////////////////////////////////////////////////////////////
    // Event handler
    // It should be called from loop() after all tick() functions.
    void processEvent(tesEvent *event);

    ///////////////////////////////////////////////////////////////////////////
    // Initializes the instance.
    // Should be called from setup()
    void    init(void);

    ///////////////////////////////////////////////////////////////////////////
    // the tick() method
    void    tick(void);

    ///////////////////////////////////////////////////////////////////////////
    // constructor
    TesMIDIController(TesLedSet *led_panel, GyverOLED<SSH1106_128x64> *oled);

private:
    TesMidiUI              _ui;           // a class responsible for processing control button events
    TesAutoDrums            _auto_drums;    // a class playing auto drums
    TesLedSet               * _led_set;     // the LED panel to be used
    TesMIDIOutQueue         _midi_queue;    // the queue of ougtoing MIDI commands
    TesMIDIControllerSettings   _settings;  // the current settings of the controller
    TesMIDIControllerRunTimeData    _var;   // run-time data (not stored in EEPROM), like /var folder in unix

    //////////////////////////////////////////////////////////////////////////
    // This method processes music button events
    void processButtonEvent(tesEvent *event);

    //////////////////////////////////////////////////////////////////////////
    // This method processes control button events
    void processControlButtonEvent(tesEvent *event);

    //////////////////////////////////////////////////////////////////////////
    // This method processes pressure events
    void processPressureEvent(tesEvent *event);

    //////////////////////////////////////////////////////////////////////////
    // This method processes Free Bass events
    void processFreeBassEvent(tesEvent *event);

    //////////////////////////////////////////////////////////////////////////
    // the class gets to know about changes of Keyboard-specific Parameters via this method
    void processChangedKbdParameter(uint8_t kbdId, uint8_t changedParameter);

    //////////////////////////////////////////////////////////////////////////
    // the class gets to know about changes of Drums-specific Parameters via this method
    void processChangedDrumsParameter(uint8_t changedParameter);

    //////////////////////////////////////////////////////////////////////////
    // Sends the specified parameter to the synthesizer.
    // No any other business logic.
    void sendKbdParameter(uint8_t kbdId, uint8_t parameterIndex);

    //////////////////////////////////////////////////////////////////////////
    // Sends drums settings to the synth
    void sendDrumsSettings(void);

    //////////////////////////////////////////////////////////////////////////
    // Sends a MIDI command to MIDI-out port
    void sendMIDICommand(TesMIDICommand *cmd);

    //////////////////////////////////////////////////////////////////////////
    // returns a keyboard ID for the specified global button ID
    uint8_t getKbdId(uint8_t button_id);

    //////////////////////////////////////////////////////////////////////////
    // Sets the volume of all actuve MIDI channels according to the specified Master Volume value
    // (puts respective MIDI commands into the MIDI-out queue)
    void setVolume(uint8_t new_master_volume);
    
    //////////////////////////////////////////////////////////////////////////
    // Sets the volume for the Drums channel (basing on current values of MasterVolume and DrumsVolume
    // (puts respective MIDI commands into the MIDI-out queue)
    void setDrumsVolume(void);
    
    //////////////////////////////////////////////////////////////////////////
    // Toggles the pressure sensor mode (on/off)
    void togglePressureSensor(void);

    //////////////////////////////////////////////////////////////////////////
    // Initializes the EEPROM
    void initializeEEPROM(void);

    //////////////////////////////////////////////////////////////////////////
    // Reads Global Settings from the EEPROM
    void readGlobalSettingsFromEEPROM(void);

    //////////////////////////////////////////////////////////////////////////
    // Writes Global Settings to the EEPROM
    void writeGlobalSettingsToEEPROM(void);

    //////////////////////////////////////////////////////////////////////////
    // Reads the specified preset from the EEPROM
    void readPresetFromEEPROM(uint8_t preset_id);

    //////////////////////////////////////////////////////////////////////////
    // Writes the specified preset to the EEPROM
    void writePresetToEEPROM(uint8_t preset_id);

    //////////////////////////////////////////////////////////////////////////
    // Activates the specified preset
    void activatePreset(uint8_t preset_id);

    //////////////////////////////////////////////////////////////////////////
    // Sends all Kbd Parameters from the current preset to the synthesizer
    void sendCurrentPresetToSynthesizer(uint8_t kbd_map, bool optimized = true);

    //////////////////////////////////////////////////////////////////////////
    // This menthod should be called each time whe global settings are changed
    void globalSettingsChangeNotification(void);
};

////////////////////////////////////////////////////////////////////////////////////////
//      MIDI controller
////////////////////////////////////////////////////////////////////////////////////////
#endif

