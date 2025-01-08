// Declares a function notifying the user about a critical error in the firmware
//
#ifndef _SWER_H
#define _SWER_H

// reserved values for SWERs
// The higher 1 bit is set to '1' in order to make obvious that an error code is displayed on LEDs.
// Otherwise, the user may think that LEDs just indicate active pre-sets (for numbers like 1, 2, 4, etc).
#define swerMidiController01    0x81
#define swerEventQueue01        0x82
#define swerMidiController02    0x83
#define swerKeyboard01          0x84
#define swerKeyboard02          0x85
#define swerLedSet01            0x86
#define swerGUI01               0x87
#define swerMidiController03    0x88
#define swerMidiController04    0x89
#define swerGUI02               0x8A
#define swerMidiController05    0x8B
#define swerGUI03               0x8C
#define swerGUI04               0x8D
#define swerGUI05               0x8E
#define swerMidiController06    0x8F
#define swerAutoDrums01         0x90
#define swerAutoDrums02         0x91
#define swerGUI06               0x92
#define swerGUI07               0x93
#define swerGUI08               0x94
#define swerGUI09               0x95
#define swerGUI10               0x96
#define swerGUI11               0x97
#define swerGUI12               0x98
#define swerGUI13               0x99
#define swerMidiController07    0x9A
#define swerAutoDrums03         0x9B

///////////////////////////////////////////////////////////////////////////////////////////////////
// Shows an error code on LEDs and goes into an infinite loop,
// so that the system stops reacting to anything.
void    SWER(uint8_t   error_code);

#endif

