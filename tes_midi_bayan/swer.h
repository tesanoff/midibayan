// Declares a function notifying the user about a critical error in the firmware
//
#ifndef _SWER_H
#define _SWER_H

// reserved values for SWERs
// The higher 4 bits are set to '1' in order to make obvious that an error code is displayed on LEDs.
// Otherwise, the user may think that LEDs just indicate active pre-sets (for numbers like 1, 2, 4, etc).
#define swerMidiController01    0xC1
#define swerEventQueue01        0xC2
#define swerMidiController02    0xC3
#define swerKeyboard01          0xC4
#define swerKeyboard02          0xC5
#define swerLedSet01            0xC6
#define swerGUI01               0xC7
#define swerMidiController03    0xC8
#define swerMidiController04    0xC9
#define swerGUI02               0xCA
#define swerMidiController05    0xCB
#define swerGUI03               0xCC
#define swerGUI04               0xCD
#define swerGUI05               0xCE
#define swerMidiController06    0xCF
#define swerAutoDrums01         0xD0
#define swerAutoDrums02         0xD1
#define swerGUI06               0xD2
#define swerGUI07               0xD3
#define swerGUI08               0xD4
#define swerGUI09               0xD5
#define swerGUI10               0xD6
#define swerGUI11               0xD7
#define swerGUI12               0xD8
#define swerGUI13               0xD9

///////////////////////////////////////////////////////////////////////////////////////////////////
// Shows an error code on LEDs and goes into an infinite loop,
// so that the system stops reacting to anything.
void    SWER(uint8_t   error_code);

#endif

