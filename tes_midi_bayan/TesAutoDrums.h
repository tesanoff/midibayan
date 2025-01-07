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
    uint8_t             _melodyId;
    bool                _is_playing;
    timer_t             _melody_timer;
    uint8_t             _current_note;
    uint8_t             _tempo;
    SDFAT               _SD;
    MD_MIDIFile         _SMF;
    StringVector        _file_names;
    bool                _is_scanning;

    void    playCurrentNote(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // receives MIDI events during MIDI files scanning
    void midiEventScanner(midi_event *pev);
    // receives META events during MIDI files scanning
    void metaEventScanner(const meta_event *pmev);
    // receives MIDI events during MIDI files playing
    void midiEventPlayer(midi_event *pev);
    // receives META events during MIDI files playing
    void metaEventPlayer(const meta_event *pmev);
};


#endif
