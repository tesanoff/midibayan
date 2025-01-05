
#include "TesAutoDrums.h"
#include "midi_note_names.h"
#include "TesMidiController.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// callback functions for MD_MIDIFIle
///

// MIDI handler for scanning
void midi_handler_scanner(midi_event *pev){
    PRINT_1("============= MIDI event (scanner) ===============");
    PRINT_2_HEX("track   =\t", pev->track);
    PRINT_2_HEX("channel =\t", pev->channel);
    PRINT_2_HEX("size    =\t", pev->size);
    for (int i=0;i<pev->size;i++){
        PRINT_2_HEX("data    =\t", pev->data[i]);
    }
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.midiEventScanner(pev);
}

// META handler for scanning
void meta_handler_scanner(const meta_event *p){
    PRINT_1("============= META event (scanner) ===============");
    PRINT_2_HEX("track   =\t", p->track);
    PRINT_2_HEX("type    =\t", p->type);
    PRINT_2_HEX("size    =\t", p->size);
    for (int i=0;i<p->size;i++){
        PRINT_2_HEX("data    =\t", p->data[i]);
    }
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.metaEventScanner(p);
}

// MIDI handler for playing
void midi_handler_player(midi_event *pev){
    PRINT_1("============= MIDI event (player) ===============");
    PRINT_2_HEX("track   =\t", pev->track);
    PRINT_2_HEX("channel =\t", pev->channel);
    PRINT_2_HEX("size    =\t", pev->size);
    for (int i=0;i<pev->size;i++){
        PRINT_2_HEX("data    =\t", pev->data[i]);
    }
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.midiEventPlayer(pev);
}

// META handler for playing
void meta_handler_player(const meta_event *p){
    PRINT_1("============= META event (player) ===============");
    PRINT_2_HEX("track   =\t", p->track);
    PRINT_2_HEX("type    =\t", p->type);
    PRINT_2_HEX("size    =\t", p->size);
    for (int i=0;i<p->size;i++){
        PRINT_2_HEX("data    =\t", p->data[i]);
    }
    // pass the event to the AutoDrums class for processing
    theMIDIController._auto_drums.metaEventPlayer(p);
}
///
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Settings

//#define SD_CARD_CS  26   // for the real version
#define SD_CARD_CS  9   // for the breadboard debug version

#define drumsChannel    9
#define defaultVolume   70

///////////////////////////////////////////////////////////////////////////////////////////////////
// *** melodies
// byte 0 = tempo (hit per minute)
// byte 1 = drumset number (0 - default)
// byte 2+ = the melody
//
// Melody is coded by 2 or 3 bytes:
// 1) note command:     bits 0..6: MIDI note Id; bit 7: 0 = "note on", 1 = "note off"
// 1a) velocity (if the note command is "note on"
// 2) pause before processing the next command; its represented either as 0 (no pause) or as a divisor
//    from 1/1 1/2 1/4 1/8 1/16 etc.  I.e. a 1/4 pause is represented as "4".
//
// Zero note means the end of melody (the same meaning as in traditional C-strings).
//

// these macros should be used for defining melodies
#define off(note)   (0x80|note) // stop playing the note
#define H(note)   (note), 127   // play with High velocity (default)
#define M(note)   (note), 100   // play with Medium velocity
#define L(note)   (note), 70    // play with Low velocity

#define drumsetIndex    1
#define firstNoteIndex  2

// Metronome 4/4:
//                                         |~~~~~~~~~~~~~~|          |~~~~~~~~~~~~~|           |~~~~~~~~~~~~~|           |~~~~~~~~~~~~|
const uint8_t melody0[]         = {60, 0, L(Bb1), 8, off(Bb1), 8, L(A1), 8, off(A1), 8, L(A1), 8, off(A1), 8, L(A1), 8, off(A1), 8, 0 };
// Metronome 3/4:
const uint8_t melody1[]         = {120, 0, H(Db2), 8, off(Db2), 8, L(Db2), 8, off(Db2), 8, L(Db2), 8, off(Db2), 8,  0 };
// bit #1
const uint8_t melody2[]         = {120, 0, H(C2),  0, H(Ab2), 8, off(Ab2), 0, off(C2),  0, H(Ab2), 8, off(Ab2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Ab2), 0, off(Db2), 0, H(Ab2), 16, H(C2), 16, off(Ab2), 0, off(C2), 0,
                                           H(C2),  0, H(Ab2), 8, off(Ab2), 0, off(C2),  0, H(Bb2), 8, off(Bb2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Ab2), 0, off(Db2), 0, H(Ab2), 8, off(Ab2), 0,
                                           0};
// Alley Cat
const uint8_t melody3[]         = {70, 0, H(C2), 0, H(Gb2), 16, off(Gb2), 16, off(C2), 0, H(Gb2), 16, off(Gb2), 32, H(Gb2), 32, off(Gb2), 0,
                                          H(Db2), 0, H(Gb2), 16, off(Gb2), 16, off(Db2), 0, H(Gb2), 16, off(Gb2), 32, H(Gb2), 32, off(Gb2), 0,  0};
// The Cat (Jimmy Smith)
const uint8_t melody4[]         = {180, 0, H(C2),  0, H(Ab2), 8, off(C2),  0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Db2), 0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 24, H(Db2), 12, off(Ab2), 0, off(Db2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                     
                                           H(C2),  0, H(Ab2), 8, off(C2),  0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Db2), 0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 24, H(Db2), 12, off(Ab2), 0, off(Db2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                     
                                           H(C2),  0, H(Ab2), 8, off(C2),  0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Db2), 0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 24, H(Db2), 12, off(Ab2), 0,
                                           H(Ab2), 24, off(Db2), 12, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                     
                                           H(Db2), 0, H(Ab2), 8, off(Db2), 0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                           H(Ab2), 8, off(Ab2), 0, H(Ab2), 24, H(Db2), 12, off(Ab2), 0,
                                           H(Ab2), 24, off(Db2), 12, off(Ab2), 0, H(Bb2), 8, off(Bb2), 0,
                                           H(Db2), 0, H(Ab2), 8, off(Db2), 0, off(Ab2), 0, H(Ab2), 8, off(Ab2), 0,
                                     
                                           0};
// Lusta Dick
const uint8_t melody5[]         = {90, 0, H(Bb4), 0, H(Eb5), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, off(Eb5), 0, M(Bb4), 0, H(Eb5), 8, off(Bb4), 0,
                                          H(Bb4), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, off(Eb5), 0, M(Bb4), 0, H(Eb5), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0,
                                          H(Bb4), 8, off(Bb4), 0, off(Eb5), 0, M(Bb4), 0, H(Eb5), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, off(Eb5), 0, 
                                          H(Bb4), 0, H(Eb5), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, off(Eb5), 0, M(Bb4), 0, H(Eb5), 8, off(Bb4), 0, M(Bb4), 8, off(Bb4), 0, off(Eb5), 0,
                                          0};

const uint8_t * const melody[numberOfMelodies] = {
    melody0, melody1, melody2, melody3, melody4, melody5
};

// Names of melodies
const char * const  melodyName[numberOfMelodies] = {
    "Метроном 4/4",
    "Метроном 3/4",
    "Бит #1",
    "Alley Cat",
    "The Cat",
    "Lusta Dick"
};


///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesAutoDrums::TesAutoDrums(TesMIDIOutQueue * midi_queue){
    _midi_queue = midi_queue;
    _is_playing = false;
    // setup the default melody
    _melodyId = 0;      // the default melody ID
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // get the default tempo
    _tempo = melody_p[0];
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
    // default handlers
    _SMF.setMidiHandler(midi_handler_player);   // TODO replace these pointers wit NULL after debugging
    _SMF.setMetaHandler(meta_handler_player);   //
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Starts playing the current melody
void    TesAutoDrums::start(void){
    _is_playing = true;
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // set the drumset
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcProgramChange;
    cmd.data1   = melody_p[drumsetIndex];
    _midi_queue->pushCommand(&cmd);
    // set the current note (i.e. MIDI command)
    _current_note = firstNoteIndex;
    // play the current note
    playCurrentNote();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Stops playing the current melody
void    TesAutoDrums::stop(void){
    _is_playing = false;
    // stop playing all notes in the channel (in case if they're playing)
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcControlChange;
    cmd.data1   = 123;  // controller number; "all notes off"
    _midi_queue->pushCommand(&cmd);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Gets the current tempo value
uint8_t TesAutoDrums::getTempo(void){
    return _tempo;  // no need to complicate yet
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the current tempo value
void    TesAutoDrums::setTempo(uint8_t    tempo){
    _tempo = tempo; // no need to complicate yet
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns an ID of the current melody
uint8_t TesAutoDrums::getCurrentMelodyId(void){
    return _melodyId;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the specified melody as the current
void TesAutoDrums::setMelodyId(uint8_t melody_id){
    if ( _is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( melody_id >= numberOfMelodies){
        SWER(swerAutoDrums02);
    }

    _melodyId = melody_id;
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // get the default tempo
    _tempo = melody_p[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the next melody as the current
void TesAutoDrums::setNextMelodyId(void){
    if ( _is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( ++_melodyId == numberOfMelodies){
        _melodyId = 0;
    }
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // get the default tempo
    _tempo = melody_p[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Sets the previous melody as the current
void TesAutoDrums::setPreviousMelodyId(void){
    if ( _is_playing ){
        // cannot do that if playing is in progress
        SWER(swerAutoDrums01);
    }

    if ( _melodyId-- == 0){
        _melodyId = numberOfMelodies - 1;
    }
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // get the default tempo
    _tempo = melody_p[0];
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns a string with the current melody name
const char *    TesAutoDrums::getCurrentMelodyName(void){
    return melodyName[_melodyId];
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns true is a melody is playing
bool    TesAutoDrums::isPlaying(void){
    return _is_playing;
}

// aux type
union EncodedNote {
    uint8_t rawNote;
    struct {
        uint8_t noteId: 7;
        uint8_t noteOff: 1;
    };
};

///////////////////////////////////////////////////////////////////////////////////////////////
// the tick() method
void    TesAutoDrums::tick(void){
    // check if we're playing a melody
    if ( ! _is_playing ){
        // we are not playing a melody; nothing to do here
        return;
    }

    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // calculate the period of the whole tact
    uint32_t  tact_period = (long)1000 * 4 * (long)60 / _tempo;
    // get the current note command, so that we chan check whether it is "On" or "Off"
    EncodedNote en;
    en.rawNote = melody_p[_current_note];
    // get the duration of the current note (the divisor from 1/2, 1/4, 1/8, 1/16, etc. )
    // "the duration" is the second byte for "note off" or the third byte for "note on"
    uint8_t noteDuration = melody_p[_current_note + ((en.noteOff)?1:2)];
    // calculate note_period
    uint32_t notePeriod = noteDuration ? tact_period / noteDuration : 0;
    // check the melody timer
    timer_t millis_snapshot = millis();
    if((millis_snapshot - _melody_timer) > notePeriod){
        // it's time for the next note
        // if the current note command is "On" then its size is 3 bytes; otherwise - its size is 2 bytes
        _current_note += (en.noteOff) ? 2 : 3;
        // check if we have reached the end of the melody
        en.rawNote = melody_p[_current_note];
        if (en.rawNote == 0){
            // start from the very 1st note again
            _current_note = firstNoteIndex;
        }
        // and play the new current note
        playCurrentNote();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////         Private Section             //////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
// plays the next note (the index of the next note is in _next_note)
void    TesAutoDrums::playCurrentNote(void){
    // get a pointer to the melody
    uint8_t * melody_p = (uint8_t *)melody[_melodyId];
    // get the current note
    EncodedNote eNote;
    eNote.rawNote = melody_p[_current_note];
    // prepare the MIDI command and put it into the queue
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcNoteOn;
    cmd.data1 = eNote.noteId;
    cmd.data2 = (eNote.noteOff) ? 0 : melody_p[_current_note + 1];
    _midi_queue->pushCommand(&cmd);
    // set the timer
    _melody_timer = millis();   // uint16_t <-- uint32_t
}

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

    // TODO implement this!
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
/*
 *typedef struct
 *{
 *  uint8_t track;    ///< the track this was on
 *  uint8_t channel;  ///< the midi channel
 *  uint8_t size;     ///< the number of data bytes
 *  uint8_t data[4];  ///< the data. Only 'size' bytes are valid
 *} midi_event;
 */
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

    // TODO implement this!
}

