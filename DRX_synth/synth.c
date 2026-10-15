/******************************************************************************

  DRX-70 FM synth
  ===============

  OPQ chip controlling module
  
  SDCC Z80

  17.3.2021

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "DRX_synth.h"
#include "synth.h"
#include "drum.h"
#include "uicom.h"
#include "ioregs.h"
#include "opq.h"
#include "rom2.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define OPQ_ADDR                0xC000  // OPQ chip address

#define VOICES                  8

#define CARR_MASK_OP0        0x01
#define CARR_MASK_OP1        0x02
#define CARR_MASK_OP2        0x04
#define CARR_MASK_OP3        0x08

#define VIB_STEP             (50/RTC_TIME)

#define VIB_DELAY            0
#define VIB_RAMP             1
#define VIB_IDLE             2

#define MAX_TSTAMP           0xFFFFFFFF

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

typedef struct voice_t {
    bool in_use; 
    uint32_t tstamp;
    bool note_off_pending;
    uint8_t notenr; 
    uint8_t veloc;
    int16_t bend_offs;
    uint8_t octave;
    uint8_t vib_state;
    uint8_t vib_ctr;
    uint8_t vib_level;
    int16_t fill;       // fill to 16 bytes
} VOICE_T;

/* =====================================================================
------------------------  Global variables  ------------------------- */

VOICE_T voice[VOICES];

bool sustain_pedal_on = false;
int16_t pitch_bend_val = 0;
uint8_t cur_soundbank = 0;

// Velocity scaling tables
// - modulator
const int8_t vsl_m_tbl[8][32] = {
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },                                                // 0: no velocity sensitivity
    { 40,34,29,25,21,17,14,12,9,7,5,4,2,1,0,-1,-2,-3,-3,-4,-5,-5,-5,-6,-6,-6,-7,-7,-7,-7,-7,-7 },                       // 1: exponential 1
    { 31,25,20,16,12,8,5,3,0,-2,-4,-5,-7,-8,-9,-10,-11,-12,-12,-13,-14,-14,-14,-15,-15,-15,-16,-16,-16,-16,-16,-16 },   // 2: exponential 1, higher level
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-2,-2,-3,-4,-4,-6,-7,-8,-10,-13 },                                 // 3: exponential 2
    { 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15 },                    // 4: linear
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15 },                           // 5: only end, linear
    { 0,0,0,0,0,0,0,0,0,-2,-4,-5,-7,-8,-9,-10,-11,-12,-12,-13,-14,-14,-14,-15,-15,-15,-16,-16,-16,-16,-16,-16 },        // 6: only end, exponential
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,-1,-2,-2,-2,-3,-3,-3,-4,-4,-4,-5,-5,-6,-6,-7,-7,-8 }                              // 7: only end, freehand
};

// - carrier
const int8_t vsl_c_tbl[8][32] = {
    { 31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0 },              // 0: linear 1
    { 24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7 },              // 1: linear 1, higher level
    { 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,-1,-2,-3,-4,-5,-6,-7,-8,-9,-10,-11,-12,-13,-14,-15 },        // 2: linear 1, even higher level
    { 25,25,24,23,22,21,20,20,19,18,17,16,15,15,14,13,12,11,10,10,9,8,7,6,5,5,4,3,2,1,0,0 },                // 3: linear 2
    { 21,21,20,19,18,17,16,16,15,14,13,12,11,11,10,9,8,7,6,6,5,4,3,2,1,1,0,-1,-2,-3,-4,-4 },                // 4: linear 2, higher level
    { 47,41,36,32,28,24,21,19,16,14,12,11,9,8,7,6,5,4,4,3,2,2,2,1,1,1,0,0,0,0,0,0 },                        // 5: exponential
    { 40,34,29,25,21,17,14,12,9,7,5,4,2,1,0,-1,-2,-3,-3,-4,-5,-5,-5,-6,-6,-6,-7,-7,-7,-7,-7,-7 },           // 6: exponential, higher level
//    { 25,22,20,17,15,13,12,10,9,8,7,6,5,5,4,3,3,3,2,2,2,1,1,1,1,1,1,0,0,0,0,0 },                            // 5: exponential 2
//    { 18,15,13,10,8,6,5,3,2,1,0,-1,-2,-2,-3,-4,-4,-4,-5,-5,-5,-6,-6,-6,-6,-6,-6,-7,-7,-7,-7,-7 },           // 6: exponential 2, higher level
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }                                     // 7: no velocity sensitivity
};

const uint8_t carriers_in_algorithm[] = { 
    CARR_MASK_OP3,
    CARR_MASK_OP3,
    CARR_MASK_OP3,
    CARR_MASK_OP3,
    CARR_MASK_OP3 | CARR_MASK_OP2,
    CARR_MASK_OP3 | CARR_MASK_OP2 | CARR_MASK_OP1,
    CARR_MASK_OP3 | CARR_MASK_OP2 | CARR_MASK_OP1,
    CARR_MASK_OP3 | CARR_MASK_OP2
};

const uint8_t op_mask[] = 
    { CARR_MASK_OP0, CARR_MASK_OP1, CARR_MASK_OP2, CARR_MASK_OP3 };

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void synth_play_task( void );
void select_rom_patch( uint8_t rpn );
void write_OPQ_all_voices( uint8_t regbase, uint8_t regval );
void voice_on( uint8_t vn );
uint8_t get_atten_value( uint8_t op );
void voice_off( uint8_t vn );
void voice_cut( uint8_t vn );
void voice_bend( uint8_t vn );
int16_t calc_bend_offset( uint8_t note_nr );
uint8_t alloc_voice( void );
void set_vibrato( uint8_t depth );
void init_voices( void );
void write_OPQ_reg( uint8_t regnr, uint8_t regval );
uint8_t read_OPQ_reg( uint8_t regnr );
void write_RYP_reg( uint8_t regnr, uint8_t regval );
uint8_t read_RYP_reg( uint8_t regnr );

/* =====================================================================
Init synth module
--------------------------------------------------------------------- */

void init_synth( void )
{
    init_voices();
    select_rom_patch(24);
    start_timer4(VIB_STEP);

}

/* =====================================================================
Synth play task
--------------------------------------------------------------------- */

void synth_play_task( void )
{
    uint8_t vn;
    
    // If no new event for synth, nothing to do
    if (midi_event.status != SYNTH_EVENT)
        return;

    // Handle the new event
    switch (midi_event.type)
    {
    case MIDI_NOTE_ON:
        if (midi_event.note.nr >= MIN_NOTE && midi_event.note.nr <= MAX_NOTE)
        {
            // Allocate voice for the new note and start it
            vn = alloc_voice();
            voice_on(vn);
        }
        //printf("N on  (%d) %d %d\r\n",midi_event.chn+1,midi_event.note.nr,midi_event.note.vel);
        break;
        
    case MIDI_NOTE_OFF:
        // End all voices which are playing this note
        for (vn = 0; vn < VOICES; vn++)
        {
            if (voice[vn].in_use && voice[vn].notenr == midi_event.note.nr)
            {
                // If pedal down, don't end the voice, just mark it pending
                if (sustain_pedal_on)
                    voice[vn].note_off_pending = true;
                else
                {
                    voice_off(vn);
                    voice[vn].note_off_pending = false;
                }
            }
        }
        //printf("N off (%d) %d %d\r\n",midi_event.chn+1,midi_event.note.nr,midi_event.note.vel);
        break;
    
    case MIDI_CTRL_CHANGE:
        switch (midi_event.ctrl.nr)
        {
        case 1:     // Modulation
            set_vibrato(midi_event.ctrl.val);
            break;

        case 32:     // Bank select LSB
            // Banks:
            // There are only 3 full banks (0,1,2) of 128 sound banks (A...F) + one 64 sound bank (G) 
            if (midi_event.ctrl.val <= 3)
                cur_soundbank = midi_event.ctrl.val;
            break;

        case 64:     // Sustain pedal
            if (midi_event.ctrl.val > 64)
                sustain_pedal_on = true;
            else
            {
                sustain_pedal_on = false;

                // pedal up -> end all voices which have already got note off
                for (vn = 0; vn < VOICES; vn++)
                {
                    if (voice[vn].note_off_pending)
                    {
                        voice[vn].note_off_pending = false;
                        voice_off(vn);
                    }
                }
            }
            break;

        case 91:     // Effects level 1 (reverb)
            patch.echo = (midi_event.ctrl.val > 64) ? 1 : 0;
            update_ech_fm_am();
            break;

        case 93:     // Effects level 3 (chorus)
            if (midi_event.ctrl.val > 100) settings.chorus_type = 4;
            else if (midi_event.ctrl.val > 75) settings.chorus_type = 3;
            else if (midi_event.ctrl.val > 50) settings.chorus_type = 2;
            else if (midi_event.ctrl.val > 25) settings.chorus_type = 1;
            else settings.chorus_type = 0;
            update_chorus();
            break;

        case 123:     // All notes off
            for (vn = 0; vn < VOICES; vn++)
            {
                voice_off(vn);
                voice[vn].note_off_pending = false;
            }
            break;

        default:
            break;
        }
        //printf("Ctrl  (%d) %d %d\r\n",midi_event.chn+1,midi_event.ctrl.nr,midi_event.ctrl.val);
        break;

    case MIDI_PROG_CHANGE:
        request_patch_from_ui(128*cur_soundbank + midi_event.prog.nr);
        //printf("Prog  (%d) %d\r\n",midi_event.chn+1,midi_event.prog.nr);
        break;

    case MIDI_PITCH_BEND:
        pitch_bend_val = midi_event.bend.val;
        
        // Pitch bend affects all active voices immediately
        for (vn = 0; vn < VOICES; vn++)
        {
            if (voice[vn].in_use)
            {
                voice_bend(vn);
            }
        }
        //printf("Bend  (%d) %d\r\n",midi_event.chn+1,midi_event.bend.val);
        break;

    default:
        break;
    }
    
    // Mark event handled
    midi_event.status = NO_EVENT;
}

/* =====================================================================
Set patch from ROM2
--------------------------------------------------------------------- */

void select_rom_patch( uint8_t rpn )
{
    uint8_t *romp;
    uint8_t op;
    
    romp = rpp_table[rpn];
    
    // printf("romp=%04X\r\n",romp);

    patch.feedback = (romp[PSO_FEEDB_ALG] >> 3) & 0x07;
    patch.algorithm = romp[PSO_FEEDB_ALG] & 0x07;
    patch.outchan = 2;  // OCH1
    patch.echo = 0;

    patch.lfo_control = 4;
    patch.lfo_fm_delay = 0;
    patch.lfo_fm_rate = 0;
    patch.lfo_fm_depth = 0;
    patch.lfo_am_depth = 0;
    
    for (op = 0; op < 4; op++)
    {
        patch.op_en[op]      = 1;   // all ops active
        patch.lfo_am_en[op]  =  romp[PSO_DECAY      + op] >> 7;
        
        patch.factor[op]     =  romp[PSO_FACTOR     + op] & 0x0F;
        patch.waveform[op]   = (romp[PSO_DECAY      + op] & 0x40) >> 6;
        patch.vsl_sel[op]    =  0;                                   // VSL does not exist in ROM patch
        patch.kse[op]        = (romp[PSO_ATTACK     + op] >> 6) & 0x03;
        patch.ksl_hi_sel[op] =  romp[PSO_KSL_HI_TBL + op];
        patch.ksl_lo_sel[op] =  romp[PSO_KSL_LO_TBL + op];
        
        patch.detune[op]     =  romp[PSO_DETUNE     + op];
        patch.attack[op]     =  romp[PSO_ATTACK     + op] & 0x1F;
        patch.decay[op]      =  romp[PSO_DECAY      + op] & 0x1F;
        patch.sustain[op]    =  romp[PSO_SUSTAIN    + op] & 0x1F;
        patch.sus_level[op]  =  15 - ((romp[PSO_SUSL_REL   + op] >> 4) & 0x0F);
        patch.release[op]    =  romp[PSO_SUSL_REL   + op] & 0x0F;
       
        patch.level[op]      = 99 - (99*romp[PSO_ATTEN_BASE+op]/128);
       
        //printf("select: op%d lev=%02X i1=%02X i2=%02X\r\n",op,patch.level[op],patch.ksl_lo_sel[op],patch.ksl_hi_sel[op]);
    }
    
    activate_patch();
}

/* =====================================================================
Activate patch
--------------------------------------------------------------------- */

void activate_patch( void )
{
    uint8_t vn;
    
    // Cut all voices before changing the patch
    for (vn = 0; vn < VOICES; vn++)
        voice_cut(vn);

    // Update all fields from patch to OPQ registers
    update_lfo();
    update_och_fb_alg();
    update_ech_fm_am();
    update_detune();
    update_factor();
    update_atten();
    update_kse_attack();
    update_ame_wf_decay();
    update_sustain();
    update_susl_rel();
    update_act_ops();
}

/* =====================================================================
Update functions for patch structure fields -> OPQ registers.
--------------------------------------------------------------------- */

void update_lfo( void )
{
    write_OPQ_reg(OPQ_REG_LFO, (patch.lfo_control << 4) | patch.lfo_control);
}
// ---------------------------------------------------------------------

void update_och_fb_alg( void )
{
    write_OPQ_all_voices(OPQ_RB_FEEDB_ALG, (patch.outchan << 6) | (patch.feedback << 3) | patch.algorithm);
    patch.carriers = carriers_in_algorithm[patch.algorithm];
}
// ---------------------------------------------------------------------

void update_ech_fm_am( void )
{
    patch.echo_am = (patch.echo << 7) | patch.lfo_am_depth;
    write_OPQ_all_voices(OPQ_RB_VIB_TREM, patch.echo_am | (patch.lfo_fm_depth << 4));
}
// ---------------------------------------------------------------------

void update_detune( void )
{
    for (uint8_t op = 0; op < 4; op++)
        write_OPQ_all_voices(OPQ_RB_0_DETUNE+(8*op), patch.detune[op]);
}
// ---------------------------------------------------------------------

void update_factor( void )
{
    for (uint8_t op = 0; op < 4; op++)
        write_OPQ_all_voices(OPQ_RB_0_FACTOR+(8*op), 0x80 | patch.factor[op]);
}
// ---------------------------------------------------------------------

void update_atten( void )
{
    // update only struct
    for (uint8_t op = 0; op < 4; op++)
    {
        if (patch.level[op] == 0)
            patch.atten_base[op] = 127;
        else
            patch.atten_base[op] = 128 - (128*patch.level[op]/99);
    }
}
// ---------------------------------------------------------------------

void update_kse_attack( void )
{
    for (uint8_t op = 0; op < 4; op++)
        write_OPQ_all_voices(OPQ_RB_0_ATTACK+(8*op), (patch.kse[op] << 6) | patch.attack[op]);
}
// ---------------------------------------------------------------------

void update_ame_wf_decay( void )
{
    for (uint8_t op = 0; op < 4; op++)
        write_OPQ_all_voices(OPQ_RB_0_DECAY+(8*op), (patch.lfo_am_en[op] << 7) | (patch.waveform[op] << 6) | patch.decay[op]);
}
// ---------------------------------------------------------------------

void update_sustain( void )
{
    for (uint8_t op = 0; op < 4; op++)
        write_OPQ_all_voices(OPQ_RB_0_SUSTAIN+(8*op), patch.sustain[op]);
}
// ---------------------------------------------------------------------

void update_susl_rel( void )
{
    for (uint8_t op = 0; op < 4; op++)
    {
        // Note: sustain level value:
        // - in patch-structure: 0=min, 15=max
        // - in OPQ register: 0=max, 15=min
        patch.susl_rel[op] = ((15 - patch.sus_level[op]) << 4) | patch.release[op];
        write_OPQ_all_voices(OPQ_RB_0_SUSL_REL+(8*op), patch.susl_rel[op]);
    }
}
// ---------------------------------------------------------------------

void update_act_ops( void )
{
    patch.act_ops = (patch.op_en[3] << 6) | (patch.op_en[2] << 5) | (patch.op_en[1] << 4) | (patch.op_en[0] << 3);
}

/* =======================================================================
Start a voice.

Times:
- calc freq/oct: 380 us
- write freq + susl_rel: 320 us
- attenuate scale + write: 730 us
- whole thing: 1.65 ms
----------------------------------------------------------------------- */

void voice_on( uint8_t vn )
{
    VOICE_T *vp;
    uint16_t freq;
    uint8_t oct;

    vp = &voice[vn];
    
    vp->bend_offs = calc_bend_offset(midi_event.note.nr);
    
    // Set frequencies
    freq = NOTE_TO_FREQ(midi_event.note.nr) + vp->bend_offs;
    oct = NOTE_TO_OCTAVE(midi_event.note.nr);

    write_OPQ_reg(OPQ_RB_13_FREQ_HI + vn,(oct << 4) | (freq >> 8));
    write_OPQ_reg(OPQ_RB_02_FREQ_HI + vn,(oct << 4) | (freq >> 8));
    write_OPQ_reg(OPQ_RB_13_FREQ_LO + vn,freq & 0xFF);
    write_OPQ_reg(OPQ_RB_02_FREQ_LO + vn,freq & 0xFF);

    // Set sus levels from patch, release rates = max
    // This method is taken from the PSR-70 firmware 
    // Why release rates are handled like this, don't know
    write_OPQ_reg(OPQ_RB_0_SUSL_REL + vn, patch.susl_rel[0] | 0x0F); 
    write_OPQ_reg(OPQ_RB_1_SUSL_REL + vn, patch.susl_rel[1] | 0x0F); 
    write_OPQ_reg(OPQ_RB_2_SUSL_REL + vn, patch.susl_rel[2] | 0x0F); 
    write_OPQ_reg(OPQ_RB_3_SUSL_REL + vn, patch.susl_rel[3] | 0x0F); 

    // Set attenuate values for operators
    write_OPQ_reg(OPQ_RB_0_ATTEN + vn, get_atten_value(0));
    write_OPQ_reg(OPQ_RB_1_ATTEN + vn, get_atten_value(1));
    write_OPQ_reg(OPQ_RB_2_ATTEN + vn, get_atten_value(2));
    write_OPQ_reg(OPQ_RB_3_ATTEN + vn, get_atten_value(3));

    // Set vibrato to 0 if sound contains a vibrato ramp
    if (patch.lfo_fm_rate != 0)
        write_OPQ_reg(OPQ_RB_VIB_TREM + vn, patch.echo_am);
    
    // Trigger all active operators on
    write_OPQ_reg(OPQ_REG_TRIG, patch.act_ops + vn);

    vp->notenr = midi_event.note.nr;
    vp->octave = oct;
    vp->veloc = midi_event.note.vel;
    vp->tstamp = cur_timestamp();
    vp->note_off_pending = false;
    vp->vib_state = VIB_DELAY;
    vp->vib_ctr = 0;
    vp->in_use = true;
}

/* =======================================================================
Calculate attenuation value for the operator.

Principles:
- Base value for attenuation is taken from patch
- Velocity scaling: attenuation is modified according to note velocity, 
  using attenuation curves separately for modulators and carriers
- Keyboard scaling: attenuation is further modified according 
  to note nr, using keyboard scaling tables in PSR-70 ROM2.

Algorithm is mostly taken from PSR-70 firmware. There were no velocity 
scaling tables in PSR-70, because it does not respond to velocity.

Time: 124 us
----------------------------------------------------------------------- */

uint8_t get_atten_value( uint8_t op )
{
    uint8_t noteval;
    int16_t atten;

    // Base value for attenuation from patch
    atten = patch.atten_base[op];

    // --- Velocity scaling
    
    // Modify attenuation according to note velocity
    if (patch.carriers & op_mask[op])
        // Carrier
        atten += vsl_c_tbl[patch.vsl_sel[op]][midi_event.note.vel >> 2];
    else
        // Modulator
        atten += vsl_m_tbl[patch.vsl_sel[op]][midi_event.note.vel >> 2];

    // --- Keyboard scaling
    
    // Scale note nr to noteval for indexing the KSL-tables
    // Midi note is in 24...107 (C1...B7)
    // Noteval is in 0...36, mid-C (note 60) is 18.
    noteval = (midi_event.note.nr >> 1) - 12;
    if (noteval > 36) noteval = 36;
    
    // Modify attenuation according to keyboard scaling-tables in PSR-70 ROM2
    if (noteval >= 18)
    {   
        // ksl_hi_tbl[] is used above mid-C
        atten += ksl_hi_tbl[patch.ksl_hi_sel[op]][noteval-18];
        //printf("ksl_hi: %02X\r\n",ksl_hi_tbl[patch.ksl_hi_sel[op]][noteval-18]);
    }
    else
    {
        // ksl_lo_tbl[] is used below mid-C
        atten += ksl_lo_tbl[patch.ksl_lo_sel[op]][noteval];
        //printf("ksl_lo: %02X\r\n",ksl_lo_tbl[patch.ksl_lo_sel[op]][noteval]);
    }

    // Limit result to 0...127
    if (atten > 127)
        return 127;
    else if (atten < 0)
        return 0;
    else
        return (uint8_t) atten;
}

/* =======================================================================
End a voice. This method is taken from the PSR-70 firmware. Why the actual
sustain leve/release rate is written just before triggering off, don't know.
----------------------------------------------------------------------- */

void voice_off( uint8_t vn )
{
    VOICE_T *vp;

    vp = &voice[vn];
    
    write_OPQ_reg(OPQ_RB_0_SUSL_REL + vn, patch.susl_rel[0]); 
    write_OPQ_reg(OPQ_RB_1_SUSL_REL + vn, patch.susl_rel[1]); 
    write_OPQ_reg(OPQ_RB_2_SUSL_REL + vn, patch.susl_rel[2]); 
    write_OPQ_reg(OPQ_RB_3_SUSL_REL + vn, patch.susl_rel[3]); 

    write_OPQ_reg(OPQ_REG_TRIG,vn);     // all triggers off
    
    vp->in_use = false;
    vp->tstamp = cur_timestamp();
}

/* =======================================================================
Cut a voice. This method to handle the sustain level/release rate is 
taken from PSR-70 firmware.
----------------------------------------------------------------------- */

void voice_cut( uint8_t vn )
{
    // Set sus level = max, release rate = 0xB for modulators, 0xC for carriers
    write_OPQ_reg(OPQ_RB_0_SUSL_REL + vn, 0x0B);  // Op0 is always modulator
    write_OPQ_reg(OPQ_RB_1_SUSL_REL + vn, patch.carriers & CARR_MASK_OP1 ? 0x0C : 0x0B); 
    write_OPQ_reg(OPQ_RB_2_SUSL_REL + vn, patch.carriers & CARR_MASK_OP2 ? 0x0C : 0x0B); 
    write_OPQ_reg(OPQ_RB_3_SUSL_REL + vn, patch.carriers & CARR_MASK_OP3 ? 0x0C : 0x0B); 

    write_OPQ_reg(OPQ_REG_TRIG,vn);     // all triggers off
}

/* =======================================================================
Update voice frequency according to pitch bend while the voice is on.
Performance: 1 ms
----------------------------------------------------------------------- */

void voice_bend( uint8_t vn )
{
    VOICE_T *vp;
    uint16_t freq;
    uint8_t freq_hi;

    vp = &voice[vn];
    
    vp->bend_offs = calc_bend_offset(vp->notenr);

    freq = NOTE_TO_FREQ(vp->notenr) + vp->bend_offs;
    freq_hi = (vp->octave << 4) | (freq >> 8);

    write_OPQ_reg(OPQ_RB_13_FREQ_HI | vn,freq_hi);
    write_OPQ_reg(OPQ_RB_02_FREQ_HI | vn,freq_hi);
    write_OPQ_reg(OPQ_RB_13_FREQ_LO | vn,freq & 0xFF);
    write_OPQ_reg(OPQ_RB_02_FREQ_LO | vn,freq & 0xFF);
}

/* =======================================================================
Calculate frequency offset caused by current pitch bend value (-256...+256).
Pitch bend range fixed to +-2 semitones.

SDCC does some kind of over-optimization, a bit weird expressions are needed.

Performance:
- when bend!=0, 550 us
----------------------------------------------------------------------- */

int16_t calc_bend_offset( uint8_t note_nr )
{
    uint8_t note_idx;
    uint16_t bend_freq,nominal_freq;
    int16_t offs_down;
    uint16_t bendabs,offs_up;
    
    if (pitch_bend_val == 0)
    {
        // bend = 0 ==> offset = 0
       return 0;
    }

    note_idx = NOTE_INDEX(note_nr) % 12;
    nominal_freq = NOTE_TO_FREQ(note_nr);
    
    if (pitch_bend_val < 0)
    {
        // Bend down
        // unsigned values has to be used to avoid overflow 
        // in the multiplication (you don't want 32-bit multiply with Z80)
        bend_freq = bend_down_freq[note_idx];
        bendabs = (uint16_t)(-pitch_bend_val);
        offs_down = -((bendabs * (nominal_freq - bend_freq))/256);
        return offs_down;
    }
    else // pitch_bend_val > 0
    {
        // Bend up: scale the freq
        bend_freq = bend_up_freq[note_idx];
        offs_up = (pitch_bend_val * (bend_freq - nominal_freq))/256;
        return offs_up;
    }
}

/* =======================================================================
Allocate a voice. Return its index in voice[] table. 
A voice is always allocated. If there are no free voices, the longest
playing is cut.

Performance:
- all voices free: 0.68 ms
- 7 voices in use: 0.48 ms
- 8 voices in use: 1.1 ms
----------------------------------------------------------------------- */

uint8_t alloc_voice( void )
{
    uint8_t vn,oldest_vn = 0;
    uint32_t oldest_ts;
    bool found_free = false;

    // Search for a free voice that has been longest out of use
    oldest_ts = MAX_TSTAMP;
    for (vn = 0; vn < VOICES; vn++)
    {
        if (!voice[vn].in_use)
        {
            found_free = true;
            if (voice[vn].tstamp < oldest_ts)
            {
                oldest_ts = voice[vn].tstamp;
                oldest_vn = vn;
            }
        }
    }
    
    if (!found_free)
    {
        // No free voices: Search for a voice that has been longest in use
        oldest_ts = MAX_TSTAMP;
        for (vn = 0; vn < VOICES; vn++)
        {
            if (voice[vn].tstamp < oldest_ts)
            {
                oldest_ts = voice[vn].tstamp;
                oldest_vn = vn;
            }
        }
    }
    
    // Cut the old voice. This is done also for a free voice, 
    // if it happens to be in a long release phase.
    voice_cut(oldest_vn);

    return oldest_vn;
}

/* =====================================================================
Set vibrato (FM)
--------------------------------------------------------------------- */

void set_vibrato( uint8_t depth )
{
    if ((depth & 0x70) == 0)
        write_OPQ_all_voices(OPQ_RB_VIB_TREM, patch.echo_am | (patch.lfo_fm_depth << 4));
    else
        write_OPQ_all_voices(OPQ_RB_VIB_TREM, patch.echo_am | (depth & 0x70));
}

/* ======================================================================= */
/* =====================        vibrato_task          ==================== */
/* ======================================================================= */

/* =====================================================================
Auto vibrato task
If sound has delayed and/or ramped vibrato, produce delay and ramp
--------------------------------------------------------------------- */

void vibrato_task( void )
{
    uint8_t vn;

    // Run only at 50 ms interval
    if (!timeout4)
        return;

    start_timer4(VIB_STEP);

    // If rate==0, depth is constant, has been set in update_ech_fm_am()
    // or if depth==0, no need to handle vibrato
    if (patch.lfo_fm_rate == 0 || patch.lfo_fm_depth == 0)
        return;

    // For all voices in use
    for (vn = 0; vn < VOICES; vn++)
    {
        if (voice[vn].in_use)
        {
            // Handle current vibrato state
            switch (voice[vn].vib_state)
            {
            case VIB_DELAY:
                // Delay running
                voice[vn].vib_ctr++;
                if (voice[vn].vib_ctr > (uint8_t)(2*patch.lfo_fm_delay))
                {
                    voice[vn].vib_ctr = 0;
                    voice[vn].vib_level = 0;
                    voice[vn].vib_state = VIB_RAMP;

                }
                break;

            case VIB_RAMP:
                // Ramp running
                voice[vn].vib_ctr++;
                if (voice[vn].vib_ctr > (uint8_t)(7 - patch.lfo_fm_rate))
                {
                    voice[vn].vib_ctr = 0;
                    voice[vn].vib_level++;
                    write_OPQ_reg(OPQ_RB_VIB_TREM + vn, patch.echo_am | (voice[vn].vib_level << 4));
                    if (voice[vn].vib_level == patch.lfo_fm_depth)
                        voice[vn].vib_state = VIB_IDLE;
                }
                break;

            default:    // VIB_IDLE
                break;
            }
        }
    }
}

/* =====================================================================
Init voices
--------------------------------------------------------------------- */

void init_voices( void )
{
    uint8_t vn;
    
    for (vn = 0; vn < VOICES; vn++)
    {
        voice[vn].in_use = false;
        voice[vn].tstamp = 0;
        voice[vn].bend_offs = 0;
        voice[vn].vib_state = VIB_IDLE;
        voice[vn].vib_ctr = 0;
    }
}

/* =====================================================================
Write regval to 8 consecutive OPQ registers starting from regbase.
--------------------------------------------------------------------- */

void write_OPQ_all_voices( uint8_t regbase, uint8_t regval )
{
    regbase; regval;
    
    __asm
    
	ld	hl, #2
	add	hl, sp
    ld	c, (hl)     ; regbase from stack
    inc hl
    ld	d, (hl)     ; regval from stack
    
	ld	b, #8       ; loop counter
    ld  h, #0xc0    ; OPQ addr hi byte
	
10000$:
	ld	l, #0x00    ; OPQ status register
10001$:
	ld	a, (hl)     ; wait for busy-flag
	rlca
	jr	C,10001$
    
    ld  l, c        ; cur regnr
    ld  (hl), d     ; regval to OPQ
    
    inc c           ; next regnr
    djnz 10000$     ; repeat 8 times
    
    __endasm;

/*    
    uint8_t vn; 
    
    for (vn = 0; vn < VOICES; vn++)
        write_OPQ_reg(regbase + vn, regval); 
*/
}

/* =====================================================================
Write one register to OPQ
--------------------------------------------------------------------- */

void write_OPQ_reg( uint8_t regnr, uint8_t regval )
{
    regnr; regval;

    __asm

; uint8_t *regp = (uint8_t *)(OPQ_ADDR + OPQ_REG_BUSY);
	ld	hl, #0xc000

; while ((*regp & OPQ_BUSY_MASK) != 0) ;
10000$:
	ld	a, (hl)
	rlca
	jr	C,10000$

; regp = (uint8_t *)(OPQ_ADDR + regnr);
	ld	iy, #2
	add	iy, sp
	ld	l, 0 (iy)

; *regp = regval;
	ld	a, 1 (iy)
	ld	(hl), a

    __endasm;
}

/*
void write_OPQ_reg( uint8_t regnr, uint8_t regval )
{
    uint8_t *regp = (uint8_t *)(OPQ_ADDR + OPQ_REG_BUSY);
    
    while ((*regp & OPQ_BUSY_MASK) != 0)
        ;
    
    regp = (uint8_t *)(OPQ_ADDR + regnr);
    *regp = regval;
    
    //printf("%02X=%02X ",regnr,regval);
}
*/

/* =====================================================================
Read one register from YM3806
--------------------------------------------------------------------- */

uint8_t read_OPQ_reg( uint8_t regnr )
{
    uint8_t *regp = (uint8_t *)(OPQ_ADDR + regnr);

    return *regp;
}

/* ============================ EOF ====================================== */
