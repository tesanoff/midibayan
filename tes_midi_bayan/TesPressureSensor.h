/*
 *  This class is a source of events related to the pressure sensor.
 *  It reads a value from the sensor, checks if it differs from the previous one,
 *  and if it does - sends an event with the new value to the event queue.
 *
 *  Range of values: 0..127 (just what we need for MIDI)
 *
 */

#ifndef TES_PRESSURE_SENSOR
#define TES_PRESSURE_SENSOR

#include "TesEventQueue.h"


class TesPressureSensor {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // constructor
    TesPressureSensor(TesEventQueue * queue);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Initializes the instance
    void init(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the tick() method that must be called from loop()
    void tick(void);


private:
    int             filteredValue;          // keeps this value to enable the RA data filter
    uint8_t         lastSentValue;          // the last value sent to the event queue
    TesEventQueue   * _event_queue;
    uint16_t        _event_timer;           // we send events by a timer

    /////////////////////////////////////////////////////////////////////////////////////////////
    // reads raw data from the sensor and converts it to the 0..127 range
    uint8_t getPressureValue(void);
};


#endif

