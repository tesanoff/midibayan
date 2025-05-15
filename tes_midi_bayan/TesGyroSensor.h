#pragma once
/*
 * This module implements a Gyro sensor.
 * The HW is the MPU-6050 module.
 *
 *  This class is a source of events related to the gyro sensor.
 *  It reads the current angles, checks if they differ from the previous ones,
 *  and if they do - sends an event with the new values to the event queue.
 *
 */
#include <MPU6050_6Axis_MotionApps20.h>
#include "TesEventQueue.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// auxiliary type
struct TesMpu6050Profile{
    int16_t offsetAX;
    int16_t offsetAY;
    int16_t offsetAZ;
    int16_t offsetGX;
    int16_t offsetGY;
    int16_t offsetGZ;
};

class   TesGyroSensor {
public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // constructor
    TesGyroSensor(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Initializes the instance
    void init(void);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // the tick() method that must be called from loop()
    void tick(void);

    ///////////////////////////////////////////////////////////////////////////
    // Event handler
    // It should be called from loop() after all tick() functions.
    void processEvent(tesEvent *event);

private:
    timer_t         _event_timer;           // we send events by a timer
    MPU6050         _mpu;
    struct {
        uint8_t     swapXY: 1;              // true if we need to swap X and Y axis
        uint8_t     reverseX: 1;            // true if we need to reverse X angle values (negative to positive and vice versa)
        uint8_t     reverseY: 1;            // true if we need to reverse Y angle values
    };
    uint8_t         sentValueX;     // the last values that were sent
    uint8_t         sentValueY;     //
    uint8_t         fifoBuffer[45];         // buffer used for calculation. It must be available all the time while the sensor is used

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Reads data from the sensor and updates the specified variables
    // Returns:
    //  - true = success
    //  - false = No valid data was available
    //
    bool    getValues(uint8_t * newValueX, uint8_t * newValueY);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Runs the calibration procedure
    void    runCalibration(void);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Reads calibration data from EEPROM
    void    restoreCalibrationProfile(void);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Writes calibration data to EEPROM
    void    writeCalibrationProfile(void);
};

// There's the only Gyro sensor
extern  TesGyroSensor       gyroSensor;

