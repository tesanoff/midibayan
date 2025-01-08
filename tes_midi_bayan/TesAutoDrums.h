#ifndef TES_AUTO_DRUMS_H
#define TES_AUTO_DRUMS_H

#include <Arduino.h>
#include "common.h"
#include <MD_MIDIFile.h>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////////////////////

#define numberOfMelodies    6   // the total number of imlemented melodies/rythms

///////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Auxiliary types

struct  MidiFileValidationData {
    uint16_t    number_of_commands;         // how many MIDI commands are in the file
    uint16_t    time_signature;             // "high" / "low"
    uint32_t    tick_time;                  // in us /tick
    uint16_t    tempo;                      // beats / min
    uint16_t    ticks_per_quarter_note;     // tick / quarter <-- this is actually read from a file header
    struct {
        bool    loop: 1;                    // should it be looped or not
        bool    is_valid: 1;
        bool    tempo_set_in_track_0: 1;
        bool    tempo_set_in_track_n: 1;
    };

    void    clear(void){
        memset(this, 0, sizeof(MidiFileValidationData));
        is_valid = true; // by default it's valid. If we find an inconsistency - we just reset it.
    }
};

struct MidiFileAttributes{
    uint16_t    tempo;
    bool        loop;
};
typedef std::vector<MidiFileAttributes>   AttributeVector;

typedef std::vector<String>   StringVector;

struct  AutoDrumsRuntimeData {
    struct {
        bool                is_playing: 1;
        bool                is_initialized: 1;
        bool                init_in_progress: 1;
    };
    StringVector            file_names;
    AttributeVector         file_attributes;
    uint8_t                 melodyId;
    uint16_t                user_defined_tempo;
    uint16_t                native_tempo;   // this is the current tempo from the "MIDI file's" point of view
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

////////////////////////////////////////////////////////////////////////////////////////////////////
/// TesAutoDrums
class TesMIDIOutQueue;

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
    // gets the current user-defined tempo value
    uint16_t getTempo(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Sets the current tempo value
    void    setTempo(uint16_t    tempo);

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
