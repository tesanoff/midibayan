/*
 *
 * This file contains PROGMEM constants for custom selection of musical instruments
 * available in the ATEMP HW synthesizer.
 *
 */

#ifndef _ATEMP_HW_H
#define _ATEMP_HW_H

// groups of instruments
const char atGroupName0[] PROGMEM   = "Фортепиано";
const char atGroupName1[] PROGMEM   = "Электронные пианино";
const char atGroupName2[] PROGMEM   = "Клавишные";
const char atGroupName3[] PROGMEM   = "Перкуссия";
const char atGroupName4[] PROGMEM   = "Органы";
const char atGroupName5[] PROGMEM   = "Гитары";
const char atGroupName6[] PROGMEM   = "Басы";
const char atGroupName7[] PROGMEM   = "Струнные";
const char atGroupName8[] PROGMEM   = "Оркестр";
const char atGroupName9[] PROGMEM   = "Медные духовые";
const char atGroupName10[] PROGMEM  = "Деревянные духовые";
const char atGroupName11[] PROGMEM  = "Leads";
const char atGroupName12[] PROGMEM  = "Pads";
const char atGroupName13[] PROGMEM  = "Спецэффекты";
const char atGroupName14[] PROGMEM  = "Этнические";
const char atGroupName15[] PROGMEM  = "Ударные";
const char atGroupName16[] PROGMEM  = "Звуки";
#define atNumberOfGroups    17
const char * const  atGroupName[atNumberOfGroups] PROGMEM = {
    atGroupName0, atGroupName1, atGroupName2, atGroupName3, atGroupName4, atGroupName5, atGroupName6, atGroupName7, 
    atGroupName8, atGroupName9, atGroupName10, atGroupName11, atGroupName12, atGroupName13, atGroupName14, atGroupName15, 
    atGroupName16
};
// First and Last indices for each group
const uint8_t atGroupMap[atNumberOfGroups * 2] PROGMEM = {
    0, 3,           // Фортепиано
    4, 16,          // Электронные пианино
    17, 20,         // Клавишные
    21, 31,         // Перкуссия
    32, 55,         // Органы
    56, 81,         // Гитары
    82, 104,        // Басы
    105, 110,       // Струнные
    111, 125,       // Оркестр
    126, 143,       // Медные духовые
    144, 167,       // Деревянные духовые
    168, 178,       // Leads
    179, 187,       // Pads
    188, 196,       // Спецэффекты
    197, 205,       // Этнические
    206, 218,       // Ударные
    219, 261        // Звуки
};

// All Instrument names
const char atInstrumentName0[] PROGMEM  = "Grand Piano 1";                  // 0:0
const char atInstrumentName1[] PROGMEM  = "Bright Piano 2";                 // 0:1
const char atInstrumentName2[] PROGMEM  = "(El Grd) Piano 3";               // 0:2
const char atInstrumentName3[] PROGMEM  = "Honky-tonk Piano";               // 0:3
const char atInstrumentName4[] PROGMEM  = "Electro Piano 1";                // 0:4
const char atInstrumentName5[] PROGMEM  = "St.Soft El. Piano";              // 8:4
const char atInstrumentName6[] PROGMEM  = "Detuned El. Piano";              // 9:4
const char atInstrumentName7[] PROGMEM  = "FM+SA El. Piano";                // 16:4
const char atInstrumentName8[] PROGMEM  = "60's El. Piano";                 // 24:4
const char atInstrumentName9[] PROGMEM  = "Hard Rhodes 1";                  // 25:4
const char atInstrumentName10[] PROGMEM  = "Electro Piano 126";             // 126:4
const char atInstrumentName11[] PROGMEM  = "Electro Piano 2";               // 0:5
const char atInstrumentName12[] PROGMEM  = "Detuned El. Piano 2";           // 8:5
const char atInstrumentName13[] PROGMEM  = "Detuned El. Piano";             // 9:5
const char atInstrumentName14[] PROGMEM  = "St. FM El. Piano";              // 16:5
const char atInstrumentName15[] PROGMEM  = "Hard FM El. Piano";             // 24:5
const char atInstrumentName16[] PROGMEM  = "Electro Piano 126";             // 126:5
const char atInstrumentName17[] PROGMEM  = "Harpsichord";                   // 0:6
const char atInstrumentName18[] PROGMEM  = "Coupled Harpsichord";           // 8:6
const char atInstrumentName19[] PROGMEM  = "Clavi";                         // 0:7
const char atInstrumentName20[] PROGMEM  = "Celesta";                       // 0:8
const char atInstrumentName21[] PROGMEM  = "Glockenspiel";                  // 0:9
const char atInstrumentName22[] PROGMEM  = "Музыкальная шкатулка";          // 0:10
const char atInstrumentName23[] PROGMEM  = "Вибрафон";                      // 0:11
const char atInstrumentName24[] PROGMEM  = "Маримба";                       // 0:12
const char atInstrumentName25[] PROGMEM  = "Barafon";                       // 16:12
const char atInstrumentName26[] PROGMEM  = "Ксилофон";                      // 0:13
const char atInstrumentName27[] PROGMEM  = "Трубчатые колокола";            // 0:14
const char atInstrumentName28[] PROGMEM  = "Церковные колокола";            // 8:14
const char atInstrumentName29[] PROGMEM  = "Карильон";                      // 9:14
const char atInstrumentName30[] PROGMEM  = "Сантур";                        // 0:15
const char atInstrumentName31[] PROGMEM  = "Сантур 2";                      // 1:15
const char atInstrumentName32[] PROGMEM  = "Drawbar Organ";                 // 0:16
const char atInstrumentName33[] PROGMEM  = "Орган 101";                     // 1:16
const char atInstrumentName34[] PROGMEM  = "Detuned Organ 1";               // 8:16
const char atInstrumentName35[] PROGMEM  = "Орган 109";                     // 9:16
const char atInstrumentName36[] PROGMEM  = "Орган 60-х 1";                  // 16:16
const char atInstrumentName37[] PROGMEM  = "Even Bar";                      // 33:16
const char atInstrumentName38[] PROGMEM  = "Органный Бас";                  // 40:16
const char atInstrumentName39[] PROGMEM  = "Орган 1-126";                   // 126:16
const char atInstrumentName40[] PROGMEM  = "Percussive Organ";              // 0:17
const char atInstrumentName41[] PROGMEM  = "Organ 201";                     // 1:17
const char atInstrumentName42[] PROGMEM  = "Detuned Organ 2";               // 8:17
const char atInstrumentName43[] PROGMEM  = "Detuned Organ 3";               // 9:17
const char atInstrumentName44[] PROGMEM  = "Organ 2-126";                   // 126:17
const char atInstrumentName45[] PROGMEM  = "Рок Орган";                     // 0:18
const char atInstrumentName46[] PROGMEM  = "Rotary Organ";                  // 8:18
const char atInstrumentName47[] PROGMEM  = "Rotary Organ Slow";             // 16:18
const char atInstrumentName48[] PROGMEM  = "Rotary Organ Fast";             // 24:18
const char atInstrumentName49[] PROGMEM  = "Церковный Орган";               // 0:19
const char atInstrumentName50[] PROGMEM  = "Церковный Орган 2";             // 8:19
const char atInstrumentName51[] PROGMEM  = "Reed Organ";                    // 0:20
const char atInstrumentName52[] PROGMEM  = "Аккордеон (Франц.)";            // 0:21
const char atInstrumentName53[] PROGMEM  = "Аккордеон (Итал.)";             // 8:21
const char atInstrumentName54[] PROGMEM  = "Гармоника";                     // 0:22
const char atInstrumentName55[] PROGMEM  = "Аккордеон Танго";               // 0:23
const char atInstrumentName56[] PROGMEM  = "Гитара Акуст. Нейлон";          // 0:24
const char atInstrumentName57[] PROGMEM  = "Укулеле";                       // 8:24
const char atInstrumentName58[] PROGMEM  = "Гитара Нейлон 1";               // 16:24
const char atInstrumentName59[] PROGMEM  = "VeloHarmnix";                   // 24:24
const char atInstrumentName60[] PROGMEM  = "Гитара Акуст. Сталь";           // 0:25
const char atInstrumentName61[] PROGMEM  = "Гитара 12-струн";               // 7:25
const char atInstrumentName62[] PROGMEM  = "Гитара 12-струн 1";             // 8:25
const char atInstrumentName63[] PROGMEM  = "Гитара Нейлон+Сталь";           // 9:25
const char atInstrumentName64[] PROGMEM  = "Мандолина";                     // 16:25
const char atInstrumentName65[] PROGMEM  = "Гитара (сталь) 2";              // 32:25
const char atInstrumentName66[] PROGMEM  = "Гитара (сталь) 126";            // 126:25
const char atInstrumentName67[] PROGMEM  = "Эл. Гитара Джаз";               // 0:26
const char atInstrumentName68[] PROGMEM  = "Гитара Гавайская";              // 8:26
const char atInstrumentName69[] PROGMEM  = "Эл. Гитара (чистая)";           // 0:27
const char atInstrumentName70[] PROGMEM  = "Эл. Гитара (хорус)";            // 8:27
const char atInstrumentName71[] PROGMEM  = "Эл. Гитара (хорус) 2";          // 9:27
const char atInstrumentName72[] PROGMEM  = "Эл. Гит. (чистая) 126";         // 126:
const char atInstrumentName73[] PROGMEM  = "Эл. Гит. (приглуш.)";           // 0:28
const char atInstrumentName74[] PROGMEM  = "Эл. Гитара (фанк)";             // 8:28
const char atInstrumentName75[] PROGMEM  = "Эл. Гит. (overdrive)";          // 0:29
const char atInstrumentName76[] PROGMEM  = "Эл. Гит. (overd.) 126";         // 126:29
const char atInstrumentName77[] PROGMEM  = "Эл. Гит. (distorsion)";         // 0:30
const char atInstrumentName78[] PROGMEM  = "Эл. Гитара (feedback)";         // 8:30
const char atInstrumentName79[] PROGMEM  = "Эл. Гитара (Гарм.)";            // 0:31
const char atInstrumentName80[] PROGMEM  = "Эл. Гит. (feedback) 2";         // 8:31
const char atInstrumentName81[] PROGMEM  = "Гитара Акуст. (Гарм.)";         // 16:31
const char atInstrumentName82[] PROGMEM  = "Бас Акустический";              // 0:32
const char atInstrumentName83[] PROGMEM  = "Бас-гитара (пальц.)";           // 0:33
const char atInstrumentName84[] PROGMEM  = "Бас-гитара (медиатор)";         // 0:34
const char atInstrumentName85[] PROGMEM  = "Бас-гитара безладовая";         // 0:35
const char atInstrumentName86[] PROGMEM  = "Бас-гитара Slap 1";             // 0:36
const char atInstrumentName87[] PROGMEM  = "Бас-гитара Slap 1-126";         // 126:36
const char atInstrumentName88[] PROGMEM  = "Бас-гитара Slap 2";             // 0:37
const char atInstrumentName89[] PROGMEM  = "Синт. Бас 1";                   // 0:38
const char atInstrumentName90[] PROGMEM  = "Синт. Бас 3";                   // 8:38
const char atInstrumentName91[] PROGMEM  = "Бас TB303";                     // 9:38
const char atInstrumentName92[] PROGMEM  = "Бас Техно";                     // 10:38
const char atInstrumentName93[] PROGMEM  = "Reso HS Bass";                  // 16:38
const char atInstrumentName94[] PROGMEM  = "Синт. Бас 1-126";               // 126:38
const char atInstrumentName95[] PROGMEM  = "Синт. Бас 2";                   // 0:39
const char atInstrumentName96[] PROGMEM  = "Синт. Бас 201";                 // 1:39
const char atInstrumentName97[] PROGMEM  = "Modular Bass";                  // 2:39
const char atInstrumentName98[] PROGMEM  = "Seq Bass";                      // 3:39
const char atInstrumentName99[] PROGMEM  = "Синт. Бас 4";                   // 8:39
const char atInstrumentName100[] PROGMEM  = "Rubber Bass";                  // 16:39
const char atInstrumentName101[] PROGMEM  = "SH101 Bass 1";                 // 17:39
const char atInstrumentName102[] PROGMEM  = "SH101 Bass 2";                 // 18:39
const char atInstrumentName103[] PROGMEM  = "Smooth Bass";                  // 19:39
const char atInstrumentName104[] PROGMEM  = "Синт. Бас 2-126";              // 126:39
const char atInstrumentName105[] PROGMEM  = "Скрипка";                      // 0:40
const char atInstrumentName106[] PROGMEM  = "Альт";                         // 0:41
const char atInstrumentName107[] PROGMEM  = "Виолончель";                   // 0:42
const char atInstrumentName108[] PROGMEM  = "Контрабас";                    // 0:43
const char atInstrumentName109[] PROGMEM  = "Струнные  (тремоло)";          // 0:44
const char atInstrumentName110[] PROGMEM  = "Струнные (пиццикато)";         // 0:45
const char atInstrumentName111[] PROGMEM  = "Арфа";                         // 0:46
const char atInstrumentName112[] PROGMEM  = "Литавры";                      // 0:47
const char atInstrumentName113[] PROGMEM  = "Струнный ансамбль 1";          // 0:48
const char atInstrumentName114[] PROGMEM  = "Оркестр";                      // 8:48
const char atInstrumentName115[] PROGMEM  = "Струнный ансамбль 2";          // 0:49
const char atInstrumentName116[] PROGMEM  = "ST Slow Strings";              // 10:49
const char atInstrumentName117[] PROGMEM  = "Синт. Струнные 1";             // 0:50
const char atInstrumentName118[] PROGMEM  = "Синт. Струнные 3";             // 8:50
const char atInstrumentName119[] PROGMEM  = "Синт. Струнные 2";             // 0:51
const char atInstrumentName120[] PROGMEM  = "Хор 'А-а-а'";                  // 0:52
const char atInstrumentName121[] PROGMEM  = "Голос 'О-о-о'";                // 0:53
const char atInstrumentName122[] PROGMEM  = "Синт. Голос";                  // 0:54
const char atInstrumentName123[] PROGMEM  = "Оркестр Hit";                  // 0:55
const char atInstrumentName124[] PROGMEM  = "Impact Hit";                   // 8:55
const char atInstrumentName125[] PROGMEM  = "Philiy Hit";                   // 9:55
const char atInstrumentName126[] PROGMEM  = "Труба";                        // 0:56
const char atInstrumentName127[] PROGMEM  = "Труба 2";                      // 1:56
const char atInstrumentName128[] PROGMEM  = "Флюгельгорн";                  // 8:56
const char atInstrumentName129[] PROGMEM  = "Труба (яркая)";                // 24:56
const char atInstrumentName130[] PROGMEM  = "Труба 126";                    // 126:56
const char atInstrumentName131[] PROGMEM  = "Тромбон";                      // 0:57
const char atInstrumentName132[] PROGMEM  = "Туба";                         // 0:58
const char atInstrumentName133[] PROGMEM  = "Труба с сурдиной";             // 0:59
const char atInstrumentName134[] PROGMEM  = "Труба с сурдиной 2";           // 1:59
const char atInstrumentName135[] PROGMEM  = "Труба с сурдиной 126";         // 126:59
const char atInstrumentName136[] PROGMEM  = "Французский рог";              // 0:60
const char atInstrumentName137[] PROGMEM  = "Медная секция";                // 0:61
const char atInstrumentName138[] PROGMEM  = "Медные 2";                     // 8:61
const char atInstrumentName139[] PROGMEM  = "Brass Fall";                   // 16:61
const char atInstrumentName140[] PROGMEM  = "Синт. Медные 1";               // 0:62
const char atInstrumentName141[] PROGMEM  = "Синт. Медные 3";               // 8:62
const char atInstrumentName142[] PROGMEM  = "Синт. Медные 2";               // 0:63
const char atInstrumentName143[] PROGMEM  = "Синт. Медные 4";               // 8:63
const char atInstrumentName144[] PROGMEM  = "Саксофон сопрано";             // 0:64
const char atInstrumentName145[] PROGMEM  = "Саксофон сопрано 126";         // 126:64
const char atInstrumentName146[] PROGMEM  = "Саксофон альт";                // 0:65
const char atInstrumentName147[] PROGMEM  = "Hyper Alto";                   // 8:65
const char atInstrumentName148[] PROGMEM  = "Саксофон альт 126";            // 126:65
const char atInstrumentName149[] PROGMEM  = "Саксофон тенор";               // 0:66
const char atInstrumentName150[] PROGMEM  = "Breathy tenor";                // 8:66
const char atInstrumentName151[] PROGMEM  = "Саксофон баритон";             // 0:67
const char atInstrumentName152[] PROGMEM  = "Баритон 126";                  // 126:67
const char atInstrumentName153[] PROGMEM  = "Гобой";                        // 0:68
const char atInstrumentName154[] PROGMEM  = "Английский рог";               // 0:69
const char atInstrumentName155[] PROGMEM  = "Английский рог 126";           // 126:69
const char atInstrumentName156[] PROGMEM  = "Фагот";                        // 0:70
const char atInstrumentName157[] PROGMEM  = "Фагот 126";                    // 126:70
const char atInstrumentName158[] PROGMEM  = "Кларнет";                      // 0:71
const char atInstrumentName159[] PROGMEM  = "Бас Кларнет";                  // 8:71
const char atInstrumentName160[] PROGMEM  = "Пикколо";                      // 0:72
const char atInstrumentName161[] PROGMEM  = "Флейта";                       // 0:73
const char atInstrumentName162[] PROGMEM  = "Блок-флейта";                  // 0:74
const char atInstrumentName163[] PROGMEM  = "Пан-флейта";                   // 0:75
const char atInstrumentName164[] PROGMEM  = "Бутылка";                      // 0:76
const char atInstrumentName165[] PROGMEM  = "Сякухати";                     // 0:77
const char atInstrumentName166[] PROGMEM  = "Свист";                        // 0:78
const char atInstrumentName167[] PROGMEM  = "Окарина";                      // 0:79
const char atInstrumentName168[] PROGMEM  = "Lead 1 (square)";              // 0:80
const char atInstrumentName169[] PROGMEM  = "Square";                       // 1:80
const char atInstrumentName170[] PROGMEM  = "Sine Wave";                    // 8:80
const char atInstrumentName171[] PROGMEM  = "Lead 2 (sawtooth)";            // 0:81
const char atInstrumentName172[] PROGMEM  = "Saw";                          // 1:81
const char atInstrumentName173[] PROGMEM  = "Lead 3 (calliope)";            // 0:82
const char atInstrumentName174[] PROGMEM  = "Lead 4 (chiff)";               // 0:83
const char atInstrumentName175[] PROGMEM  = "Lead 5 (charang)";             // 0:84
const char atInstrumentName176[] PROGMEM  = "Lead 6 (voice)";               // 0:85
const char atInstrumentName177[] PROGMEM  = "Lead 7 (fifths)";              // 0:86
const char atInstrumentName178[] PROGMEM  = "Lead8 (bass+lead)";            // 0:87
const char atInstrumentName179[] PROGMEM  = "Pad 1 (fantasia)";             // 0:88
const char atInstrumentName180[] PROGMEM  = "Pad 2 (warm)";                 // 0:89
const char atInstrumentName181[] PROGMEM  = "Rotary String";                // 3:89
const char atInstrumentName182[] PROGMEM  = "Pad 3 (polysynth)";            // 0:90
const char atInstrumentName183[] PROGMEM  = "Pad 4 (choir)";                // 0:91
const char atInstrumentName184[] PROGMEM  = "Pad 5 (bowed)";                // 0:92
const char atInstrumentName185[] PROGMEM  = "Pad 6 (metallic)";             // 0:93
const char atInstrumentName186[] PROGMEM  = "Pad 7 (halo)";                 // 0:94
const char atInstrumentName187[] PROGMEM  = "Pad 8 (sweep)";                // 0:95
const char atInstrumentName188[] PROGMEM  = "FX 1 (rain)";                  // 0:96
const char atInstrumentName189[] PROGMEM  = "FX 2 (soundtrack)";            // 0:97
const char atInstrumentName190[] PROGMEM  = "FX 3 (crystal)";               // 0:98
const char atInstrumentName191[] PROGMEM  = "FX 4 (atmosphere)";            // 0:99
const char atInstrumentName192[] PROGMEM  = "FX 5 (brightness)";            // 0:100
const char atInstrumentName193[] PROGMEM  = "FX 6 (goblins)";               // 0:101
const char atInstrumentName194[] PROGMEM  = "FX 7 (echoes)";                // 0:102
const char atInstrumentName195[] PROGMEM  = "Echo Pan";                     // 2:102
const char atInstrumentName196[] PROGMEM  = "FX 8 (sci-fi)";                // 0:103
const char atInstrumentName197[] PROGMEM  = "Ситар";                        // 0:104
const char atInstrumentName198[] PROGMEM  = "Банджо";                       // 0:105
const char atInstrumentName199[] PROGMEM  = "Сямисен";                      // 0:106
const char atInstrumentName200[] PROGMEM  = "Кото";                         // 0:107
const char atInstrumentName201[] PROGMEM  = "Таишо Кото";                   // 8:107
const char atInstrumentName202[] PROGMEM  = "Калимба";                      // 0:108
const char atInstrumentName203[] PROGMEM  = "Волынка";                      // 0:109
const char atInstrumentName204[] PROGMEM  = "Скрипка Fiddle";               // 0:110
const char atInstrumentName205[] PROGMEM  = "Shanai";                       // 0:111
const char atInstrumentName206[] PROGMEM  = "Колокольчик";                  // 0:112
const char atInstrumentName207[] PROGMEM  = "Agogo";                        // 0:113
const char atInstrumentName208[] PROGMEM  = "Стальные барабаны";            // 0:114
const char atInstrumentName209[] PROGMEM  = "Деревяшка";                    // 0:115
const char atInstrumentName210[] PROGMEM  = "Кастанеты";                    // 8:115
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
const char atInstrumentName227[] PROGMEM  = "Дождь";                        // 1:122
const char atInstrumentName228[] PROGMEM  = "Гром";                         // 2:122
const char atInstrumentName229[] PROGMEM  = "Ветер";                        // 3:122
const char atInstrumentName230[] PROGMEM  = "Поток";                        // 4:122
const char atInstrumentName231[] PROGMEM  = "Bubble";                       // 5:122
const char atInstrumentName232[] PROGMEM  = "Bird Tweet";                   // 0:123
const char atInstrumentName233[] PROGMEM  = "Собака";                       // 1:123
const char atInstrumentName234[] PROGMEM  = "Лошадиный галоп";              // 2:123
const char atInstrumentName235[] PROGMEM  = "Птичка 2";                     // 3:123
const char atInstrumentName236[] PROGMEM  = "Телефон";                      // 0:124
const char atInstrumentName237[] PROGMEM  = "Телефон 2";                    // 1:124
const char atInstrumentName238[] PROGMEM  = "Скрип двери";                  // 2:124
const char atInstrumentName239[] PROGMEM  = "Закрывание двери";             // 3:124
const char atInstrumentName240[] PROGMEM  = "Царапина";                     // 4:124
const char atInstrumentName241[] PROGMEM  = "Wind Chime";                   // 5:124
const char atInstrumentName242[] PROGMEM  = "Вертолет";                     // 0:125
const char atInstrumentName243[] PROGMEM  = "Старт Двигателя";              // 1:125
const char atInstrumentName244[] PROGMEM  = "Автомобильные тормоза";        // 2:125
const char atInstrumentName245[] PROGMEM  = "Проезд машины";                // 3:125
const char atInstrumentName246[] PROGMEM  = "Авария";                       // 4:125
const char atInstrumentName247[] PROGMEM  = "Полицейская сирена";           // 5:125
const char atInstrumentName248[] PROGMEM  = "Поезд";                        // 6:125
const char atInstrumentName249[] PROGMEM  = "Взлёт истребителя";            // 7:125
const char atInstrumentName250[] PROGMEM  = "Космический корабль";          // 8:125
const char atInstrumentName251[] PROGMEM  = "Burst Noise";                  // 9:125
const char atInstrumentName252[] PROGMEM  = "Аплодисменты";                 // 0:126
const char atInstrumentName253[] PROGMEM  = "Смех";                         // 1:126
const char atInstrumentName254[] PROGMEM  = "Крик";                         // 2:126
const char atInstrumentName255[] PROGMEM  = "Удар";                         // 3:126
const char atInstrumentName256[] PROGMEM  = "Биение сердца";                // 4:126
const char atInstrumentName257[] PROGMEM  = "Шаги";                         // 5:126
const char atInstrumentName258[] PROGMEM  = "Выстрел";                      // 0:127
const char atInstrumentName259[] PROGMEM  = "Очередь выстрелов";            // 1:127
const char atInstrumentName260[] PROGMEM  = "Бластер";                      // 2:127
const char atInstrumentName261[] PROGMEM  = "Взрыв";                        // 3:127
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

// Each pair of numbers defines 1 instrument: <bank>,<instrument>
const uint8_t atInstrumentMap[atNumberOfInstruments * 2] PROGMEM = {
    0,0, 0,1, 0,2, 0,3, 0,4, 8,4, 9,4, 16,4, 24,4, 25,4, 126,4, 0,5, 8,5, 9,5, 16,5, 24,5, 126,5, 0,6, 8,6, 0,7, 
    0,8, 0,9, 0,10, 0,11, 0,12, 16,12, 0,13, 0,14, 8,14, 9,14, 0,15, 1,15, 0,16, 1,16, 8,16, 9,16, 16,16, 33,16, 40,16, 
    126,16, 0,17, 1,17, 8,17, 9,17, 126,17, 0,18, 8,18, 16,18, 24,18, 0,19, 8,19, 0,20, 0,21, 8,21, 0,22, 0,23, 0,24, 8,24, 
    16,24, 24,24, 0,25, 7,25, 8,25, 9,25, 16,25, 32,25, 126,25, 0,26, 8,26, 0,27, 8,27, 9,27, 126,27, 0,28, 8,28, 0,29, 126,29, 
    0,30, 8,30, 0,31, 8,31, 16,31, 0,32, 0,33, 0,34, 0,35, 0,36, 126,36, 0,37, 0,38, 8,38, 9,38, 10,38, 16,38, 126,38, 0,39, 
    1,39, 2,39, 3,39, 8,39, 16,39, 17,39, 18,39, 19,39, 126,39, 0,40, 0,41, 0,42, 0,43, 0,44, 0,45, 0,46, 0,47, 0,48, 8,48, 
    0,49, 10,49, 0,50, 8,50, 0,51, 0,52, 0,53, 0,54, 0,55, 8,55, 9,55, 0,56, 1,56, 8,56, 24,56, 126,56, 0,57, 0,58, 0,59, 
    1,59, 126,59, 0,60, 0,61, 8,61, 16,61, 0,62, 8,62, 0,63, 8,63, 0,64, 126,64, 0,65, 8,65, 126,65, 0,66, 8,66, 0,67, 126,67, 
    0,68, 0,69, 126,69, 0,70, 126,70, 0,71, 8,71, 0,72, 0,73, 0,74, 0,75, 0,76, 0,77, 0,78, 0,79, 0,80, 1,80, 8,80, 0,81, 
    1,81, 0,82, 0,83, 0,84, 0,85, 0,86, 0,87, 0,88, 0,89, 3,89, 0,90, 0,91, 0,92, 0,93, 0,94, 0,95, 0,96, 0,97, 0,98, 
    0,99, 0,100, 0,101, 0,102, 2,102, 0,103, 0,104, 0,105, 0,106, 0,107, 8,107, 0,108, 0,109, 0,110, 0,111, 0,112, 0,113, 0,114, 0,115, 
    8,115, 0,116, 8,116, 0,117, 8,117, 0,118, 8,118, 9,118, 0,119, 0,120, 1,120, 2,120, 5,120, 6,120, 0,121, 1,121 , 0,122, 1,122, 2,122, 
    3,122, 4,122, 5,122, 0,123, 1,123, 2,123, 3,123, 0,124, 1,124, 2,124, 3,124, 4,124, 5,124, 0,125, 1,125, 2,125, 3,125, 4,125, 5,125, 
    6,125, 7,125, 8,125, 9,125, 0,126, 1,126, 2,126, 3,126, 4,126, 5,126, 0,127, 1,127, 2,127, 3,127
}; 

#endif
