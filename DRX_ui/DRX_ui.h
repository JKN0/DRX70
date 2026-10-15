/***************************************************************************

 DRX_ui.h

 DRX-70 UI, main module for the 8051-part.

 3.10.2021  - First version

 ***************************************************************************/

#ifndef DRX_UI_H
#define DRX_UI_H

// UI modes
#define MODE_MAIN              0    // main screen
#define MODE_SOUND             1    // sound editing pages
#define MODE_DRUMS             2    // drum editing pages
#define MODE_SETTINGS          3    // settings page

// Edit modes
#define MODE_ORIGINAL          0    
#define MODE_EDITED            1
#define MODE_COMPARE           2

#define NAMELEN                11   // 10 chars + '\0'

#define RTC_TIME               5

#define MIDI_CHANNEL_OMNI      16

// --- Synth commands (UI -> synth)
#define SCMD_NEW_PATCH          1
#define SCMD_NEW_DRUMKIT        2
#define SCMD_NEW_SETTINGS       30

// patch
#define SCMD_SET_ALGORITHM      3
#define SCMD_SET_FEEDBACK       4
#define SCMD_SET_OUTCHAN        5
#define SCMD_SET_ECHO           6
#define SCMD_SET_OP_EN          7     // op specific

#define SCMD_SET_LFO_CONTROL    8
#define SCMD_SET_LFO_FM_DELAY   9
#define SCMD_SET_LFO_FM_RATE   10
#define SCMD_SET_LFO_FM_DEPTH  11
#define SCMD_SET_LFO_AM_DEPTH  12
#define SCMD_SET_LFO_AM_EN     13    // op specific

#define SCMD_SET_FACTOR        14    // op specific
#define SCMD_SET_WAVEFORM      15    // op specific
#define SCMD_SET_VSL_SEL       16    // op specific
#define SCMD_SET_KSE           17    // op specific
#define SCMD_SET_KSL_HI_SEL    18    // op specific
#define SCMD_SET_KSL_LO_SEL    19    // op specific

#define SCMD_SET_LEVEL         20    // op specific
#define SCMD_SET_DETUNE        21    // op specific
#define SCMD_SET_ATTACK        22    // op specific
#define SCMD_SET_DECAY         23    // op specific
#define SCMD_SET_SUSTAIN       24    // op specific
#define SCMD_SET_SUS_LEVEL     25    // op specific
#define SCMD_SET_RELEASE       26    // op specific

// drumkit
#define SCMD_SET_PAN           27    // instr specific
#define SCMD_SET_S_START       28    // instr specific
#define SCMD_SET_S_END         29    // instr specific

// settings
#define SCMD_SET_SOUND_CH      31
#define SCMD_SET_DRUM_CH       32
#define SCMD_SET_CHORUS_TYPE   33

// other
#define SCMD_MIDI_MSG          34

// --- UI requests (synth -> UI)
#define UREQ_NEW_PATCH        101
#define UREQ_NEW_DRUMKIT      102

typedef struct patch_t {
    BYTE name[NAMELEN];  // patch name
    BYTE scmd;           // SCMD_NEW_PATCH
    
    // General
    BYTE algorithm;      // 0...7
    BYTE feedback;       // 0...7
    BYTE outchan;        // bitmap: chorus | direct
    BYTE echo;           // on/off
    BYTE op_en[4];       // on/off per op

    BYTE lfo_control;    // LFO freq/disable
    BYTE lfo_fm_delay;   // FM start delay 0...7
    BYTE lfo_fm_rate;    // FM increase rate 0...7
    BYTE lfo_fm_depth;   // FM depth (vibrato) 0...7
    BYTE lfo_am_depth;   // AM depth (tremolo) 0...3
    BYTE lfo_am_en[4];   // AM enable per op

    // Per operator
    BYTE factor[4];      // mult factor 0...15
    BYTE waveform[4];    // 0=sine/1=half-sine
    BYTE vsl_sel[4];     // Velocity Scaling for Level curve select 0...?
    BYTE kse[4];         // Key Scaling for Envelope 0...3
    BYTE ksl_hi_sel[4];  // Key Scaling for Level, high half, curve select 0...22 
    BYTE ksl_lo_sel[4];  // Key Scaling for Level, low half, curve select 0...13

    BYTE level[4];       // level 0...99 ?
    BYTE detune[4];      // 0...63, 32 = in tune
    BYTE attack[4];      // attack rate 0...31
    BYTE decay[4];       // decay rate 0...31
    BYTE sustain[4];     // sustain rate 0...31
    BYTE sus_level[4];   // sustain level 0...15
    BYTE release[4];     // release rate 0...15
} PATCH_T;

#define PATCHDEF_SIZE     70    // sizeof(PATCH_T) - NAMELEN

typedef struct drumkit_t {
    BYTE name[NAMELEN];  // drumkit name
    BYTE scmd;           // SCMD_NEW_DRUMKIT

    // per instrument
    BYTE pan[21];       // pan value
    WORD s_start[21];   // sample start
    WORD s_end[21];     // sample end
    
    BYTE fill[15];      // make sure > 128 bytes
} DRUMKIT_T;

#define DRUMKITDEF_SIZE  106    // sizeof(DRUMKIT_T) - NAMELEN - sizeof(fill)

typedef struct settings_t {
    BYTE scmd;           // SCMD_NEW_SETTINGS
    
    BYTE sound_ch;       // 0...16, 16 = omni
    BYTE drum_ch;        // 0...15
    BYTE chorus_type;    // 0...4
} SETTINGS_T;

#define SETTINGSDEF_SIZE     4    // sizeof(SETTINGS_T)

extern XDATA PATCH_T patch;
extern XDATA DRUMKIT_T drumkit;
extern XDATA SETTINGS_T settings;

#define TP1                  P3_4
#define TP2                  P3_3
#define TP3                  P3_2
#define TP4                  P1_7

#endif  // DRX_UI_H
