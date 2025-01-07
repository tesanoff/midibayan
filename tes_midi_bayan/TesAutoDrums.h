#ifndef TES_AUTO_DRUMS_H
#define TES_AUTO_DRUMS_H

#include <Arduino.h>
#include "common.h"
#include <MD_MIDIFile.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////

#define numberOfMelodies    6   // the total number of imlemented melodies/rythms

///////////////////////////////////////////////////////////////////////////////////////////////////

typedef std::vector<String>   StringVector;

class TesMIDIOutQueue;

struct  MidiFileValidationData {
    uint8_t     tempo;                      // the tempo set in the file
    uint16_t    number_of_commands;         // how many MIDI commands are in the file
    struct {
        bool    loop: 1;                    // should it be looped or not
        bool    is_valid: 1;
    };

    void    clear(void){
        memset(this, 0, sizeof(MidiFileValidationData));
        is_valid = true; // by default it's valid. If we find an inconsistency - we just reset it.
    }
};

struct  AutoDrumsRuntimeData {
    struct {
        bool                is_playing: 1;
        bool                is_initialized: 1;
        bool                init_in_progress: 1;
    };
    StringVector            file_names;
    uint8_t                 melodyId;
    uint8_t                 tempo;
    timer_t                 init_timer;
    MidiFileValidationData  validation_data;

    AutoDrumsRuntimeData(void){
        is_playing          = false;
        is_initialized      = false;
        init_in_progress    = false;

        melodyId        = 0;
        init_timer      = 0;
    }
};

class TesAutoDrums {
    friend void midi_handler_scanner(midi_event *pev);
    friend void meta_handler_scanner(const meta_event *p);
    friend void midi_handler_player(midi_event *pev);
    friend void meta_handler_player(const meta_event *p);
public:
    TesAutoDrums(TesMIDIOutQueue * midi_queue);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Initializes the engine
    void    init(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Starts playing the current melody
    void    start(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Stops playing the current melody
    void    stop(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // gets the current tempo value
    uint8_t getTempo(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sets the current tempo value
    void    setTempo(uint8_t    tempo);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Returns an ID of the current melody
    uint8_t getCurrentMelodyId(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sets the specified melody as the current
    void setMelodyId(uint8_t melody_id);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sets the next melody as the current
    void setNextMelodyId(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sets the previous melody as the current
    void setPreviousMelodyId(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Returns a string with the current melody name
    const char *    getCurrentMelodyName(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Returns true is a melody is playing
    bool    isPlaying(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the tick() method
    void    tick(void);

private:
    TesMIDIOutQueue     * _midi_queue;
    AutoDrumsRuntimeData _var;
    SDFAT               _SD;
    MD_MIDIFile         _SMF;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // receives MIDI events during MIDI files scanning
    void midiEventScanner(midi_event *pev);
    // receives META events during MIDI files scanning
    void metaEventScanner(const meta_event *pmev);
    // receives MIDI events during MIDI files playing
    void midiEventPlayer(midi_event *pev);
    // receives META events during MIDI files playing
    void metaEventPlayer(const meta_event *pmev);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Validates the current MIDI file
    void    validateMidiFile(void);
};


#endif
