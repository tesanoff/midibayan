/*
 *  This class is a source of events related to the battery.
 *  It reads the battery voltage, checks if it differs from the previous one,
 *  and if it does - sends an event with the new value to the event queue.
 *
 *  Range of values: 0..100 (%)
 *
 */

#ifndef TES_BATTERY_SENSOR
#define TES_BATTERY_SENSOR

#include "TesEventQueue.h"
#include "common.h"


class TesBatterySensor {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // constructor
    TesBatterySensor(TesEventQueue * queue);

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
    timer_t         _event_timer;           // we send events by a timer

    /////////////////////////////////////////////////////////////////////////////////////////////
    // reads raw data from the sensor and converts it to the 0..100 range
    uint8_t getBatteryValue(void);
};


#endif

