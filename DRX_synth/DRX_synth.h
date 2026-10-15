/******************************************************************************

  DRX-70 FM synth
  ===============

  Main module

  SDCC Z80

  28.2.2021

*****************************************************************************/

#ifndef DRX_SYNTH_H
#define DRX_SYNTH_H

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define MIDI_BUF_LEN            512

#define MIDI_NOTE_ON            0x90
#define MIDI_NOTE_OFF           0x80
#define MIDI_CTRL_CHANGE        0xB0
#define MIDI_PROG_CHANGE        0xC0
#define MIDI_PITCH_BEND         0xE0
#define MIDI_SYSEX_START        0xF0
#define MIDI_SYSEX_END          0xF7
#define MIDI_TIMING_CLOCK       0xF8
#define MIDI_START              0xFA
#define MIDI_CONTINUE           0xFB
#define MIDI_STOP               0xFC
#define MIDI_ACTIVE_SENSING     0xFE

#define NO_EVENT                0
#define SYNTH_EVENT             1
#define DRUM_EVENT              2

#define RTC_TIME                10

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

typedef struct midi_event_t {
    uint8_t status; 
    uint8_t type; 
    uint8_t chn; 
    union {
        struct note_t {
            uint8_t nr; 
            uint8_t vel; 
        } note;
        struct ctrl_t {
            uint8_t nr; 
            uint8_t val; 
        } ctrl;
        struct prog_t {
            uint8_t nr; 
        } prog;
        struct bend_t {
            int16_t val; 
        } bend;
    };
} MIDI_EVENT_T;

typedef struct patch_t {
    // General
    uint8_t algorithm;      // 0...7
    uint8_t feedback;       // 0...7
    uint8_t outchan;        // bitmap: chorus | direct
    uint8_t echo;           // on/off
    uint8_t op_en[4];       // on/off per op

    uint8_t lfo_control;    // LFO freq/disable
    uint8_t lfo_fm_delay;   // FM start delay 0...7
    uint8_t lfo_fm_rate;    // FM increase rate 0...7
    uint8_t lfo_fm_depth;   // FM depth (vibrato) 0...7
    uint8_t lfo_am_depth;   // AM depth (tremolo) 0...3
    uint8_t lfo_am_en[4];   // AM enable per op

    // Per operator
    uint8_t factor[4];      // mult factor 0...15
    uint8_t waveform[4];    // 0=sine/1=half-sine
    uint8_t vsl_sel[4];     // Velocity Scaling for Level curve select 0...?
    uint8_t kse[4];         // Key Scaling for Envelope 0...3
    uint8_t ksl_hi_sel[4];  // Key Scaling for Level, high half, curve select 0...22 
    uint8_t ksl_lo_sel[4];  // Key Scaling for Level, low half, curve select 0...13

    uint8_t level[4];       // level 0...99 ?
    uint8_t detune[4];      // 0...63, 32 = in tune
    uint8_t attack[4];      // attack rate 0...31
    uint8_t decay[4];       // decay rate 0...31
    uint8_t sustain[4];     // sustain rate 0...31
    uint8_t sus_level[4];   // sustain level 0...15, value in patch is 15-regval!
    uint8_t release[4];     // release rate 0...15
    
    // Shortcuts, calculated from above
    uint8_t carriers;       // from algorithm
    uint8_t act_ops;        // from op_en[]
    uint8_t echo_am;        // from echo + lfo_am_depth
    uint8_t atten_base[4];  // from level
    uint8_t susl_rel[4];    // from sus_level + release
} PATCH_T;

typedef struct drumkit_t {
    uint8_t  pan[21];      // pan value
    uint16_t s_start[21];  // sample start
    uint16_t s_end[21];    // sample end
} DRUMKIT_T;

typedef struct settings_t {
    uint8_t synth_ch;      // channel for sounds, 0...16, 16 = omni
    uint8_t drum_ch;       // channel for drums, 0...15
    uint8_t chorus_type;   // chorus type, 0...4
} SETTINGS_T;

/* =====================================================================
------------------------  Global variables  ------------------------- */

extern MIDI_EVENT_T midi_event;
extern PATCH_T patch;
extern DRUMKIT_T drumkit;
extern SETTINGS_T settings;

extern bool timeout1;
extern bool timeout2;
extern bool timeout3;
extern bool timeout4;
extern bool timeout5;

/* =====================================================================
------------------------ Function prototypes ------------------------ */

uint32_t cur_timestamp( void );
void start_timer1( uint8_t delay );
void start_timer2( uint8_t delay );
void start_timer3( uint8_t delay );
void start_timer4( uint8_t delay );
void start_timer5( uint8_t delay );
void put_midi_msg( uint8_t *midimsg, uint8_t buflen );

#endif  // DRX_SYNTH_H
