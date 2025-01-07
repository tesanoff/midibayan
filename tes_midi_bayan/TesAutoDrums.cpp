
#include "TesAutoDrums.h"
#include "midi_note_names.h"
#include "TesMidiController.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// callback functions for MD_MIDIFIle
///

// MIDI handler for scanning
void midi_handler_scanner(midi_event *pev){
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.midiEventScanner(pev);
}

// META handler for scanning
void meta_handler_scanner(const meta_event *p){
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.metaEventScanner(p);
}

// MIDI handler for playing
void midi_handler_player(midi_event *pev){
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.midiEventPlayer(pev);
}

// META handler for playing
void meta_handler_player(const meta_event *p){
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.metaEventPlayer(p);
}
///
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Settings

//#define SD_CARD_CS  26   // for the real version
#define SD_CARD_CS  9   // for the breadboard debug version

#define MAX_FILE_NAME_LENGTH    120

#define drumsChannel    9
#define defaultVolume   70

#define INIT_PERIOD     25  // ms

///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesAutoDrums::TesAutoDrums(TesMIDIOutQueue * midi_queue){
    _midi_queue = midi_queue;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the engine
void    TesAutoDrums::init(void){
    SdSpiConfig sd_config(SD_CARD_CS, SHARED_SPI, SPI_FULL_SPEED, &SPI1);

    if(!_SD.begin(sd_config)){
        PRINT_1("*** SD Card init failed!");
        SWER(0xF0); // TODO assign a proper SWER id
    }
    PRINT_1("SD Card - ok");

    _SMF.begin(&_SD);

    // Now, we need to read names of all files in the root directory, assuming that all of them are MIDI files
    SDDIR   root;
    SDFILE  file;

    if(!root.open("/")){
        PRINT_1("Could not open root directory");
        // TODO handle the error properly
    }
    else{
        while(file.openNext(&root, O_RDONLY)){
            char    buffer[MAX_FILE_NAME_LENGTH];
            size_t name_length = file.getName(buffer, MAX_FILE_NAME_LENGTH);
            PRINT_2("name length =", name_length);
            if(!file.isDirectory()){
                _var.file_names.push_back(buffer);
            }
            else {
                PRINT_2("Skipping directory:", buffer);
            }
            file.close();
        }
    }
    PRINT_2("Collected names = ", _var.file_names.size());
    for(int i=0; i<_var.file_names.size();i++){
        PRINT_1(_var.file_names[i]);
    }
    // *** The initial list of file names has been composed.
    //     Now, it's time to start validating those files.
    _var.init_in_progress   = true;
    _var.melodyId           = 0;    // we start with this one
    _SMF.setMidiHandler(midi_handler_scanner);
    _SMF.setMetaHandler(meta_handler_scanner);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Starts playing the current melody
void    TesAutoDrums::start(void){
    if (!_var.is_initialized){
        // the engine is not Initialized yet
        return;
    }
    // *** The assumption is that is a melody exists in the list - then it's validated successfully
    PRINT_2("Starting melody: ", _var.file_names[_var.melodyId]);
    // open a melody
    int err = _SMF.load(_var.file_names[_var.melodyId].c_str());
    if (err != MD_MIDIFile::E_OK){
        PRINT_2("ERROR = ", err);
        return;
    }
    // TODO turn "looping" on, if required
    // set a flag
    _var.is_playing = true;
    // set the drumset
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcProgramChange;
    cmd.data1   = 0;    // TODO rework this (put a proper drumset)
    _midi_queue->pushCommand(&cmd);
    // The drumset can be changed if the MIDI file has a respecitve command; but we don't care about such cases.
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Stops playing the current melody
void    TesAutoDrums::stop(void){
    if(!_var.is_playing){
        return;
    }
    _SMF.close();
    _var.is_playing = false;
    // stop playing all notes in the channel (in case if they're playing)
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcControlChange;
    cmd.data1   = 123;  // controller number; "all notes off"
    _midi_queue->pushCommand(&cmd);
    // TODO remove the code below
    {
        PRINT_2("old melody id = ", _var.melodyId);
        setNextMelodyId();
        PRINT_2("new melody id = ", _var.melodyId);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Gets the current tempo value
uint8_t TesAutoDrums::getTempo(void){
    if(!_var.is_initialized){
        return 0;
    }
    return _var.tempo;  // no need to complicate yet - TODO revise this
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the current tempo value
void    TesAutoDrums::setTempo(uint8_t    tempo){
    if(!_var.is_initialized){
        return;
    }
    // TODO re-implement this
    _var.tempo = tempo; // no need to complicate yet
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns an ID of the current melody
uint8_t TesAutoDrums::getCurrentMelodyId(void){
    if(!_var.is_initialized){
        return 0;
    }
    return _var.melodyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the specified melody as the current
void TesAutoDrums::setMelodyId(uint8_t melody_id){
    if(!_var.is_initialized){
        return;
    }

    if ( _var.is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( melody_id >= _var.file_names.size()){
        SWER(swerAutoDrums02);
    }

    _var.melodyId = melody_id;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the next melody as the current
void TesAutoDrums::setNextMelodyId(void){
    if(!_var.is_initialized){
        return;
    }

    if ( _var.is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( ++_var.melodyId == _var.file_names.size()){
        _var.melodyId = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the previous melody as the current
void TesAutoDrums::setPreviousMelodyId(void){
    if(!_var.is_initialized){
        return;
    }

    if ( _var.is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( _var.melodyId-- == 0){
        _var.melodyId = _var.file_names.size() - 1;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns a string with the current melody name
const char *    TesAutoDrums::getCurrentMelodyName(void){
    if(!_var.is_initialized){
        return NULL;
    }

    return _var.file_names[_var.melodyId].c_str();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns true is a melody is playing
bool    TesAutoDrums::isPlaying(void){
    return (_var.is_initialized && _var.is_playing);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// the tick() method
void    TesAutoDrums::tick(void){

    // check if we're initializing
    if ( _var.init_in_progress ){
        if(millis() - _var.init_timer > INIT_PERIOD){
            validateMidiFile();
            // reset the timer
            _var.init_timer = millis();
        }
    }

    // check if we're playing a melody
    if ( _var.is_playing ){
        // standard playing of the current MIDI file
        if(!_SMF.isEOF()){
            _SMF.getNextEvent();
        }
        else{
            // EOF is reached, and the file was not set for looping.
            // So, we need to stop playing.
            _var.is_playing = false; // TODO rework this. This is a temporary version
                                     // we will need to adjust the state of drum machine somehow.
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////         Private Section             //////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
// receives MIDI events during MIDI files scanning
void TesAutoDrums::midiEventScanner(midi_event *pev){
/*
 *typedef struct
 *{
 *  uint8_t track;    ///< the track this was on
 *  uint8_t channel;  ///< the midi channel
 *  uint8_t size;     ///< the number of data bytes
 *  uint8_t data[4];  ///< the data. Only 'size' bytes are valid
 *} midi_event;
 */

    // check if the MIDI channel is correct
    if(pev->channel != drumsChannel){
        // the file cannot send MIDI commands to non-drums channels
        _var.validation_data.is_valid = false;
    }

    // TODO implement the rest!

    // count MIDI commands
    _var.validation_data.number_of_commands++;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives META events during MIDI files scanning
void TesAutoDrums::metaEventScanner(const meta_event *pmev){
/*
 *typedef struct
 *{
 *  uint8_t track;    ///< the track this was on
 *  uint16_t size;    ///< the number of data bytes
 *  uint8_t type;     ///< meta event type
 *  union 
 *  {
 *    uint8_t data[50]; ///< byte data. Only 'size' bytes are valid
 *    char chars[50];   ///< string data. Only 'size' bytes are valid
 *  };
 *} meta_event;
 */

    // TODO implement this!
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives MIDI events during MIDI files playing
void TesAutoDrums::midiEventPlayer(midi_event *pev){
    // prepare the MIDI command and put it into the queue
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;           // no other option, as the MIDI file was validated already
    cmd.midiCommand = pev->data[0] >> 4;    // the MD_MIDIFile library uses the 0xV0 format for commands (where "V" is the "value")
    cmd.data1 = pev->data[1];
    cmd.data2 = pev->data[2];   // this makes sense only for 3-bytes commands, but it's safe to always copy it
    _midi_queue->pushCommand(&cmd);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives META events during MIDI files playing
void TesAutoDrums::metaEventPlayer(const meta_event *pmev){
/*
 *typedef struct
 *{
 *  uint8_t track;    ///< the track this was on
 *  uint16_t size;    ///< the number of data bytes
 *  uint8_t type;     ///< meta event type
 *  union 
 *  {
 *    uint8_t data[50]; ///< byte data. Only 'size' bytes are valid
 *    char chars[50];   ///< string data. Only 'size' bytes are valid
 *  };
 *} meta_event;
 */
    // debug print
    PRINT_1("============= META event (player) ===============");
    PRINT_2_HEX("track   =\t", pmev->track);
    PRINT_2_HEX("type    =\t", pmev->type);
    PRINT_2_HEX("size    =\t", pmev->size);
    for (int i=0;i<pmev->size;i++){
        PRINT_2_HEX("data    =\t", pmev->data[i]);
    }

    // TODO implement this!
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Validates the current MIDI file
void    TesAutoDrums::validateMidiFile(void){
    PRINT_2("Start validation for: ", _var.file_names[_var.melodyId].c_str());
    // _var.melodyId points to the file to be validated now
    // open a melody
    int err = _SMF.load(_var.file_names[_var.melodyId].c_str());
    if (err != MD_MIDIFile::E_OK){
        PRINT_2("ERROR = ", err);
        return;
    }
    _var.validation_data.clear();   // clear the validation data

    // TODO Obtain initial information from the file header (tempo, etc.)
    //      Also, decide if the file should be looped.

    // run the validation
    if(!_SMF.isEOF()){
        _SMF.scanAllTracks();
    }
    _SMF.close();
    // *** _var.validation_data contains the validation result

    if (_var.validation_data.is_valid){
        PRINT_1("This file is valid");
        // TODO add MIDI file attributes to the respective list
        // move the pointer to the next MIDI file
        _var.melodyId++;
    }
    else{
        // *** The file is not valid.
        PRINT_1("This file is NOT valid");
        // remove it from the list
        _var.file_names.erase(_var.file_names.begin() + _var.melodyId);
        // _var.melodyId points to the next file now
    }

    // check if the next file exists
    if(_var.melodyId == _var.file_names.size()){
        PRINT_1("All MIDI files have been validated");
        // *** all files were validated
        _var.init_in_progress   = false;
        _var.is_initialized     = true;
        _SMF.setMidiHandler(midi_handler_player);
        _SMF.setMetaHandler(meta_handler_player);
        // set the default melodyId
        _var.melodyId           = 0;
    }
}
