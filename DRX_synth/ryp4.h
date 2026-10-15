/***************************************************************************
 
 ryp4.h
 
 YM2154 (RYP4) definitions.
 
 17.3.2021 
         
 ***************************************************************************/

#ifndef RYP4_H
#define RYP4_H

// RYP4 write registers

// REG = single register
// RB = register base for all channels, channels are RB+0...RB+5

#define RYP_REG_UNKNOWN1        0x01
#define RYP_REG_TIMERVAL        0x02
#define RYP_REG_TMR_OUT_CTL     0x03
#define RYP_REG_MASTER_LEVEL    0x04
#define RYP_REG_TRIG_B          0x05
#define RYP_REG_TRIG_A          0x06
#define RYP_REG_DAC_MODE        0x07

#define RYP_RB_CH_PAN           0x08

#define RYP_RB_CH_LEVEL_A       0x10
#define RYP_RB_CH_LEVEL_B       0x18

#define RYP_RB_CH_SS1_A         0x20
#define RYP_RB_CH_SS2_A         0x28
#define RYP_RB_CH_SS3_A         0x30

#define RYP_RB_CH_SS1_B         0x38
#define RYP_RB_CH_SS2_B         0x40
#define RYP_RB_CH_SS3_B         0x48

// RYP4 read registers

// REG = single register
// RB = register base for all A/D-channels, channels are RB+0...RB+9

#define RYP_RB_ADVAL            0x01

#define RYP_REG_INTACK          0x0E

// Output controls for chorus
#define CHORUS_0                0x70    // totally off (FET off)
#define CHORUS_1                0x60    // about off (T=0,C=0)
#define CHORUS_2                0x00    // slight chorus
#define CHORUS_3                0x40    // normal chorus
#define CHORUS_4                0x20    // "stereo symphonic"

// Midi note nr limits
#define MIN_NOTE                36      // C2 (BD)
#define MAX_NOTE                79      // G5 (Cuica Lo)

#define NOTE_INDEX(nr)          ((uint8_t)(nr - MIN_NOTE))

#endif  // RYP4_H
