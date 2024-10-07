
#include "TesAutoDrums.h"
#include "midi_note_names.h"
#include "TesMidiController.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Settings

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
const uint8_t melody0[] PROGMEM = {60, 0, H(_Db2), 8, off(_Db2), 8, L(_Db2), 8, off(_Db2), 8, L(_Db2), 8, off(_Db2), 8, L(_Db2), 8, off(_Db2), 8, 0 };
// Metronome 3/4:
const uint8_t melody1[] PROGMEM = {120, 0, H(_Db2), 8, off(_Db2), 8, L(_Db2), 8, off(_Db2), 8, L(_Db2), 8, off(_Db2), 8,  0 };
// bit #1
const uint8_t melody2[] PROGMEM = {120, 0, H(_C2),  0, H(_Ab2), 8, off(_Ab2), 0, off(_C2),  0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Ab2), 0, off(_Db2), 0, H(_Ab2), 16, H(_C2), 16, off(_Ab2), 0, off(_C2), 0,
                                           H(_C2),  0, H(_Ab2), 8, off(_Ab2), 0, off(_C2),  0, H(_Bb2), 8, off(_Bb2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Ab2), 0, off(_Db2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           0};
// Alley Cat
const uint8_t melody3[] PROGMEM = {70, 0, H(_C2), 0, H(_Gb2), 16, off(_Gb2), 16, off(_C2), 0, H(_Gb2), 16, off(_Gb2), 32, H(_Gb2), 32, off(_Gb2), 0,
                                          H(_Db2), 0, H(_Gb2), 16, off(_Gb2), 16, off(_Db2), 0, H(_Gb2), 16, off(_Gb2), 32, H(_Gb2), 32, off(_Gb2), 0,  0};
// The Cat (Jimmy Smith)
const uint8_t melody4[] PROGMEM = {180, 0, H(_C2),  0, H(_Ab2), 8, off(_C2),  0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Db2), 0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 24, H(_Db2), 12, off(_Ab2), 0, off(_Db2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                     
                                           H(_C2),  0, H(_Ab2), 8, off(_C2),  0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Db2), 0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 24, H(_Db2), 12, off(_Ab2), 0, off(_Db2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                     
                                           H(_C2),  0, H(_Ab2), 8, off(_C2),  0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Db2), 0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 24, H(_Db2), 12, off(_Ab2), 0,
                                           H(_Ab2), 24, off(_Db2), 12, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                     
                                           H(_Db2), 0, H(_Ab2), 8, off(_Db2), 0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                           H(_Ab2), 8, off(_Ab2), 0, H(_Ab2), 24, H(_Db2), 12, off(_Ab2), 0,
                                           H(_Ab2), 24, off(_Db2), 12, off(_Ab2), 0, H(_Bb2), 8, off(_Bb2), 0,
                                           H(_Db2), 0, H(_Ab2), 8, off(_Db2), 0, off(_Ab2), 0, H(_Ab2), 8, off(_Ab2), 0,
                                     
                                           0};
const uint8_t * const melody[numberOfMelodies] PROGMEM = {
    melody0, melody1, melody2, melody3, melody4
};

// Names of melodies
const char mName0[] PROGMEM  = "Метроном 4/4";
const char mName1[] PROGMEM  = "Метроном 3/4";
const char mName2[] PROGMEM  = "Бит #1";
const char mName3[] PROGMEM  = "Alley Cat";
const char mName4[] PROGMEM  = "The Cat";
const char * const  melodyName[numberOfMelodies] PROGMEM = {
    mName0,  mName1,  mName2, mName3, mName4 
};


///////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesAutoDrums::TesAutoDrums(TesMIDIOutQueue * midi_queue){
    _midi_queue = midi_queue;
    _is_playing = false;
    // setup the default melody
    _melodyId = 0;      // the default melody ID
    // get a pointer to the melody
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // get the default tempo
    _tempo = pgm_read_byte(melody_p);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Starts playing the current melody
void    TesAutoDrums::start(void){
    _is_playing = true;
    // get a pointer to the melody
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // set the drumset
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcProgramChange;
    cmd.data1   = pgm_read_byte(melody_p + drumsetIndex);
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
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // get the default tempo
    _tempo = pgm_read_byte(melody_p);
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
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // get the default tempo
    _tempo = pgm_read_byte(melody_p);
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
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // get the default tempo
    _tempo = pgm_read_byte(melody_p);
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Returns a PROGMEM string with the current melody name
PGM_P   TesAutoDrums::getCurrentMelodyName(void){
    return (PGM_P)pgm_read_ptr(melodyName + _melodyId);
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
    uint8_t * melody_p = pgm_read_ptr(melody + _melodyId);
    // calculate the period of the whole tact
    uint32_t  tact_period = (long)1000 * 4 * (long)60 / _tempo;
    // get the current note command, so that we chan check whether it is "On" or "Off"
    EncodedNote en;
    en.rawNote = pgm_read_byte(melody_p + _current_note);
    // get the duration of the current note (the divisor from 1/2, 1/4, 1/8, 1/16, etc. )
    // "the duration" is the second byte for "note off" or the third byte for "note on"
    uint8_t noteDuration = pgm_read_byte(melody_p + _current_note + ((en.noteOff)?1:2));
    // calculate note_period
    uint32_t notePeriod = noteDuration ? tact_period / noteDuration : 0;
    // check the melody timer
    uint16_t millis_snapshot = millis();
    if((millis_snapshot - _melody_timer) > notePeriod){
        // it's time for the next note
        // if the current note command is "On" then its size is 3 bytes; otherwise - its size is 2 bytes
        _current_note += (en.noteOff) ? 2 : 3;
        // check if we have reached the end of the melody
        en.rawNote = pgm_read_byte(melody_p + _current_note);
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
    uint8_t * melody_p = pgm_read_ptr(melody +_melodyId);
    // get the current note
    EncodedNote eNote;
    eNote.rawNote = pgm_read_byte(melody_p + _current_note);
    // prepare the MIDI command and put it into the queue
    TesMIDICommand  cmd;
    cmd.channelId = drumsChannel;
    cmd.midiCommand = mcNoteOn;
    cmd.data1 = eNote.noteId;
    cmd.data2 = (eNote.noteOff) ? 0 : pgm_read_byte(melody_p + _current_note + 1);
    _midi_queue->pushCommand(&cmd);
    // set the timer
    _melody_timer = millis();   // uint16_t <-- uint32_t
}

