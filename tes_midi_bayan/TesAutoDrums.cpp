
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

#define defaultDrumset  0

#define INIT_PERIOD     25  // ms

String  loopPrefix("/LOOP/");
String  singlePrefix("/SINGLE/");

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
        SWER(swerAutoDrums03);
    }
    PRINT_1("SD Card - ok");

    _SMF.begin(&_SD);

    // Now, we need to read names of all files in the root directory, assuming that all of them are MIDI files
    SDDIR   root;
    SDFILE  file;

    if(!root.open(loopPrefix.c_str())){
        PRINT_1("Could not open LOOP directory");
    }
    else{
        while(file.openNext(&root, O_RDONLY)){
            char    buffer[MAX_FILE_NAME_LENGTH];
            loopPrefix.toCharArray(buffer, MAX_FILE_NAME_LENGTH);
            size_t name_length = file.getName(buffer+loopPrefix.length(), MAX_FILE_NAME_LENGTH - loopPrefix.length());
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
    root.close();
    if(!root.open(singlePrefix.c_str())){
        PRINT_1("Could not open SINGLE directory");
    }
    else{
        while(file.openNext(&root, O_RDONLY)){
            char    buffer[MAX_FILE_NAME_LENGTH];
            singlePrefix.toCharArray(buffer, MAX_FILE_NAME_LENGTH);
            size_t name_length = file.getName(buffer+singlePrefix.length(), MAX_FILE_NAME_LENGTH - singlePrefix.length());
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

    if(_var.file_names.size() > 0){
        // *** The initial list of file names has been composed.
        //     Now, it's time to start validating those files.
        _var.init_in_progress   = true;
        _var.melodyId           = 0;    // we start with this one
        _SMF.setMidiHandler(midi_handler_scanner);
        _SMF.setMetaHandler(meta_handler_scanner);
    }
    else{
        // *** No MIDI files detected

        // we consider the engine not initialized
        _var.init_in_progress   = false;
        _var.is_initialized     = false;
    }
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
    // turn "looping" on, if required
    _SMF.looping( _var.file_attributes[_var.melodyId].loop );
    // set a flag
    _var.is_playing = true;
    // set the initial tempo
    _SMF.setTempo(_var.user_defined_tempo);      // Initially, it's the same as the value encoded in the file.
                                    // But the user can change it in _var.user_defined_tempo, so, _var.user_defined_tempo is the best place to take
                                    // the initial value of tempo from.
    // set the drumset
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcProgramChange;
    cmd.data1   = defaultDrumset;
    _midi_queue->pushCommand(&cmd);
    // The drumset can be changed if the MIDI file has a respecitve command; but we don't care about such cases.
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Stops playing the current melody
void    TesAutoDrums::stop(void){
    _SMF.close();
    _var.is_playing = false;
    // stop playing all notes in the channel (in case if they're playing)
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcControlChange;
    cmd.data1   = 123;  // controller number; "all notes off"
    _midi_queue->pushCommand(&cmd);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Gets the current user-defined tempo value
uint16_t TesAutoDrums::getTempo(void){
    if(!_var.is_initialized){
        return 0;
    }
    return _var.user_defined_tempo;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the current tempo value
void    TesAutoDrums::setTempo(uint16_t    tempo){
    if(!_var.is_initialized){
        return;
    }
    // First, we store the new value of the user-defined tempo
    _var.user_defined_tempo = tempo;
    // check if we need to adjust the current playback tempo as well
    if(_var.is_playing){
        // The new value of the current tempo should be adjusted according to the new difference
        // between the user-defined tempo and the "native" MIDI file tempo stored in the MIDI file attributes.
        uint16_t    new_tempo   = map(_var.native_tempo, 0, _var.file_attributes[_var.melodyId].tempo, 0, _var.user_defined_tempo);
        _SMF.setTempo(new_tempo);
        PRINT_2("user defined tempo = ", _var.user_defined_tempo);
        PRINT_2("\tbase tempo = ", _var.file_attributes[_var.melodyId].tempo);
        PRINT_2("\tcurrent native tempo = ", _var.native_tempo);
        PRINT_2("new playback tempo = ", new_tempo);
    }
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
    _var.user_defined_tempo  = _var.file_attributes[_var.melodyId].tempo;
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
    _var.user_defined_tempo  = _var.file_attributes[_var.melodyId].tempo;
    _var.native_tempo  = _var.file_attributes[_var.melodyId].tempo;
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
    _var.user_defined_tempo  = _var.file_attributes[_var.melodyId].tempo;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns a string with the current melody name
const char *    TesAutoDrums::getCurrentMelodyName(void){
    if(!_var.is_initialized){
        return NULL;
    }
    // a returned name shoould be:
    //  * wihout a prefix (like "/LOOP/")
    //  * without the extension ".mid"
    // NOTE: the returned value will live until this function is called for another melody
    int start_pos = (_var.file_attributes[_var.melodyId].loop)? loopPrefix.length() : singlePrefix.length();
    int end_pos = _var.file_names[_var.melodyId].length() - 4;  // without ".mid"
    static  char    buffer[MAX_FILE_NAME_LENGTH];
    strcpy(buffer, _var.file_names[_var.melodyId].substring(start_pos, end_pos).c_str());

    return buffer;
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
            _var.is_playing = false; // TODO revise this. Do we need to implement "auto stop"?
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////         Private Section             //////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
// receives MIDI events during MIDI files scanning
void TesAutoDrums::midiEventScanner(midi_event *pev){
    // check if the MIDI channel is correct
    if(pev->channel != drumsChannel){
        // the file cannot send MIDI commands to non-drums channels
        _var.validation_data.is_valid = false;
    }

    // count MIDI commands
    _var.validation_data.number_of_commands++;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives META events during MIDI files scanning
void TesAutoDrums::metaEventScanner(const meta_event *pmev){
    // we analyse just a subset of parameters
    switch(pmev->type){
    case 0x2F:      // end of track
        {
            PRINT_2("end of track ", pmev->track);
        }
        break;
    case 0x51:      // set Tempo in "us / tick"
        // Generally, it's set in track #0 and "played" just once in a very beginning.
        // This is the real Tempo (if present in a MIDI file) which overrides the header's value.
        {
            // when we get here, the SMF object has already been adjusted to the new tempo.
            // so, just read new values, if this is track #0
            if (pmev->track == 0){
                _var.validation_data.tempo_set_in_track_0   = true;
                _var.validation_data.tick_time              = _SMF.getTickTime();               // us / tick
                _var.validation_data.tempo                  = _SMF.getTempo();                  // beats / min
                _var.validation_data.ticks_per_quarter_note = _SMF.getTicksPerQuarterNote();    // tick / quarter
                PRINT_2("new tempo = ", _var.validation_data.tempo);
            }
            else {
                _var.validation_data.tempo_set_in_track_n   = true;
                PRINT_2("  tempo change in track1+ :", _SMF.getTempo());
            }
        }
        break;
    case 0x58:      // time signature
        {
            // when we get here, the SMF object has already been adjusted to the new time signature.
            // so, just read new values
            // NOTE: this doesn't change the tempo
            if (pmev->track == 0){
                _var.validation_data.time_signature     = _SMF.getTimeSignature();
                PRINT_2_HEX("new time signature = ", _var.validation_data.time_signature);
            }
            else {
                PRINT_2_HEX("  time signature change in track1+ :", _SMF.getTimeSignature());
            }
        }
        break;
    }
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

    // *** filter out "banned" MIDI commands:
    //  * CC 7  - Volume
    //  * CC 11 - Expression
    switch (cmd.midiCommand){
    case mcControlChange:
        {
            switch(cmd.data1){
            case 7:         // Volume
            case 11:        // Expression
                // just return without sending this command
                return;
            }
        }
        break;
    }
    // *** all other MIDI commands are allowed

    _midi_queue->pushCommand(&cmd);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives META events during MIDI files playing
void TesAutoDrums::metaEventPlayer(const meta_event *pmev){
    // we analyse just a subset of parameters
    switch(pmev->type){
    case 0x51:      // set Tempo in "us / tick"
        // Generally, it's set in track #0 and "played" just once in a very beginning.
        // This is the real Tempo (if present in a MIDI file) which overrides the header's value.
        {
            // when we get here, the SMF object has already been adjusted to the new tempo.

            PRINT_2("META: old native tempo = ", _var.native_tempo);
            // a new native tempo was set to the _SMF already; so, just get it from there
            _var.native_tempo   =   _SMF.getTempo();
            // The new value of the current tempo should be adjusted according to the difference
            // between the user-defined tempo and the "native" MIDI file tempo stored in the MIDI file attributes.
            uint16_t    new_tempo   = map(_var.native_tempo, 0, _var.file_attributes[_var.melodyId].tempo, 0, _var.user_defined_tempo);
            _SMF.setTempo(new_tempo);
            PRINT_2("META: user defined tempo = ", _var.user_defined_tempo);
            PRINT_2("META: new native tempo = ", _var.native_tempo);
            PRINT_2("META: new playback tempo = ", new_tempo);
        }
        break;
    case 0x58:      // time signature
        {
            // when we get here, the SMF object has already been adjusted to the new time signature.
            // NOTE: this doesn't change the tempo.

            // TODO do we need to do anything here? (looks like we don't)
        }
        break;
    }
    
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

    // Obtain initial information from the file header (tempo, etc.)
    _var.validation_data.time_signature         = _SMF.getTimeSignature();          // like "4/4" ("high" / "low")
    _var.validation_data.tick_time              = _SMF.getTickTime();               // us / tick
    _var.validation_data.tempo                  = _SMF.getTempo();                  // beats / min
    _var.validation_data.ticks_per_quarter_note = _SMF.getTicksPerQuarterNote();    // tick / quarter
    PRINT_2("Header tempo = ", _var.validation_data.tempo);
    PRINT_2_HEX("Header time signature = ", _var.validation_data.time_signature);
    // Also, decide if the file should be looped.
    _var.validation_data.loop                   = _var.file_names[_var.melodyId].startsWith(loopPrefix);

    // run the validation
    if(!_SMF.isEOF()){
        _SMF.scanAllTracks();
    }
    _SMF.close();
    // *** We cannot call _SMF.initialise() here, but we need to reset some parameters to default values,
    //     so that validation of the next file starts with default numbers.
    // Set MIDI specified standard defaults
    // - the code below is copy/pasted from the MD_MIDIFile library.
    _SMF.setTicksPerQuarterNote(48); // 48 ticks per quarter note
    _SMF.setTempo(120);              // 120 beats per minute
    _SMF.setTempoAdjust(0);          // 0 beats per minute adjustment
    _SMF.setMicrosecondPerQuarterNote(500000);  // 500,000 microseconds per quarter note
    _SMF.setTimeSignature(4, 4);     // 4/4 time
    // *** _var.validation_data contains the validation result
    // Final checks (if they make sense)
    if(_var.validation_data.is_valid){
        // check if the file was not empty (there're MIDI commands)
        _var.validation_data.is_valid = (_var.validation_data.number_of_commands > 0);
    }

    if (_var.validation_data.is_valid){
        PRINT_1("This file is valid");
        PRINT_2("\tloop = ", _var.validation_data.loop);
        // add MIDI file attributes to the respective list
        MidiFileAttributes  attr;
        attr.loop       = _var.validation_data.loop;
        attr.tempo      = _var.validation_data.tempo;
        _var.file_attributes.push_back(attr);
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
        _var.user_defined_tempo  = _var.file_attributes[_var.melodyId].tempo;    // don't forget about the tempo
        _var.native_tempo  = _var.file_attributes[_var.melodyId].tempo;
    }
}

