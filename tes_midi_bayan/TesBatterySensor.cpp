#include "TesBatterySensor.h"

#define BATTERY_EVENT_PERIOD   60000    // once per minute

///////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesBatterySensor::TesBatterySensor(TesEventQueue *queue){
    _event_queue = queue;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// initializes the instance
void TesBatterySensor::init(void){
    filteredValue   = 2048;
    lastSentValue   = 0;    // this will trigger sending the very first event, because the starting value
                            // will not be 0
    // TODO ensure the sensor sends an event right after start.
    _event_timer = millis();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// the tick() method that must be called from loop()
void TesBatterySensor::tick(void){
    timer_t     millis_snapshot = millis();

    // check the timer
    if (millis_snapshot - _event_timer > BATTERY_EVENT_PERIOD){
        // read the sensor and send an event
        uint8_t freshValue = getBatteryValue();

        // check if we need to send an event
        if (freshValue != lastSentValue){
            tesEvent    event;
            event.eventType         = tesEvBattery;
            event.newBatteryValue  = lastSentValue = freshValue;
            _event_queue->pushEvent(&event);
        }
        //reset the timer
        _event_timer = millis();
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////              Private stuff              //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

// configuration parameters for battery sensor
#define batterySensorPin   29

#define BATTERY_FILTER        0.6     // Значение фильтра датчика давления. От 0 до 1. Ближе к 1 - слабая фильтрация. Ближе к 0 - сильная фильтрация.

// TODO adjust these values
#define batteryLowValue     2048
#define batteryTopValue     4095

///////////////////////////////////////////////////////////////////////////////////////////////////
// reads raw data from the sensor and converts it to the 0..127 range
uint8_t TesBatterySensor::getBatteryValue(void){
    int tmpValue;

    // Считать сырое (необработанное) значение датчика давления и вычесть из него среднее значение
    // Значение должно быть положительным.
    //tmpValue = abs(analogRead(pressureSensorPin) - centerValue);
    tmpValue = 4095;    // TODO replace this with actual reading

    // отфильтровать его
    filteredValue = BATTERY_FILTER*((float)(abs(tmpValue))) + ((float)1.0-BATTERY_FILTER)*(float)filteredValue;

    // TODO rework this using Alex Gyver's algorythm of translating voltage measurements into percentage
    return constrain(map(filteredValue, batteryLowValue, batteryTopValue, 0, 100), 0, 100);
}


