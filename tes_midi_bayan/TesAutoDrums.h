#ifndef TES_AUTO_DRUMS_H
#define TES_AUTO_DRUMS_H

#include <Arduino.h>

///////////////////////////////////////////////////////////////////////////////////////////////////

#define numberOfMelodies    5   // the total number of imlemented melodies/rythms

///////////////////////////////////////////////////////////////////////////////////////////////////

class TesMIDIOutQueue;

class TesAutoDrums {
public:
    TesAutoDrums(TesMIDIOutQueue * midi_queue);

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
    // Returns a PROGMEM string with the current melody name
    PGM_P   getCurrentMelodyName(void);

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
    uint16_t            _melody_timer;
    uint8_t             _current_note;
    uint8_t             _tempo;

    void    playCurrentNote(void);
};


#endif
