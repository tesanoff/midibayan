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
const char atInstrumentName0[] PROGMEM    = "Grand Piano";                  // 0:0
const char atInstrumentName1[] PROGMEM    = "Bright Piano";                 // 0:1
const char atInstrumentName2[] PROGMEM    = "(El Grd) Piano";               // 0:2
const char atInstrumentName3[] PROGMEM    = "Honky-tonk";               // 0:3
const char atInstrumentName4[] PROGMEM    = "El.Piano 1";                // 0:4
const char atInstrumentName5[] PROGMEM    = "St.Soft El.P";              // 8:4
const char atInstrumentName6[] PROGMEM    = "Detuned El.P";              // 9:4
const char atInstrumentName7[] PROGMEM    = "FM+SA El.P";                // 16:4
const char atInstrumentName8[] PROGMEM    = "60's El.P";                 // 24:4
const char atInstrumentName9[] PROGMEM    = "Hard Rhodes";                  // 25:4
const char atInstrumentName10[] PROGMEM   = "El.Piano 126";             // 126:4
const char atInstrumentName11[] PROGMEM   = "El.Piano 2";               // 0:5
const char atInstrumentName12[] PROGMEM   = "Detuned El.P2";           // 8:5
const char atInstrumentName13[] PROGMEM   = "Detuned El.P";             // 9:5
const char atInstrumentName14[] PROGMEM   = "St. FM El.P";              // 16:5
const char atInstrumentName15[] PROGMEM   = "Hard FM El.P.";             // 24:5
const char atInstrumentName16[] PROGMEM   = "El.P 126";             // 126:5
const char atInstrumentName17[] PROGMEM   = "Harpsichord";                   // 0:6
const char atInstrumentName18[] PROGMEM   = "Coupled Harp.";           // 8:6
const char atInstrumentName19[] PROGMEM   = "Clavi";                         // 0:7
const char atInstrumentName20[] PROGMEM   = "Celesta";                       // 0:8
const char atInstrumentName21[] PROGMEM   = "Glockenspiel";                  // 0:9
const char atInstrumentName22[] PROGMEM   = "Music box";          // 0:10
const char atInstrumentName23[] PROGMEM   = "Vibraphone";                      // 0:11
const char atInstrumentName24[] PROGMEM   = "Marimba";                       // 0:12
const char atInstrumentName25[] PROGMEM   = "Barafon";                       // 16:12
const char atInstrumentName26[] PROGMEM   = "Xylophone";                      // 0:13
const char atInstrumentName27[] PROGMEM   = "Tubular Bells";            // 0:14
const char atInstrumentName28[] PROGMEM   = "Church Bells";            // 8:14
const char atInstrumentName29[] PROGMEM   = "Carillon";                      // 9:14
const char atInstrumentName30[] PROGMEM   = "Santur";                        // 0:15
const char atInstrumentName31[] PROGMEM   = "Santur 2";                      // 1:15
const char atInstrumentName32[] PROGMEM   = "Drawbar Organ";                 // 0:16
const char atInstrumentName33[] PROGMEM   = "Орган 101";                     // 1:16
const char atInstrumentName34[] PROGMEM   = "Detuned Org.";               // 8:16
const char atInstrumentName35[] PROGMEM   = "Organ 109";                     // 9:16
const char atInstrumentName36[] PROGMEM   = "60's Organ";                  // 16:16
const char atInstrumentName37[] PROGMEM   = "Even Bar";                      // 33:16
const char atInstrumentName38[] PROGMEM   = "Organ Bass";                  // 40:16
const char atInstrumentName39[] PROGMEM   = "Organ 1-126";                   // 126:16
const char atInstrumentName40[] PROGMEM   = "Percuss. Organ";              // 0:17
const char atInstrumentName41[] PROGMEM   = "Organ 201";                     // 1:17
const char atInstrumentName42[] PROGMEM   = "Detuned Organ 2";               // 8:17
const char atInstrumentName43[] PROGMEM   = "Detuned Organ 3";               // 9:17
const char atInstrumentName44[] PROGMEM   = "Organ 2-126";                   // 126:17
const char atInstrumentName45[] PROGMEM   = "Rock Organ";                     // 0:18
const char atInstrumentName46[] PROGMEM   = "Rotary Organ";                  // 8:18
const char atInstrumentName47[] PROGMEM   = "Rotary Organ S";             // 16:18
const char atInstrumentName48[] PROGMEM   = "Rotary Organ F";             // 24:18
const char atInstrumentName49[] PROGMEM   = "Church Organ";               // 0:19
const char atInstrumentName50[] PROGMEM   = "Church Organ 2";             // 8:19
const char atInstrumentName51[] PROGMEM   = "Reed Organ";                    // 0:20
const char atInstrumentName52[] PROGMEM   = "Accordion (Fr)";            // 0:21
const char atInstrumentName53[] PROGMEM   = "Accordion (It)";             // 8:21
const char atInstrumentName54[] PROGMEM   = "Harmonica";                     // 0:22
const char atInstrumentName55[] PROGMEM   = "Accordion Tango";               // 0:23
const char atInstrumentName56[] PROGMEM   = "Guitar Nylon";          // 0:24
const char atInstrumentName57[] PROGMEM   = "Ukulele";                       // 8:24
const char atInstrumentName58[] PROGMEM   = "Guitar Nylon 1";               // 16:24
const char atInstrumentName59[] PROGMEM   = "VeloHarmnix";                   // 24:24
const char atInstrumentName60[] PROGMEM   = "Guitar Ac. Steel";           // 0:25
const char atInstrumentName61[] PROGMEM   = "Guitar 12-str";               // 7:25
const char atInstrumentName62[] PROGMEM   = "Guitar 12-str 1";             // 8:25
const char atInstrumentName63[] PROGMEM   = "Gt. Nylon+Steel";           // 9:25
const char atInstrumentName64[] PROGMEM   = "Mandoline";                     // 16:25
const char atInstrumentName65[] PROGMEM   = "Guitar Steel 2";              // 32:25
const char atInstrumentName66[] PROGMEM   = "Guitar Steel 6";            // 126:25
const char atInstrumentName67[] PROGMEM   = "El.Guitar Jazz";               // 0:26
const char atInstrumentName68[] PROGMEM   = "Guitar Hawai";              // 8:26
const char atInstrumentName69[] PROGMEM   = "El.Gt.(clear)";           // 0:27
const char atInstrumentName70[] PROGMEM   = "El.Gt.(chorus)";            // 8:27
const char atInstrumentName71[] PROGMEM   = "El.Gt.chorus 2";          // 9:27
const char atInstrumentName72[] PROGMEM   = "El.Gt.clear 6";         // 126:
const char atInstrumentName73[] PROGMEM   = "El.Gt.(muted)";           // 0:28
const char atInstrumentName74[] PROGMEM   = "El.Gt.(funk)";             // 8:28
const char atInstrumentName75[] PROGMEM   = "El.Gt.(overd.)";          // 0:29
const char atInstrumentName76[] PROGMEM   = "El.Gt.overd. 6";         // 126:29
const char atInstrumentName77[] PROGMEM   = "El.Gt.distors.";         // 0:30
const char atInstrumentName78[] PROGMEM   = "El.Gt.feedback";         // 8:30
const char atInstrumentName79[] PROGMEM   = "El.Guitar Harm.";            // 0:31
const char atInstrumentName80[] PROGMEM   = "El.Gt. feedb. 2";         // 8:31
const char atInstrumentName81[] PROGMEM   = "Guitar Harm.";         // 16:31
const char atInstrumentName82[] PROGMEM   = "Acoustic Bass";              // 0:32
const char atInstrumentName83[] PROGMEM   = "Finger Bass";           // 0:33
const char atInstrumentName84[] PROGMEM   = "Pick Bass";         // 0:34
const char atInstrumentName85[] PROGMEM   = "Fretless Bass";         // 0:35
const char atInstrumentName86[] PROGMEM   = "Slap Bass 1";             // 0:36
const char atInstrumentName87[] PROGMEM   = "Slap Bass 126";         // 126:36
const char atInstrumentName88[] PROGMEM   = "Slap Bass 2";             // 0:37
const char atInstrumentName89[] PROGMEM   = "Synth. Bass 1";                   // 0:38
const char atInstrumentName90[] PROGMEM   = "Synth. Bass 3";                   // 8:38
const char atInstrumentName91[] PROGMEM   = "Bass TB303";                     // 9:38
const char atInstrumentName92[] PROGMEM   = "Bass Techno";                     // 10:38
const char atInstrumentName93[] PROGMEM   = "Reso HS Bass";                  // 16:38
const char atInstrumentName94[] PROGMEM   = "Synth. Bass 6";               // 126:38
const char atInstrumentName95[] PROGMEM   = "Synth. Bass 2";                   // 0:39
const char atInstrumentName96[] PROGMEM   = "Synth. Bass 201";                 // 1:39
const char atInstrumentName97[] PROGMEM   = "Modular Bass";                  // 2:39
const char atInstrumentName98[] PROGMEM   = "Seq Bass";                      // 3:39
const char atInstrumentName99[] PROGMEM   = "Synth. Bass 4";                   // 8:39
const char atInstrumentName100[] PROGMEM  = "Rubber Bass";                  // 16:39
const char atInstrumentName101[] PROGMEM  = "SH101 Bass 1";                 // 17:39
const char atInstrumentName102[] PROGMEM  = "SH101 Bass 2";                 // 18:39
const char atInstrumentName103[] PROGMEM  = "Smooth Bass";                  // 19:39
const char atInstrumentName104[] PROGMEM  = "Синт. Бас 2-126";              // 126:39
const char atInstrumentName105[] PROGMEM  = "Violin";                      // 0:40
const char atInstrumentName106[] PROGMEM  = "Viola";                         // 0:41
const char atInstrumentName107[] PROGMEM  = "Cello";                   // 0:42
const char atInstrumentName108[] PROGMEM  = "Contrabass";                    // 0:43
const char atInstrumentName109[] PROGMEM  = "Tremolo Strings";          // 0:44
const char atInstrumentName110[] PROGMEM  = "Pizzicato Str.";         // 0:45
const char atInstrumentName111[] PROGMEM  = "Harp";                         // 0:46
const char atInstrumentName112[] PROGMEM  = "Timpani";                      // 0:47
const char atInstrumentName113[] PROGMEM  = "Str. Ensemble 1";          // 0:48
const char atInstrumentName114[] PROGMEM  = "Orchestra";                      // 8:48
const char atInstrumentName115[] PROGMEM  = "Str. Ensemble 2";          // 0:49
const char atInstrumentName116[] PROGMEM  = "ST Slow Strings";              // 10:49
const char atInstrumentName117[] PROGMEM  = "Synth.Str. 1";             // 0:50
const char atInstrumentName118[] PROGMEM  = "Synth.Str. 3";             // 8:50
const char atInstrumentName119[] PROGMEM  = "Synth.Str. 2";             // 0:51
const char atInstrumentName120[] PROGMEM  = "Choir Aahs";                  // 0:52
const char atInstrumentName121[] PROGMEM  = "Voice Ooh";                // 0:53
const char atInstrumentName122[] PROGMEM  = "Synth. Voice";                  // 0:54
const char atInstrumentName123[] PROGMEM  = "Orchestra Hit";                  // 0:55
const char atInstrumentName124[] PROGMEM  = "Impact Hit";                   // 8:55
const char atInstrumentName125[] PROGMEM  = "Philiy Hit";                   // 9:55
const char atInstrumentName126[] PROGMEM  = "Trumpet";                        // 0:56
const char atInstrumentName127[] PROGMEM  = "Trumpet 2";                      // 1:56
const char atInstrumentName128[] PROGMEM  = "Flugel Horn";                  // 8:56
const char atInstrumentName129[] PROGMEM  = "Trumpet Bright";                // 24:56
const char atInstrumentName130[] PROGMEM  = "Trumpet 126";                    // 126:56
const char atInstrumentName131[] PROGMEM  = "Trombone";                      // 0:57
const char atInstrumentName132[] PROGMEM  = "Tuba";                         // 0:58
const char atInstrumentName133[] PROGMEM  = "Trumpet (muted)";             // 0:59
const char atInstrumentName134[] PROGMEM  = "Trump. Muted 2";           // 1:59
const char atInstrumentName135[] PROGMEM  = "Trump. Muted 6";         // 126:59
const char atInstrumentName136[] PROGMEM  = "French Horn";              // 0:60
const char atInstrumentName137[] PROGMEM  = "Brass";                // 0:61
const char atInstrumentName138[] PROGMEM  = "Brass 2";                     // 8:61
const char atInstrumentName139[] PROGMEM  = "Brass Fall";                   // 16:61
const char atInstrumentName140[] PROGMEM  = "Synth. Brass 1";               // 0:62
const char atInstrumentName141[] PROGMEM  = "Synth. Brass 3";               // 8:62
const char atInstrumentName142[] PROGMEM  = "Synth. Brass 2";               // 0:63
const char atInstrumentName143[] PROGMEM  = "Synth. Brass 4";               // 8:63
const char atInstrumentName144[] PROGMEM  = "Sax Soprano";             // 0:64
const char atInstrumentName145[] PROGMEM  = "Sax Soprano 6";         // 126:64
const char atInstrumentName146[] PROGMEM  = "Sax Alto";                // 0:65
const char atInstrumentName147[] PROGMEM  = "Hyper Alto";                   // 8:65
const char atInstrumentName148[] PROGMEM  = "Sax Alto 6";            // 126:65
const char atInstrumentName149[] PROGMEM  = "Sax tenor";               // 0:66
const char atInstrumentName150[] PROGMEM  = "Breathy tenor";                // 8:66
const char atInstrumentName151[] PROGMEM  = "Sax Baritone";             // 0:67
const char atInstrumentName152[] PROGMEM  = "Baritone 6";                  // 126:67
const char atInstrumentName153[] PROGMEM  = "Oboe";                        // 0:68
const char atInstrumentName154[] PROGMEM  = "Eng. Horn";               // 0:69
const char atInstrumentName155[] PROGMEM  = "Eng. Horn 6";           // 126:69
const char atInstrumentName156[] PROGMEM  = "Bassoon";                        // 0:70
const char atInstrumentName157[] PROGMEM  = "Bassoon 6";                    // 126:70
const char atInstrumentName158[] PROGMEM  = "Clarinet";                      // 0:71
const char atInstrumentName159[] PROGMEM  = "Bs Clarinet";                  // 8:71
const char atInstrumentName160[] PROGMEM  = "Piccolo";                      // 0:72
const char atInstrumentName161[] PROGMEM  = "Flute";                       // 0:73
const char atInstrumentName162[] PROGMEM  = "Recorder";                  // 0:74
const char atInstrumentName163[] PROGMEM  = "Pan Flute";                   // 0:75
const char atInstrumentName164[] PROGMEM  = "Blown Bottle";                      // 0:76
const char atInstrumentName165[] PROGMEM  = "Shakuhachi";                     // 0:77
const char atInstrumentName166[] PROGMEM  = "Whistle";                        // 0:78
const char atInstrumentName167[] PROGMEM  = "Ocarina";                      // 0:79
const char atInstrumentName168[] PROGMEM  = "Lead 1 (square)";              // 0:80
const char atInstrumentName169[] PROGMEM  = "Square";                       // 1:80
const char atInstrumentName170[] PROGMEM  = "Sine Wave";                    // 8:80
const char atInstrumentName171[] PROGMEM  = "Ld 2 (sawtooth)";            // 0:81
const char atInstrumentName172[] PROGMEM  = "Saw";                          // 1:81
const char atInstrumentName173[] PROGMEM  = "Ld 3 (calliope)";            // 0:82
const char atInstrumentName174[] PROGMEM  = "Lead 4 (chiff)";               // 0:83
const char atInstrumentName175[] PROGMEM  = "Ld 5 (charang)";             // 0:84
const char atInstrumentName176[] PROGMEM  = "Lead 6 (voice)";               // 0:85
const char atInstrumentName177[] PROGMEM  = "Lead 7 (fifths)";              // 0:86
const char atInstrumentName178[] PROGMEM  = "Ld8 (bass+lead)";            // 0:87
const char atInstrumentName179[] PROGMEM  = "Pd 1 (fantasia)";             // 0:88
const char atInstrumentName180[] PROGMEM  = "Pad 2 (warm)";                 // 0:89
const char atInstrumentName181[] PROGMEM  = "Rotary String";                // 3:89
const char atInstrumentName182[] PROGMEM  = "Pd 3 polysynth";            // 0:90
const char atInstrumentName183[] PROGMEM  = "Pad 4 (choir)";                // 0:91
const char atInstrumentName184[] PROGMEM  = "Pad 5 (bowed)";                // 0:92
const char atInstrumentName185[] PROGMEM  = "Pd 6 (metallic)";             // 0:93
const char atInstrumentName186[] PROGMEM  = "Pad 7 (halo)";                 // 0:94
const char atInstrumentName187[] PROGMEM  = "Pad 8 (sweep)";                // 0:95
const char atInstrumentName188[] PROGMEM  = "FX1 (rain)";                  // 0:96
const char atInstrumentName189[] PROGMEM  = "FX2 soundtrack";            // 0:97
const char atInstrumentName190[] PROGMEM  = "FX3 (crystal)";               // 0:98
const char atInstrumentName191[] PROGMEM  = "FX4 atmosphere";            // 0:99
const char atInstrumentName192[] PROGMEM  = "FX5 brightness";            // 0:100
const char atInstrumentName193[] PROGMEM  = "FX6 (goblins)";               // 0:101
const char atInstrumentName194[] PROGMEM  = "FX7 (echoes)";                // 0:102
const char atInstrumentName195[] PROGMEM  = "Echo Pan";                     // 2:102
const char atInstrumentName196[] PROGMEM  = "FX 8 (sci-fi)";                // 0:103
const char atInstrumentName197[] PROGMEM  = "Sitar";                        // 0:104
const char atInstrumentName198[] PROGMEM  = "Banjo";                       // 0:105
const char atInstrumentName199[] PROGMEM  = "Shamisen";                      // 0:106
const char atInstrumentName200[] PROGMEM  = "Koto";                         // 0:107
const char atInstrumentName201[] PROGMEM  = "Taisho Koto";                   // 8:107
const char atInstrumentName202[] PROGMEM  = "Kalimba";                      // 0:108
const char atInstrumentName203[] PROGMEM  = "Bag pipe";                      // 0:109
const char atInstrumentName204[] PROGMEM  = "Fiddle";               // 0:110
const char atInstrumentName205[] PROGMEM  = "Shanai";                       // 0:111
const char atInstrumentName206[] PROGMEM  = "Tinkle Bell";                  // 0:112
const char atInstrumentName207[] PROGMEM  = "Agogo";                        // 0:113
const char atInstrumentName208[] PROGMEM  = "Steel Drums";            // 0:114
const char atInstrumentName209[] PROGMEM  = "Woodblock";                    // 0:115
const char atInstrumentName210[] PROGMEM  = "Castanets";                    // 8:115
const char atInstrumentName211[] PROGMEM  = "Taiko Drum";                   // 0:116
const char atInstrumentName212[] PROGMEM  = "Concert BD";                   // 8:116
const char atInstrumentName213[] PROGMEM  = "Melodic Tom";                  // 0:117
const char atInstrumentName214[] PROGMEM  = "Melo Tom 2";                   // 8:117
const char atInstrumentName215[] PROGMEM  = "Synth Drum";                   // 0:118
const char atInstrumentName216[] PROGMEM  = "808 Tom";                      // 8:118
const char atInstrumentName217[] PROGMEM  = "Elec Perc 1";                  // 9:118
const char atInstrumentName218[] PROGMEM  = "Reverse Cymbal";               // 0:119
const char atInstrumentName219[] PROGMEM  = "Gt. Fret Noise";               // 0:120
const char atInstrumentName220[] PROGMEM  = "Gt. Cut Noise";                // 1:120
const char atInstrumentName221[] PROGMEM  = "String Slap";                  // 2:120
const char atInstrumentName222[] PROGMEM  = "Bass Slide";                   // 5:120
const char atInstrumentName223[] PROGMEM  = "Pick Strape";                  // 6:120
const char atInstrumentName224[] PROGMEM  = "Breath Noise";                 // 0:121
const char atInstrumentName225[] PROGMEM  = "Fl. Key Click";                // 1:121 
const char atInstrumentName226[] PROGMEM  = "Seashore";                     // 0:122
const char atInstrumentName227[] PROGMEM  = "Rain";                        // 1:122
const char atInstrumentName228[] PROGMEM  = "Thunder";                         // 2:122
const char atInstrumentName229[] PROGMEM  = "Wind";                        // 3:122
const char atInstrumentName230[] PROGMEM  = "Stream";                        // 4:122
const char atInstrumentName231[] PROGMEM  = "Bubble";                       // 5:122
const char atInstrumentName232[] PROGMEM  = "Bird Tweet";                   // 0:123
const char atInstrumentName233[] PROGMEM  = "Собака";                       // 1:123
const char atInstrumentName234[] PROGMEM  = "Horse Hallop";              // 2:123
const char atInstrumentName235[] PROGMEM  = "Bird 2";                     // 3:123
const char atInstrumentName236[] PROGMEM  = "Phone";                      // 0:124
const char atInstrumentName237[] PROGMEM  = "Phone 2";                    // 1:124
const char atInstrumentName238[] PROGMEM  = "Door Creaking";                  // 2:124
const char atInstrumentName239[] PROGMEM  = "Door Closing";             // 3:124
const char atInstrumentName240[] PROGMEM  = "Scratch";                     // 4:124
const char atInstrumentName241[] PROGMEM  = "Wind Chime";                   // 5:124
const char atInstrumentName242[] PROGMEM  = "Helicopter";                     // 0:125
const char atInstrumentName243[] PROGMEM  = "Engine Start";              // 1:125
const char atInstrumentName244[] PROGMEM  = "Car Brakes";        // 2:125
const char atInstrumentName245[] PROGMEM  = "Car Pass";                // 3:125
const char atInstrumentName246[] PROGMEM  = "Car Crash";                       // 4:125
const char atInstrumentName247[] PROGMEM  = "Police Siren";           // 5:125
const char atInstrumentName248[] PROGMEM  = "Train";                        // 6:125
const char atInstrumentName249[] PROGMEM  = "Jet Takeoff";            // 7:125
const char atInstrumentName250[] PROGMEM  = "Starship";          // 8:125
const char atInstrumentName251[] PROGMEM  = "Burst Noise";                  // 9:125
const char atInstrumentName252[] PROGMEM  = "Applause";                 // 0:126
const char atInstrumentName253[] PROGMEM  = "Laugh";                         // 1:126
const char atInstrumentName254[] PROGMEM  = "Scream";                         // 2:126
const char atInstrumentName255[] PROGMEM  = "Kick";                         // 3:126
const char atInstrumentName256[] PROGMEM  = "Heart Beat";                // 4:126
const char atInstrumentName257[] PROGMEM  = "Steps";                         // 5:126
const char atInstrumentName258[] PROGMEM  = "Gun Shot";                      // 0:127
const char atInstrumentName259[] PROGMEM  = "Machine Gun";            // 1:127
const char atInstrumentName260[] PROGMEM  = "Lasergun";                      // 2:127
const char atInstrumentName261[] PROGMEM  = "Explosion";                        // 3:127
//
#define atNumberOfInstruments   262
//
const char * const  atInstrumentName[atNumberOfInstruments] PROGMEM = {
    atInstrumentName0, atInstrumentName1, atInstrumentName2, atInstrumentName3, atInstrumentName4, atInstrumentName5, atInstrumentName6, 
    atInstrumentName7, atInstrumentName8, atInstrumentName9, atInstrumentName10, atInstrumentName11, atInstrumentName12, atInstrumentName13, 
    atInstrumentName14, atInstrumentName15, atInstrumentName16, atInstrumentName17, atInstrumentName18, atInstrumentName19, atInstrumentName20, 
    atInstrumentName21, atInstrumentName22, atInstrumentName23, atInstrumentName24, atInstrumentName25, atInstrumentName26, atInstrumentName27, 
    atInstrumentName28, atInstrumentName29, atInstrumentName30, atInstrumentName31, atInstrumentName32, atInstrumentName33, atInstrumentName34, 
    atInstrumentName35, atInstrumentName36, atInstrumentName37, atInstrumentName38, atInstrumentName39, atInstrumentName40, atInstrumentName41, 
    atInstrumentName42, atInstrumentName43, atInstrumentName44, atInstrumentName45, atInstrumentName46, atInstrumentName47, atInstrumentName48, 
    atInstrumentName49, atInstrumentName50, atInstrumentName51, atInstrumentName52, atInstrumentName53, atInstrumentName54, atInstrumentName55, 
    atInstrumentName56, atInstrumentName57, atInstrumentName58, atInstrumentName59, atInstrumentName60, atInstrumentName61, atInstrumentName62, 
    atInstrumentName63, atInstrumentName64, atInstrumentName65, atInstrumentName66, atInstrumentName67, atInstrumentName68, atInstrumentName69, 
    atInstrumentName70, atInstrumentName71, atInstrumentName72, atInstrumentName73, atInstrumentName74, atInstrumentName75, atInstrumentName76, 
    atInstrumentName77, atInstrumentName78, atInstrumentName79, atInstrumentName80, atInstrumentName81, atInstrumentName82, atInstrumentName83, 
    atInstrumentName84, atInstrumentName85, atInstrumentName86, atInstrumentName87, atInstrumentName88, atInstrumentName89, atInstrumentName90, 
    atInstrumentName91, atInstrumentName92, atInstrumentName93, atInstrumentName94, atInstrumentName95, atInstrumentName96, atInstrumentName97, 
    atInstrumentName98, atInstrumentName99, atInstrumentName100, atInstrumentName101, atInstrumentName102, atInstrumentName103, atInstrumentName104, 
    atInstrumentName105, atInstrumentName106, atInstrumentName107, atInstrumentName108, atInstrumentName109, atInstrumentName110, atInstrumentName111, 
    atInstrumentName112, atInstrumentName113, atInstrumentName114, atInstrumentName115, atInstrumentName116, atInstrumentName117, atInstrumentName118, 
    atInstrumentName119, atInstrumentName120, atInstrumentName121, atInstrumentName122, atInstrumentName123, atInstrumentName124, atInstrumentName125, 
    atInstrumentName126, atInstrumentName127, atInstrumentName128, atInstrumentName129, atInstrumentName130, atInstrumentName131, atInstrumentName132, 
    atInstrumentName133, atInstrumentName134, atInstrumentName135, atInstrumentName136, atInstrumentName137, atInstrumentName138, atInstrumentName139, 
    atInstrumentName140, atInstrumentName141, atInstrumentName142, atInstrumentName143, atInstrumentName144, atInstrumentName145, atInstrumentName146, 
    atInstrumentName147, atInstrumentName148, atInstrumentName149, atInstrumentName150, atInstrumentName151, atInstrumentName152, atInstrumentName153, 
    atInstrumentName154, atInstrumentName155, atInstrumentName156, atInstrumentName157, atInstrumentName158, atInstrumentName159, atInstrumentName160, 
    atInstrumentName161, atInstrumentName162, atInstrumentName163, atInstrumentName164, atInstrumentName165, atInstrumentName166, atInstrumentName167, 
    atInstrumentName168, atInstrumentName169, atInstrumentName170, atInstrumentName171, atInstrumentName172, atInstrumentName173, atInstrumentName174, 
    atInstrumentName175, atInstrumentName176, atInstrumentName177, atInstrumentName178, atInstrumentName179, atInstrumentName180, atInstrumentName181, 
    atInstrumentName182, atInstrumentName183, atInstrumentName184, atInstrumentName185, atInstrumentName186, atInstrumentName187, atInstrumentName188, 
    atInstrumentName189, atInstrumentName190, atInstrumentName191, atInstrumentName192, atInstrumentName193, atInstrumentName194, atInstrumentName195, 
    atInstrumentName196, atInstrumentName197, atInstrumentName198, atInstrumentName199, atInstrumentName200, atInstrumentName201, atInstrumentName202, 
    atInstrumentName203, atInstrumentName204, atInstrumentName205, atInstrumentName206, atInstrumentName207, atInstrumentName208, atInstrumentName209, 
    atInstrumentName210, atInstrumentName211, atInstrumentName212, atInstrumentName213, atInstrumentName214, atInstrumentName215, atInstrumentName216, 
    atInstrumentName217, atInstrumentName218, atInstrumentName219, atInstrumentName220, atInstrumentName221, atInstrumentName222, atInstrumentName223, 
    atInstrumentName224, atInstrumentName225, atInstrumentName226, atInstrumentName227, atInstrumentName228, atInstrumentName229, atInstrumentName230, 
    atInstrumentName231, atInstrumentName232, atInstrumentName233, atInstrumentName234, atInstrumentName235, atInstrumentName236, atInstrumentName237, 
    atInstrumentName238, atInstrumentName239, atInstrumentName240, atInstrumentName241, atInstrumentName242, atInstrumentName243, atInstrumentName244, 
    atInstrumentName245, atInstrumentName246, atInstrumentName247, atInstrumentName248, atInstrumentName249, atInstrumentName250, atInstrumentName251, 
    atInstrumentName252, atInstrumentName253, atInstrumentName254, atInstrumentName255, atInstrumentName256, atInstrumentName257, atInstrumentName258, 
    atInstrumentName259, atInstrumentName260, atInstrumentName261 
};

#endif
