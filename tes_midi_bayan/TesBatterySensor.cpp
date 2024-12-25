#include "TesBatterySensor.h"

#define BATTERY_EVENT_PERIOD   30000    // twice per minute

// configuration parameters for battery sensor
#define batterySensorPin   29

#define BATTERY_FILTER        0.6       // Значение фильтра датчика батареи. От 0 до 1. Ближе к 1 - слабая фильтрация. Ближе к 0 - сильная фильтрация.

#define batteryLowValue     737         // 3.0 V
#define batteryTopValue     1020        // 4.13 V


///////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
TesBatterySensor::TesBatterySensor(TesEventQueue *queue){
    _event_queue = queue;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// initializes the instance
void TesBatterySensor::init(void){
    filteredValue = analogRead(batterySensorPin);
    lastSentValue   = 0;    // this will trigger sending the very first event, because the starting value
                            // will not be 0

    _event_timer = 0xFFFF8ACF;  // 30000 earlier than "0" (i.e. millis() right after the start)
                                // This should cause the very 1st message to be sent right after the start
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

///////////////////////////////////////////////////////////////////////////////////////////////////
// reads raw data from the sensor and converts it to the 0..127 range
uint8_t TesBatterySensor::getBatteryValue(void){
    int tmpValue;

    // Считать сырое (необработанное) значение датчика давления и вычесть из него среднее значение
    // Значение должно быть положительным.
    tmpValue = analogRead(batterySensorPin);

    // отфильтровать его
    filteredValue = BATTERY_FILTER*((float)(abs(tmpValue))) + ((float)1.0-BATTERY_FILTER)*(float)filteredValue;

/*
 * // отображение заряда в процентах по ёмкости! Интерполировано
 * // вручную по графику разряда ЛИТИЕВОГО аккумулятора
 *int volts = analogRead(0) * 5 * (float)0.977;    // несовсем корректно, так как 5 вольт ровно не бывает. Смотри предыдущий пример
 *int capacity;
 *if (volts > 3870)
 *capacity = map(volts, 4200, 3870, 100, 77);
 *else if ((volts <= 3870) && (volts > 3750) )
 *capacity = map(volts, 3870, 3750, 77, 54);
 *else if ((volts <= 3750) && (volts > 3680) )
 *capacity = map(volts, 3750, 3680, 54, 31);
 *else if ((volts <= 3680) && (volts > 3400) )
 *capacity = map(volts, 3680, 3400, 31, 8);
 *else if (volts <= 3400)
 *capacity = map(volts, 3400, 2600, 8, 0);
 */

    // TODO rework this using Alex Gyver's algorythm of translating voltage measurements into percentage (above)
    return constrain(map(filteredValue, batteryLowValue, batteryTopValue, 0, 100), 0, 100);
}


