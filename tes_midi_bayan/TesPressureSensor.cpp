#include "TesPressureSensor.h"

#define PRESSURE_EVENT_PERIOD   25      // 40 events per second

///////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesPressureSensor::TesPressureSensor(TesEventQueue *queue){
    _event_queue = queue;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// initializes the instance
void TesPressureSensor::init(void){
    filteredValue   = 0;
    lastSentValue   = 127;  // this will trigger sending the very first event, because the starting value
                            // will be 0 or around 0
    _event_timer = millis();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// the tick() method that must be called from loop()
void TesPressureSensor::tick(void){
    timer_t     millis_snapshot = millis();

    // check the timer
    if (millis_snapshot - _event_timer > PRESSURE_EVENT_PERIOD){
        // read the sensor and send an event
        uint8_t freshValue = getPressureValue();

        // check if we need to send an event
        if (freshValue != lastSentValue){
            tesEvent    event;
            event.eventType         = tesEvPressure;
            event.newPressureValue  = lastSentValue = freshValue;
            _event_queue->pushEvent(&event);
        }
        //reset the timer
        _event_timer = millis();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////              Private stuff              //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

// configuration parameters for pressure sensor
#define pressureSensorPin   28
// TODO revise the 3 values below
#define centerValue         566     // Pressure raw value when no difference between two inputs
#define pressureThreshold   20      // raw values below the threshold are translated to 0 (to avoid permanent sending MIDI messages for sub-zero values)
#define pressureTopValue    300     // max values for "blow" & "suck" are not the same; let's use the lowest one, so that both can reach this value
                                    // Note: it was 432 before, but I had to push too hard to get the maximum volume, so I decided to make it lower.
#define PRESS_FILTER        0.6     // Значение фильтра датчика давления. От 0 до 1. Ближе к 1 - слабая фильтрация. Ближе к 0 - сильная фильтрация.

///////////////////////////////////////////////////////////////////////////////////////////////////
// reads raw data from the sensor and converts it to the 0..127 range
uint8_t TesPressureSensor::getPressureValue(void){
    int tmpValue;

    // Считать сырое (необработанное) значение датчика давления и вычесть из него среднее значение
    // Значение должно быть положительным.
    tmpValue = abs(analogRead(pressureSensorPin) - centerValue);

    // отфильтровать его
    filteredValue = PRESS_FILTER*((float)(abs(tmpValue))) + ((float)1.0-PRESS_FILTER)*(float)filteredValue;

    return constrain(map(filteredValue, pressureThreshold, pressureTopValue, 0, 127), 0, 127);
}


