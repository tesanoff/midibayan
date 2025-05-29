
#include "TesAutoDrums.h"
#include "midi_note_names.h"
#include "TesMidiController.h"
#include <bits/stdc++.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// Settings

#define SD_CARD_CS  26   // for the real version
//#define SD_CARD_CS  9   // for the breadboard debug version

#define MAX_FILE_NAME_LENGTH    120

#define defaultDrumset  0

#define INIT_PERIOD     25  // ms

String  loopPrefix("/LOOP/");
String  singlePrefix("/SINGLE/");
String  fillSuffix(".1");
String  fileExtension(".mid");

const   char    drumFillLabel[] =   "FILL-START";

// DANGER! this is a badly implemented "sort of a singletone"
// (for the sake of simplicity)
TesAutoDrums * TesAutoDrums::_self = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesAutoDrums::TesAutoDrums(TesMIDIOutQueue * midi_queue, TesMIDIControllerSettings * mc_settings){
    _midi_queue = midi_queue;
    _mc_settings = mc_settings;
    // DANGER!!! we assume that the only instance is created in the application
    if(_self == nullptr){
        _self   = this;
    }
    else {
        SWER(swerAutoDrums07);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the engine
void    TesAutoDrums::init(void){
    SdSpiConfig sd_config(SD_CARD_CS, SHARED_SPI, SPI_FULL_SPEED, &SPI1);

    if(!_SD.begin(sd_config)){
        PRINT_1("*** SD Card init failed!");
        // we consider the engine not initialized
        _var.init_in_progress   = false;
        _var.is_initialized     = false;
        return;
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
            if(!file.isDirectory()){
                _var.file_names.push_back(buffer);
            }
            else {
                PRINT_2("Skipping directory:", buffer);
            }
            file.close();
        }
    }

    if(_var.file_names.size() > 0){
        // *** The initial list of file names has been composed.
        // Sort the list
        sort(_var.file_names.begin(), _var.file_names.end());
        //     Now, it's time to start validating those files.
        _var.init_in_progress   = true;
        _var.melodyId           = 0;    // we start with this one
        _SMF.setMidiHandler(midiEventScanner);
        _SMF.setMetaHandler(metaEventScanner);
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
    
    _var.playing_drums_fill = false;
    // set the drumset
    TesMIDICommand  cmd;
    cmd.channelId = _mc_settings->global.drumsMidiChannel;
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
    _var.playing_drums_fill = false;
    _var.drums_fill_requested = false;
    // stop playing all notes in the channel (in case if they're playing)
    TesMIDICommand  cmd;
    cmd.channelId = _mc_settings->global.drumsMidiChannel;
    cmd.midiCommand = mcControlChange;
    cmd.data1   = 123;  // controller number; "all notes off"
    _midi_queue->pushCommand(&cmd);
    // stop playing all notes in the additional channel (in case if they're playing)
    cmd.channelId = _mc_settings->global.drumsAdditionalChannel;
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
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns the number of available melodies
uint8_t TesAutoDrums::getNumberOfMelodies(void){
    return  _var.file_names.size();
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
// Returns a string with the specified melody name
const char *    TesAutoDrums::getMelodyName(uint8_t melody_id){
    if(!_var.is_initialized){
        return NULL;
    }
    // a returned name shoould be:
    //  * wihout a prefix (like "/LOOP/")
    //  * without the extension ".mid"
    // NOTE: the returned value will live until this function is called for another melody
    int start_pos = (_var.file_attributes[melody_id].loop)? loopPrefix.length() : singlePrefix.length();
    int end_pos = _var.file_names[melody_id].length() - fileExtension.length();     // without ".mid"
    static  char    buffer[MAX_FILE_NAME_LENGTH];
    strcpy(buffer, _var.file_names[melody_id].substring(start_pos, end_pos).c_str());

    return buffer;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns a string with the current melody name
const char *    TesAutoDrums::getCurrentMelodyName(void){
    return  getMelodyName(_var.melodyId);
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
        // check if we need to start Drums Fill
        if (_var.drums_fill_starting){
            PRINT_1("starting Drums Fill");
            // close the main MIDI file
            _SMF.close();
            // open the Drums Fill melody
            int err = _SMF.load(_var.fill_names[ _var.file_attributes[_var.melodyId].fill_id ].c_str());
            if (err != MD_MIDIFile::E_OK){
                PRINT_2("ERROR = ", err);
                SWER(swerAutoDrums05);
            }
            // always turn "looping" off for Drums Fills
            _SMF.looping( false );
            // set a flag
            _var.playing_drums_fill = true;
            // reset the request (because it's one-off)
            _var.drums_fill_requested   = false;
            // setting the same tempo as for the main file
            _SMF.setTempo(_var.user_defined_tempo);
            // reset the flag
            _var.drums_fill_starting = false;
        }

        // standard playing of the current MIDI file
        if(!_SMF.isEOF()){
            _SMF.getNextEvent();
        }
        else{
            // EOF is reached, and the file was not set for looping.
            if (_var.playing_drums_fill){
                // *** Drums Fill - done
                PRINT_1("drums fill - done");
                // Restarting the main melody
                _var.playing_drums_fill = false;
                _SMF.close();
                // open the melody again
                int err = _SMF.load(_var.file_names[_var.melodyId].c_str());
                if (err != MD_MIDIFile::E_OK){
                    PRINT_2("ERROR = ", err);
                    SWER(swerAutoDrums06);
                }
                // turn "looping" on, if required
                _SMF.looping( _var.file_attributes[_var.melodyId].loop );
                // set the initial tempo again
                _SMF.setTempo(_var.user_defined_tempo);
            }
            else{
                // So, we need to stop playing.
                _var.is_playing = false; // TODO revise this. Do we need to implement "auto stop"?
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// requests playing Drums Fill (if applicable)
void    TesAutoDrums::requestDrumsFill(void){
    if(!_var.is_initialized){
        return;
    }

    if((_var.is_playing)
            && (_var.file_attributes[_var.melodyId].has_fill)
            && (!_var.playing_drums_fill)){
        // set the flag - it will be processed when the label (drumFillLabel) is detected
        _var.drums_fill_requested = true;
        PRINT_1("** drums fill was requested");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////         Private Section             //////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
// receives MIDI events during MIDI files scanning
void TesAutoDrums::midiEventScanner(midi_event *pev){
    // check if the MIDI channel is correct
    if((pev->channel != _self->_mc_settings->global.drumsMidiChannel)
            && (pev->channel != _self->_mc_settings->global.drumsAdditionalChannel)){
        // the file cannot send MIDI commands to non-drums channels
        _self->_var.validation_data.is_valid = false;
    }

    // count MIDI commands
    _self->_var.validation_data.number_of_commands++;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// receives META events during MIDI files scanning
void TesAutoDrums::metaEventScanner(const meta_event *pmev){
    // we analyse just a subset of parameters
    switch(pmev->type){
    case 0x2F:      // end of track
        {
            // nothing to do
        }
        break;
    case 0x51:      // set Tempo in "us / tick"
        // Generally, it's set in track #0 and "played" just once in a very beginning.
        // This is the real Tempo (if present in a MIDI file) which overrides the header's value.
        {
            // when we get here, the SMF object has already been adjusted to the new tempo.
            // so, just read new values, if this is track #0
            if (pmev->track == 0){
                _self->_var.validation_data.tempo_set_in_track_0   = true;
                _self->_var.validation_data.tick_time              = _self->_SMF.getTickTime();               // us / tick
                _self->_var.validation_data.tempo                  = _self->_SMF.getTempo();                  // beats / min
                _self->_var.validation_data.ticks_per_quarter_note = _self->_SMF.getTicksPerQuarterNote();    // tick / quarter
            }
            else {
                _self->_var.validation_data.tempo_set_in_track_n   = true;
            }
        }
        break;
    case 0x58:      // time signature
        {
            // when we get here, the SMF object has already been adjusted to the new time signature.
            // so, just read new values
            // NOTE: this doesn't change the tempo
            if (pmev->track == 0){
                _self->_var.validation_data.time_signature     = _self->_SMF.getTimeSignature();
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
    cmd.channelId = pev->channel;           // it must be correct, because the MIDI file was validated already
    cmd.midiCommand = pev->data[0] >> 4;    // the MD_MIDIFile library uses the 0xV0 format for commands (where "V" is the "value")
    cmd.data1 = pev->data[1];
    cmd.data2 = pev->data[2];   // this makes sense only for 3-bytes commands, but it's safe to always copy it

    // *** filter out "banned" MIDI commands:
    //  * CC 7  - Volume
    //  * CC 11 - Expression
    switch (cmd.midiCommand){
    case mcControlChange:
        {
            if(cmd.channelId == _self->_mc_settings->global.drumsMidiChannel){
                switch(cmd.data1){
                case 7:         // Volume
                case 11:        // Expression
                    // just return without sending this command
                    // This command is not allowed in this channel
                    return;
                }
            }
            else if (cmd.channelId == _self->_mc_settings->global.drumsAdditionalChannel){
                if(cmd.data1 == 7){     // Volume
                    // just return without sending this command
                    // This command is not allowed in this channel
                    return;
                }
            }
            else{
                SWER(swerAutoDrums04);  // we must not get here
            }
        }
        break;
    }
    // *** all other MIDI commands are allowed

    _self->_midi_queue->pushCommand(&cmd);
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

            // a new native tempo was set to the _SMF already; so, just get it from there
            _self->_var.native_tempo   =   _self->_SMF.getTempo();
            // The new value of the current tempo should be adjusted according to the difference
            // between the user-defined tempo and the "native" MIDI file tempo stored in the MIDI file attributes.
            uint16_t    new_tempo   = map(_self->_var.native_tempo, 0, _self->_var.file_attributes[_self->_var.melodyId].tempo,
                    0, _self->_var.user_defined_tempo);
            _self->_SMF.setTempo(new_tempo);
        }
        break;
    case 0x58:      // time signature
        {
            // when we get here, the SMF object has already been adjusted to the new time signature.
            // NOTE: this doesn't change the tempo.

            // TODO do we need to do anything here? (looks like we don't)
        }
        break;
    case 0x01:      // text
        {
            if((pmev->size == strlen(drumFillLabel)) && (0 == strcmp(pmev->chars, drumFillLabel))){
                if ((_self->_var.file_attributes[_self->_var.melodyId].has_fill)
                        && (! _self->_var.playing_drums_fill)
                        && (_self->_var.drums_fill_requested)){
                    // the actual start will be done in tick()
                    PRINT_1("triggering Drums Fill");
                    _self->_var.drums_fill_starting = true;
                }
            }
        }
        break;
    }
    
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Validates the current MIDI file
void    TesAutoDrums::validateMidiFile(void){
    // _var.melodyId points to the file to be validated now
    _var.validation_data.clear();   // clear the validation data
    // open a melody
    int err = _SMF.load(_var.file_names[_var.melodyId].c_str());
    if (err != MD_MIDIFile::E_OK){
        PRINT_2("ERROR = ", err);
        _var.validation_data.is_valid = false;
    }
    else{
        // Obtain initial information from the file header (tempo, etc.)
        _var.validation_data.time_signature         = _SMF.getTimeSignature();          // like "4/4" ("high" / "low")
        _var.validation_data.tick_time              = _SMF.getTickTime();               // us / tick
        _var.validation_data.tempo                  = _SMF.getTempo();                  // beats / min
        _var.validation_data.ticks_per_quarter_note = _SMF.getTicksPerQuarterNote();    // tick / quarter
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
    }

    if (_var.validation_data.is_valid){
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
        // remove it from the list
        _var.file_names.erase(_var.file_names.begin() + _var.melodyId);
        // _var.melodyId points to the next file now
    }

    // check if the list is not empty (we might erase all files because all of them were invalid)
    if(_var.file_names.size() > 0){
        // *** ok, the list of files is not empty

        // check if the next file exists
        if(_var.melodyId == _var.file_names.size()){
            // *** all files were validated

            // arrange the MIDI file names (some of them may be for drum fills)
            arrangeMidiFiles();
            // set flags & variables up
            _var.init_in_progress   = false;
            _var.is_initialized     = true;
            _SMF.setMidiHandler(midiEventPlayer);
            _SMF.setMetaHandler(metaEventPlayer);
            // set the default melodyId
            _var.melodyId           = 0;
            _var.user_defined_tempo  = _var.file_attributes[_var.melodyId].tempo;    // don't forget about the tempo
            _var.native_tempo  = _var.file_attributes[_var.melodyId].tempo;
        }
    }
    else{
        // *** the list of files is empty! All file names were removed from the list during validation.

        // In this case we consider the engine NOT initialized
        _var.init_in_progress   = false;
        _var.is_initialized     = false;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Arranges MIDI file names
void    TesAutoDrums::arrangeMidiFiles(void){
    /*
     * All file names are collected in _var.file_names. Some of those files are probably Drum Fills.
     * We check file names with fillSuffix and move them to _var.fill_names.
     */
    PRINT_1("*** Arranging MIDI files");
    for(int i=0; i < _var.file_names.size(); i++){
        // Only loop files can have Drum Fills
        if (_var.file_attributes[i].loop){
            // check if this is a Drum Fill file
            int suffix_position = _var.file_names[i].lastIndexOf(fillSuffix);
            if(suffix_position == _var.file_names[i].length() - fileExtension.length() - fillSuffix.length()){
                // *** the fill suffix is exactly where it should be. I.e. this is a Drum Fill file.
                // put the file name into the fill list
                _var.fill_names.push_back(_var.file_names[i]);
                // remove the file name from the main list
                _var.file_names.erase(_var.file_names.begin() + i);
                _var.file_attributes.erase(_var.file_attributes.begin() + i);
                // return the index back
                i--;
            }
        }
    }
    // find the main file for each drum fill file
    for(int i=0; i < _var.fill_names.size(); i++){
        String  base_name = _var.fill_names[i].substring(0, _var.fill_names[i].length() - fillSuffix.length() - fileExtension.length());
        // find the respective main file
        for(int j=0; j < _var.file_names.size(); j++){
            if((_var.file_names[j].length() == (_var.fill_names[i].length() - fillSuffix.length())) && (_var.file_names[j].lastIndexOf(base_name) == 0)){
                // register the drum fill
                _var.file_attributes[j].has_fill    = true;
                _var.file_attributes[j].fill_id     = i;
                break;
            }
        }
    }
    PRINT_1("*** MIDI files arranged");
}

