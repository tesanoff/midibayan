/*
 * This is an auxiliary file.
 * It helps to ensure we're not trying to use more EEPROM than we have.
 */

#pragma once
#include "TesMidiController.h"
#include "TesGyroSensor.h"

#define GYRO_CALIBRATION_SIGNATURE          0x2015
#define GYRO_CALIBRATION_SIGNATURE_ADDRESS  (2048 - 2 - 2)
#define GYRO_CALIBRATION_DATA_ADDRESS       (GYRO_CALIBRATION_SIGNATURE_ADDRESS - sizeof(TesMpu6050Profile))

///////////////////////////////////////////////////////////////////////////////////////////////////
//  EEPROM-specific constants and variables for TesMidiController

#define signatureAddress    (2048 - 2)
#define eepromBaseAddress   0

///////////////////////////////////////////////////////////////////////////////////////////////////
/// This is where we're assessing usage of EEPROM

// EEPROM usage:
// ------------------- top ------------------
// * WORD                                           - signature
// * WORD                                           - calibration signature
// * sizeof(TesMpu6050Profile)                      - calibration data
//      ...
// * sizeof(TesMIDIControllerSettings::TesMIDIControllerPreset) * NUMBER_OF_PRESET_PAGES * NUMBER_OF_PRESETS    - preset data
// * sizeof(TesMIDIControllerSettings::TesMIDIControllerGlobalSettings)                                         - global settings
// ------------------- bottom ------------------

static_assert(
        (sizeof(TesMIDIControllerSettings::TesMIDIControllerGlobalSettings)
        + sizeof(TesMIDIControllerSettings::TesMIDIControllerPreset) * NUMBER_OF_PRESET_PAGES * NUMBER_OF_PRESETS
        + sizeof(TesMpu6050Profile)
        + 2
        + 2) <= 2048, "EEPROM is overloaded; we cannot write that much data" );

