/*
 *
 * This file contains PROGMEM constants for custom selection of musical instruments
 * available in the ATEMP HW synthesizer.
 *
 */

#ifndef _ATEMP_HW_H
#define _ATEMP_HW_H

// groups of instruments
const char atGroupName0[] PROGMEM   = "Pianos";
const char atGroupName1[] PROGMEM   = "El. Pianos";
const char atGroupName2[] PROGMEM   = "Keyboards";
const char atGroupName3[] PROGMEM   = "Percussion";
const char atGroupName4[] PROGMEM   = "Organs";
const char atGroupName5[] PROGMEM   = "Guitars";
const char atGroupName6[] PROGMEM   = "Basses";
const char atGroupName7[] PROGMEM   = "Strings";
const char atGroupName8[] PROGMEM   = "Orchestral";
const char atGroupName9[] PROGMEM   = "Brass";
const char atGroupName10[] PROGMEM  = "Sax Wood";
const char atGroupName11[] PROGMEM  = "Leads";
const char atGroupName12[] PROGMEM  = "Pads";
const char atGroupName13[] PROGMEM  = "SFX";
const char atGroupName14[] PROGMEM  = "Ethnic";
const char atGroupName15[] PROGMEM  = "Tone Drums";
const char atGroupName16[] PROGMEM  = "Sounds";
//
#define atNumberOfGroups    17
//
const char * const  atGroupName[atNumberOfGroups] PROGMEM = {
    atGroupName0, atGroupName1, atGroupName2, atGroupName3, atGroupName4, atGroupName5, atGroupName6, atGroupName7, 
    atGroupName8, atGroupName9, atGroupName10, atGroupName11, atGroupName12, atGroupName13, atGroupName14, atGroupName15, 
    atGroupName16
};
// Last Program ID for each group
const uint8_t atGroupMap[atNumberOfGroups] PROGMEM = {
    3,           // Фортепиано
    5,          // Электронные пианино
    8,         // Клавишные
    15,         // Перкуссия
    23,         // Органы
    31,         // Гитары
    39,        // Басы
    45,       // Струнные
    55,       // Оркестр
    63,       // Медные духовые
    79,       // Деревянные духовые
    87,       // Leads
    95,       // Pads
    103,       // Спецэффекты
    111,       // Этнические
    119,       // Ударные
    127        // Звуки
};

// All Instrument names
const char atInstrumentNames[] PROGMEM    =
    "Grand Piano\0"
    "Bright Piano\0"
    "(El Grd) Piano\0"
    "Honky-tonk\0"
    "El.Piano 1\0"
    "St.Soft El.P\0"
    "Detuned El.P\0"
    "FM+SA El.P\0"
    "60's El.P\0"
    "Hard Rhodes\0"
    "El.Piano 126\0"
    "El.Piano 2\0"
    "Detuned El.P2\0"
    "Detuned El.P\0"
    "St. FM El.P\0"
    "Hard FM El.P.\0"
    "El.P 126\0"
    "Harpsichord\0"
    "Coupled Harp.\0"
    "Clavi\0"
    "Celesta\0"
    "Glockenspiel\0"
    "Music box\0"
    "Vibraphone\0"
    "Marimba\0"
    "Barafon\0"
    "Xylophone\0"
    "Tubular Bells\0"
    "Church Bells\0"
    "Carillon\0"
    "Santur\0"
    "Santur 2\0"
    "Drawbar Organ\0"
    "Орган 101\0"
    "Detuned Org.\0"
    "Organ 109\0"
    "60's Organ\0"
    "Even Bar\0"
    "Organ Bass\0"
    "Organ 1-126\0"
    "Percuss. Organ\0"
    "Organ 201\0"
    "Detuned Organ 2\0"
    "Detuned Organ 3\0"
    "Organ 2-126\0"
    "Rock Organ\0"
    "Rotary Organ\0"
    "Rotary Organ S\0"
    "Rotary Organ F\0"
    "Church Organ\0"
    "Church Organ 2\0"
    "Reed Organ\0"
    "Accordion (Fr)\0"
    "Accordion (It)\0"
    "Harmonica\0"
    "Accordion Tango\0"
    "Guitar Nylon\0"
    "Ukulele\0"
    "Guitar Nylon 1\0"
    "VeloHarmnix\0"
    "Guitar Ac. Steel\0"
    "Guitar 12-str\0"
    "Guitar 12-str 1\0"
    "Gt. Nylon+Steel\0"
    "Mandoline\0"
    "Guitar Steel 2\0"
    "Guitar Steel 6\0"
    "El.Guitar Jazz\0"
    "Guitar Hawai\0"
    "El.Gt.(clear)\0"
    "El.Gt.(chorus)\0"
    "El.Gt.chorus 2\0"
    "El.Gt.clear 6\0"
    "El.Gt.(muted)\0"
    "El.Gt.(funk)\0"
    "El.Gt.(overd.)\0"
    "El.Gt.overd. 6\0"
    "El.Gt.distors.\0"
    "El.Gt.feedback\0"
    "El.Guitar Harm.\0"
    "El.Gt. feedb. 2\0"
    "Guitar Harm.\0"
    "Acoustic Bass\0"
    "Finger Bass\0"
    "Pick Bass\0"
    "Fretless Bass\0"
    "Slap Bass 1\0"
    "Slap Bass 126\0"
    "Slap Bass 2\0"
    "Synth. Bass 1\0"
    "Synth. Bass 3\0"
    "Bass TB303\0"
    "Bass Techno\0"
    "Reso HS Bass\0"
    "Synth. Bass 6\0"
    "Synth. Bass 2\0"
    "Synth. Bass 201\0"
    "Modular Bass\0"
    "Seq Bass\0"
    "Synth. Bass 4\0"
    "Rubber Bass\0"
    "SH101 Bass 1\0"
    "SH101 Bass 2\0"
    "Smooth Bass\0"
    "Синт. Бас 2-126\0"
    "Violin\0"
    "Viola\0"
    "Cello\0"
    "Contrabass\0"
    "Tremolo Strings\0"
    "Pizzicato Str.\0"
    "Harp\0"
    "Timpani\0"
    "Str. Ensemble 1\0"
    "Orchestra\0"
    "Str. Ensemble 2\0"
    "ST Slow Strings\0"
    "Synth.Str. 1\0"
    "Synth.Str. 3\0"
    "Synth.Str. 2\0"
    "Choir Aahs\0"
    "Voice Ooh\0"
    "Synth. Voice\0"
    "Orchestra Hit\0"
    "Impact Hit\0"
    "Philiy Hit\0"
    "Trumpet\0"
    "Trumpet 2\0"
    "Flugel Horn\0"
    "Trumpet Bright\0"
    "Trumpet 126\0"
    "Trombone\0"
    "Tuba\0"
    "Trumpet (muted)\0"
    "Trump. Muted 2\0"
    "Trump. Muted 6\0"
    "French Horn\0"
    "Brass\0"
    "Brass 2\0"
    "Brass Fall\0"
    "Synth. Brass 1\0"
    "Synth. Brass 3\0"
    "Synth. Brass 2\0"
    "Synth. Brass 4\0"
    "Sax Soprano\0"
    "Sax Soprano 6\0"
    "Sax Alto\0"
    "Hyper Alto\0"
    "Sax Alto 6\0"
    "Sax tenor\0"
    "Breathy tenor\0"
    "Sax Baritone\0"
    "Baritone 6\0"
    "Oboe\0"
    "Eng. Horn\0"
    "Eng. Horn 6\0"
    "Bassoon\0"
    "Bassoon 6\0"
    "Clarinet\0"
    "Bs Clarinet\0"
    "Piccolo\0"
    "Flute\0"
    "Recorder\0"
    "Pan Flute\0"
    "Blown Bottle\0"
    "Shakuhachi\0"
    "Whistle\0"
    "Ocarina\0"
    "Lead 1 (square)\0"
    "Square\0"
    "Sine Wave\0"
    "Ld 2 (sawtooth)\0"
    "Saw\0"
    "Ld 3 (calliope)\0"
    "Lead 4 (chiff)\0"
    "Ld 5 (charang)\0"
    "Lead 6 (voice)\0"
    "Lead 7 (fifths)\0"
    "Ld8 (bass+lead)\0"
    "Pd 1 (fantasia)\0"
    "Pad 2 (warm)\0"
    "Rotary String\0"
    "Pd 3 polysynth\0"
    "Pad 4 (choir)\0"
    "Pad 5 (bowed)\0"
    "Pd 6 (metallic)\0"
    "Pad 7 (halo)\0"
    "Pad 8 (sweep)\0"
    "FX1 (rain)\0"
    "FX2 soundtrack\0"
    "FX3 (crystal)\0"
    "FX4 atmosphere\0"
    "FX5 brightness\0"
    "FX6 (goblins)\0"
    "FX7 (echoes)\0"
    "Echo Pan\0"
    "FX 8 (sci-fi)\0"
    "Sitar\0"
    "Banjo\0"
    "Shamisen\0"
    "Koto\0"
    "Taisho Koto\0"
    "Kalimba\0"
    "Bag pipe\0"
    "Fiddle\0"
    "Shanai\0"
    "Tinkle Bell\0"
    "Agogo\0"
    "Steel Drums\0"
    "Woodblock\0"
    "Castanets\0"
    "Taiko Drum\0"
    "Concert BD\0"
    "Melodic Tom\0"
    "Melo Tom 2\0"
    "Synth Drum\0"
    "808 Tom\0"
    "Elec Perc 1\0"
    "Reverse Cymbal\0"
    "Gt. Fret Noise\0"
    "Gt. Cut Noise\0"
    "String Slap\0"
    "Bass Slide\0"
    "Pick Strape\0"
    "Breath Noise\0"
    "Fl. Key Click\0"
    "Seashore\0"
    "Rain\0"
    "Thunder\0"
    "Wind\0"
    "Stream\0"
    "Bubble\0"
    "Bird Tweet\0"
    "Собака\0"
    "Horse Hallop\0"
    "Bird 2\0"
    "Phone\0"
    "Phone 2\0"
    "Door Creaking\0"
    "Door Closing\0"
    "Scratch\0"
    "Wind Chime\0"
    "Helicopter\0"
    "Engine Start\0"
    "Car Brakes\0"
    "Car Pass\0"
    "Car Crash\0"
    "Police Siren\0"
    "Train\0"
    "Jet Takeoff\0"
    "Starship\0"
    "Burst Noise\0"
    "Applause\0"
    "Laugh\0"
    "Scream\0"
    "Kick\0"
    "Heart Beat\0"
    "Steps\0"
    "Gun Shot\0"
    "Machine Gun\0"
    "Lasergun\0"
    "Explosion\0";
//
#define atNumberOfInstruments   262
//

#endif
