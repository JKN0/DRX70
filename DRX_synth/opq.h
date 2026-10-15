/***************************************************************************
 
 opq.h
 
 YM3806 (OPQ) definitions.
 
 1.3.2021 
         
 ***************************************************************************/

#ifndef OPQ_H
#define OPQ_H

// OPQ registers

// REG = single register
// RB = register base for all operators, channels are RB+0...RB+7
// RB_x = register base for operator x, channels are RB_x+0...RB_x+7
// RB_xy = register base for operator pair xy, channels are RB_xy+0...RB_xy+7

#define OPQ_REG_BUSY        0x00
#define OPQ_BUSY_MASK       0x80

#define OPQ_REG_LFO         0x04
#define OPQ_LFO_DIS_MASK    0x08

#define OPQ_REG_TRIG        0x05
#define OPQ_KEY0            0x08
#define OPQ_KEY1            0x10
#define OPQ_KEY2            0x20
#define OPQ_KEY3            0x40

#define OPQ_RB_FEEDB_ALG    0x10
#define OPQ_OCH1_MASK       0x80
#define OPQ_OCH2_MASK       0x40

#define OPQ_RB_VIB_TREM     0x18
#define OPQ_ECHO_MASK       0x80

#define OPQ_RB_13_FREQ_HI   0x20
#define OPQ_RB_13_FREQ_LO   0x30
#define OPQ_RB_02_FREQ_HI   0x28
#define OPQ_RB_02_FREQ_LO   0x38

#define OPQ_RB_0_DETUNE     0x40
#define OPQ_RB_1_DETUNE     0x48
#define OPQ_RB_2_DETUNE     0x50
#define OPQ_RB_3_DETUNE     0x58
#define OPQ_DETUNE_MASK     0x00

#define OPQ_RB_0_FACTOR     0x40
#define OPQ_RB_1_FACTOR     0x48
#define OPQ_RB_2_FACTOR     0x50
#define OPQ_RB_3_FACTOR     0x58
#define OPQ_FACTOR_MASK     0x80

#define OPQ_RB_0_ATTEN      0x60
#define OPQ_RB_1_ATTEN      0x68
#define OPQ_RB_2_ATTEN      0x70
#define OPQ_RB_3_ATTEN      0x78

#define OPQ_RB_0_ATTACK     0x80
#define OPQ_RB_1_ATTACK     0x88
#define OPQ_RB_2_ATTACK     0x90
#define OPQ_RB_3_ATTACK     0x98

#define OPQ_RB_0_DECAY      0xA0
#define OPQ_RB_1_DECAY      0xA8
#define OPQ_RB_2_DECAY      0xB0
#define OPQ_RB_3_DECAY      0xB8
#define OPQ_TERD_MASK       0x80
#define OPQ_WF_MASK         0x40

#define OPQ_RB_0_SUSTAIN    0xC0
#define OPQ_RB_1_SUSTAIN    0xC8
#define OPQ_RB_2_SUSTAIN    0xD0
#define OPQ_RB_3_SUSTAIN    0xD8

#define OPQ_RB_0_SUSL_REL   0xE0
#define OPQ_RB_1_SUSL_REL   0xE8
#define OPQ_RB_2_SUSL_REL   0xF0
#define OPQ_RB_3_SUSL_REL   0xF8

// FREQ values for one octave
#define FREQ_AIS_PREV        0x444      // A# (previous octave)
#define FREQ_B_PREV          0x485      // B  (previous octave)

#define FREQ_C               0x4CA      // C
#define FREQ_CIS             0x513      // C#
#define FREQ_D               0x560      // D
#define FREQ_DIS             0x5B2      // D#
#define FREQ_E               0x609      // E
#define FREQ_F               0x665      // F
#define FREQ_FIS             0x6C6      // F#
#define FREQ_G               0x72D      // G
#define FREQ_GIS             0x79A      // G#
#define FREQ_A               0x80E      // A
#define FREQ_AIS             0x889      // A#
#define FREQ_B               0x90A      // B

#define FREQ_C_NEXT          0x994      // C  (next octave)
#define FREQ_CIS_NEXT        0xA25      // C# (next octave)

// Midi note nr limits
#define MIN_NOTE             24         // C1
#define MAX_NOTE             107        // B7

// Midi note nr -> FREQ
// index = (note_nr - MIN_NOTE) % 12
const uint16_t note_to_freq[] = {
    FREQ_C,
    FREQ_CIS,
    FREQ_D,
    FREQ_DIS,
    FREQ_E,
    FREQ_F,
    FREQ_FIS,
    FREQ_G,
    FREQ_GIS,
    FREQ_A,
    FREQ_AIS,
    FREQ_B
};

#define NOTE_INDEX(nr)       ((uint8_t)(nr - MIN_NOTE))
#define NOTE_TO_FREQ(nr)     (note_to_freq[(uint8_t)(nr - MIN_NOTE) % (uint8_t)12])
#define NOTE_TO_OCTAVE(nr)   ((uint8_t)(nr - MIN_NOTE+12) / (uint8_t)12)

// Midi note nr -> FREQ bent up +2 semitones
// index = (note_nr - MIN_NOTE) % 12
const uint16_t bend_up_freq[] = {
    FREQ_D,      // C + 2 semitones = D
    FREQ_DIS,    // C# + 2 semitones = D#
    FREQ_E,      // etc.
    FREQ_F,
    FREQ_FIS,
    FREQ_G,
    FREQ_GIS,
    FREQ_A,
    FREQ_AIS,
    FREQ_B,
    FREQ_C_NEXT,
    FREQ_CIS_NEXT
};

// Midi note nr -> FREQ bent down -2 semitones
// index = (note_nr - MIN_NOTE) % 12
const uint16_t bend_down_freq[] = {
    FREQ_AIS_PREV,   // C - 2 semitones = A#
    FREQ_B_PREV,     // C# - 2 semitones = B
    FREQ_C,          // etc.
    FREQ_CIS,
    FREQ_D,
    FREQ_DIS,
    FREQ_E,
    FREQ_F,
    FREQ_FIS,
    FREQ_G,
    FREQ_GIS,
    FREQ_A
};

#endif  // OPQ_H
