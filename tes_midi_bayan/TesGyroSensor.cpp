#include "TesGyroSensor.h"
#include "common.h"
#include "eeprom24.h"
#include "swer.h"
#include "TesLedSet.h"

// There's the only Gyro sensor
TesGyroSensor       gyroSensor;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration

// this must be more than 11 ms
#define TES_GYRO_PERIOD             15      // ~65 events per second
#define TES_GYRO_MAX_ANGLE          15      // 15 degrees
#define TES_GYRO_MIN_ANGLE          0       // 0 degrees
#define TES_GYRO_MAX_OUT_VALUE      127     // the output value that corresponds to the Max Angle
#define TES_GYRO_MIN_OUT_VALUE      0       //    -- "" -- "" -- "" -- "" -- "" --      Min Angle

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

#define GYRO_CALIBRATION_SIGNATURE          0x2015
#define GYRO_CALIBRATION_SIGNATURE_ADDRESS  (2048 - 2 - 2)
#define GYRO_CALIBRATION_DATA_ADDRESS       (GYRO_CALIBRATION_SIGNATURE_ADDRESS - sizeof(TesMpu6050Profile))

///////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
TesGyroSensor::TesGyroSensor(void)
{
    swapXY      = false;
    reverseX    = false;
    reverseY    = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Initializes the instance
void TesGyroSensor::init(void){
    // !!! We assume the i2c bus and EEPROM were initialized already
    // HW init
    _mpu.initialize();
    if(_mpu.dmpInitialize()){
        // *** it returns "1" in case of failure
        // Init failure
        SWER(swerGyro03);
    }
    _mpu.setDMPEnabled(true);

    // check if calibration data is stored in EEPROM
    if(GYRO_CALIBRATION_SIGNATURE != EEPROM.readUint16(GYRO_CALIBRATION_SIGNATURE_ADDRESS)){
        // *** There's no calibration data in EEPROM
        // just do temporary calibration without saving data to EEPROM
        runCalibration();
    }
    else {
        // *** We have calibration data stored in EEPROM
        // read it
        restoreCalibrationProfile();
    }
    
    sentValueX      = 190;  // this will trigger sending the very first event, because the starting value
    sentValueY      = 190;  // will be 0 or around 0

    _event_timer    = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// the tick() method that must be called from loop()
void TesGyroSensor::tick(void){
    timer_t     millis_snapshot = millis();

    // check the timer
    if (millis_snapshot - _event_timer > TES_GYRO_PERIOD){
        uint8_t     newX;
        uint8_t     newY;

        bool    got_data = false;
        if(swapXY){
            got_data    = getValues(&newY, &newX);
        }
        else {
            got_data    = getValues(&newX, &newY);
        }

        if(got_data){
            bool    changedX    =   (newX != sentValueX);
            bool    changedY    =   (newY != sentValueY);
            if ((changedX) || (changedY)){
                // need to send an update event
                tesEvent    event;
                event.eventType     = tesEvGyro;
                event.g.valueX      = newX;
                event.g.valueY      = newY;
                event.g.changedX    = changedX;
                event.g.changedY    = changedY;
                theQueue.pushEvent(&event);
                sentValueX  = newX;
                sentValueY  = newY;
            }
        }
        // reset the timer
        _event_timer    = millis_snapshot;
    }
}

///////////////////////////////////////////////////////////////////////////
// Event handler
// It should be called from loop() after all tick() functions.
void TesGyroSensor::processEvent(tesEvent *event){
    // we handle just one message
    if(event->eventType == tesEvSystemControl){
        if(event->command == scRecalibrateGyroSensor){
            // run the calibration procedure
            runCalibration();
            // store the calibration data
            writeCalibrationProfile();

            // flash the System LED
            ledSet.flashLED(ledSystem);

            // clear the event
            event->eventType = tesEmpty;
        }
    }
}


/***********************************************************************************************
 * Private stuff
 */

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads data from the sensor and updates the specified variables
// Returns:
//  - true = success
//  - false = No valid data was available
//
bool    TesGyroSensor::getValues(uint8_t * newValueX, uint8_t * newValueY){

    if (_mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
        // переменные для расчёта
        Quaternion q;
        VectorFloat gravity;
        float ypr[3];

        // расчёты
        _mpu.dmpGetQuaternion(&q, fifoBuffer);
        _mpu.dmpGetGravity(&gravity, &q);
        _mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

        // Добываем углы X и Y в виде целых чисел
        // (Z не используем)
        int newAngleX = (reverseX) ? -degrees(ypr[2]) : degrees(ypr[2]);
        int newAngleY = (reverseY) ? -degrees(ypr[1]) : degrees(ypr[1]);

        // translate angles into outputput values
        *newValueX = map(constrain(newAngleX, TES_GYRO_MIN_ANGLE, TES_GYRO_MAX_ANGLE),
                TES_GYRO_MIN_ANGLE, TES_GYRO_MAX_ANGLE, TES_GYRO_MIN_OUT_VALUE, TES_GYRO_MAX_OUT_VALUE);
        *newValueY = map(constrain(newAngleY, TES_GYRO_MIN_ANGLE, TES_GYRO_MAX_ANGLE),
                TES_GYRO_MIN_ANGLE, TES_GYRO_MAX_ANGLE, TES_GYRO_MIN_OUT_VALUE, TES_GYRO_MAX_OUT_VALUE);

        return true;
    }
    else {
        PRINT_1("*** Warning!!! No valid Gyro Sensor is available, but we tried to read it!");
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Runs the calibration procedure
void    TesGyroSensor::runCalibration(void){
    PRINT_1("Gyro calibration start...");
    _mpu.CalibrateAccel();
    _mpu.CalibrateGyro();
    PRINT_1("Gyro calibration done.");
    // NOTE! The calibration data hasn't been stored anywhere. It will be lost on power off.
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Reads calibration data from EEPROM
void    TesGyroSensor::restoreCalibrationProfile(void){
    TesMpu6050Profile   calibration_profile;

    PRINT_1("Reading Gyro calibration data from EEPROM...");
    if(sizeof(calibration_profile) != EEPROM.read(GYRO_CALIBRATION_DATA_ADDRESS, &calibration_profile, sizeof(calibration_profile))){
        // cannot read data from EEPROM
        SWER(swerGyro01);
    }
    _mpu.setXAccelOffset(calibration_profile.offsetAX);
    _mpu.setYAccelOffset(calibration_profile.offsetAY);
    _mpu.setZAccelOffset(calibration_profile.offsetAZ);
    _mpu.setXGyroOffset(calibration_profile.offsetGX);
    _mpu.setYGyroOffset(calibration_profile.offsetGY);
    _mpu.setZGyroOffset(calibration_profile.offsetGZ);
    PRINT_1("Gyro calibration data was restored from EEPROM");
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Writes calibration data to EEPROM
void    TesGyroSensor::writeCalibrationProfile(void){
    TesMpu6050Profile   calibration_profile;

    if (!EEPROM.updateUint16(GYRO_CALIBRATION_SIGNATURE_ADDRESS, GYRO_CALIBRATION_SIGNATURE)){
        // write error
        SWER(swerGyro02);
    }
    calibration_profile.offsetAX = _mpu.getXAccelOffset();
    calibration_profile.offsetAY = _mpu.getYAccelOffset();
    calibration_profile.offsetAZ = _mpu.getZAccelOffset();
    calibration_profile.offsetGX = _mpu.getXGyroOffset();
    calibration_profile.offsetGY = _mpu.getYGyroOffset();
    calibration_profile.offsetGZ = _mpu.getZGyroOffset();
    if(sizeof(calibration_profile) != EEPROM.update(GYRO_CALIBRATION_DATA_ADDRESS, &calibration_profile, sizeof(calibration_profile))){
        // write error
        SWER(swerGyro02);
    }
}

