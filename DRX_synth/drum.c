/******************************************************************************

  DRX-70 FM synth
  ===============

  RYP4 chip controlling module
  
  SDCC Z80

  17.3.2021

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "DRX_synth.h"
#include "drum.h"
#include "uicom.h"
#include "ioregs.h"
#include "ryp4.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define SPCHANNELS           6    // channels per group

#define NOT_USED             0
#define GROUP_A              1
#define GROUP_B              2

#define MAX_TSTAMP           0xFFFFFFFF

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

typedef struct spchan_t {
    uint32_t tstamp;
    uint8_t notenr; 
    uint8_t level;
} SPCHAN_T;

/* =====================================================================
------------------------  Global variables  ------------------------- */

SPCHAN_T spchanA[SPCHANNELS];
SPCHAN_T spchanB[SPCHANNELS];
uint8_t panreg[SPCHANNELS];          // copy of registers 08H...0DH

const uint8_t trig_mask[] = { 0x01,0x02,0x04,0x08,0x10,0x20 };

// index = notenr - 36
INST_DATA_T inst_data[MAX_NOTE-MIN_NOTE+1] = {
    // group   changed pan  ss1   ss2   ss3         idx sample      note
    { GROUP_A,  false, 0x8, 0x19, 0x81, 0xA7 },  //  0  BD          C2  36
    { GROUP_A,  false, 0x6, 0x19, 0x01, 0x97 },  //  1  Rimshot     C#2 37
    { GROUP_A,  false, 0x6, 0x17, 0x41, 0x8F },  //  2  SD Lo       D2  38
    { GROUP_B,  false, 0xA, 0x1E, 0xC1, 0xFF },  //  3  Claps       D#2 39
    { GROUP_A,  false, 0x6, 0x16, 0x41, 0x73 },  //  4  SD Hi       E2  40
    { GROUP_B,  false, 0xE, 0x00, 0x00, 0x77 },  //  5  Tom 3       F2  41
    { GROUP_A,  false, 0x5, 0x15, 0x81, 0x63 },  //  6  HH closed   F#2 42
    { NOT_USED                               },  //  7              G2  43
    { NOT_USED                               },  //  8              G#2 44
    { GROUP_A,  false, 0xA, 0x1D, 0x01, 0xFF },  //  9  Tom 2       A2  45
    { GROUP_A,  false, 0x5, 0x0F, 0x81, 0x57 },  // 10  HH open     A#2 46
    { NOT_USED                               },  // 11              B2  47
    { GROUP_A,  false, 0x2, 0x1A, 0x81, 0xCF },  // 12  Tom 1       C3  48
    { NOT_USED                               },  // 13              C#3 49
    { NOT_USED                               },  // 14              D3  50
    { GROUP_A,  false, 0xD, 0x00, 0x00, 0xF7 },  // 15  Ride        D#3 51
    { NOT_USED                               },  // 16              E3  52
    { NOT_USED                               },  // 17              F3  53
    { NOT_USED                               },  // 18              F#3 54
    { NOT_USED                               },  // 19              G3  55
    { NOT_USED                               },  // 20              G#3 56
    { NOT_USED                               },  // 21              A3  57
    { NOT_USED                               },  // 22              A#3 58
    { NOT_USED                               },  // 23              B3  59
    { GROUP_B,  false, 0x8, 0x07, 0x80, 0x87 },  // 24  Bongo 1     C4  60
    { GROUP_B,  false, 0x8, 0x09, 0x00, 0xAF },  // 25  Bongo 2     C#4 61
    { GROUP_B,  false, 0x8, 0x08, 0x80, 0x8F },  // 26  Bongo 3     D4  62
    { NOT_USED                               },  // 27              D#4 63
    { NOT_USED                               },  // 28              E4  64
    { GROUP_B,  false, 0xC, 0x0B, 0xC0, 0xEB },  // 29  Timbale Hi  F4  65
    { GROUP_B,  false, 0xA, 0x0E, 0xC1, 0x1B },  // 30  Timbale Lo  F#4 66
    { GROUP_B,  false, 0xC, 0x18, 0x81, 0x9B },  // 31  Agogo Hi    G4  67
    { GROUP_B,  false, 0xA, 0x19, 0xC1, 0xEB },  // 32  Agogo Lo    G#4 68
    { NOT_USED                               },  // 33              A4  69
    { NOT_USED                               },  // 34              A#4 70
    { NOT_USED                               },  // 35              B4  71
    { NOT_USED                               },  // 36              C5  72
    { NOT_USED                               },  // 37              C#5 73
    { NOT_USED                               },  // 38              D5  74
    { GROUP_B,  false, 0x8, 0x0B, 0x00, 0xBB },  // 39  Claves      D#5 75
    { NOT_USED                               },  // 40              E5  76
    { NOT_USED                               },  // 41              F5  77
    { GROUP_B,  false, 0x8, 0x11, 0xC1, 0x37 },  // 42  Cuica Hi    F#5 78
    { GROUP_B,  false, 0x8, 0x13, 0x81, 0x87 }   // 43  Cuica Lo    G5  79
};

#define NOTE_IDX_HHCL             6
#define NOTE_IDX_HHOP             10
#define NOTENR_HHOP               46

                                     // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20
const uint8_t inst_nr_to_note_idx[] = { 0, 1, 2, 3, 4, 5, 6, 9,10,12,15,24,25,26,29,30,31,32,39,42,43 };
    
// Used in initialization
#define ROM_A_INIT_SAMPLE      15   // Ride
#define ROM_B_INIT_SAMPLE       5   // Tom 3

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void drum_play_task( void );
uint8_t alloc_spchannel( uint8_t note_idx );
void trig_spchannel( uint8_t cn, uint8_t group );
void cut_hhop( void );
void write_RYP_reg( uint8_t regnr, uint8_t regval );
uint8_t read_RYP_reg( uint8_t regnr );

/* =====================================================================
Init drum module
--------------------------------------------------------------------- */

void init_drum( void )
{
    uint8_t cn;
    
    // Init sequence from PSR-70
    write_RYP_reg(RYP_REG_UNKNOWN1,0x00);       // unknown init, must be 0
    write_RYP_reg(RYP_REG_TIMERVAL,0x80);       // timerval hi = 0
    write_RYP_reg(RYP_REG_TMR_OUT_CTL,0x00);    // timer = stop, chorus = none
    write_RYP_reg(RYP_REG_MASTER_LEVEL,0x3F);   // master level = max
    write_RYP_reg(RYP_REG_DAC_MODE,0x01);       // DAC mode = serial

    read_RYP_reg(RYP_REG_INTACK);               // remove interrupt

    for (cn = 0; cn < SPCHANNELS; cn++)
    {
        write_RYP_reg(RYP_RB_CH_PAN+cn,0x88);    // all pans to center
        panreg[cn] = 0x88;
    }
    
    write_RYP_reg(RYP_REG_TMR_OUT_CTL,CHORUS_3);    // chorus normal

    // RYP4 needs some samples loaded in every channel, 
    // otherwise it won't start properly
    for (cn = 0; cn < SPCHANNELS; cn++)
    {
        write_RYP_reg(RYP_RB_CH_SS1_A+cn,inst_data[ROM_A_INIT_SAMPLE].ss1);
        write_RYP_reg(RYP_RB_CH_SS2_A+cn,inst_data[ROM_A_INIT_SAMPLE].ss2);
        write_RYP_reg(RYP_RB_CH_SS3_A+cn,inst_data[ROM_A_INIT_SAMPLE].ss3);
        
        write_RYP_reg(RYP_RB_CH_SS1_B+cn,inst_data[ROM_B_INIT_SAMPLE].ss1);
        write_RYP_reg(RYP_RB_CH_SS2_B+cn,inst_data[ROM_B_INIT_SAMPLE].ss2);
        write_RYP_reg(RYP_RB_CH_SS3_B+cn,inst_data[ROM_B_INIT_SAMPLE].ss3);
    }
    
    // Init channel status tables
    for (cn = 0; cn < SPCHANNELS; cn++)
    {
        spchanA[cn].tstamp = 0;
        spchanA[cn].notenr = 0;
        spchanA[cn].level = 0;
        spchanB[cn].tstamp = 0;
        spchanB[cn].notenr = 0;
        spchanB[cn].level = 0;
    }   
}

/* =====================================================================
Drum play task
--------------------------------------------------------------------- */

void drum_play_task( void )
{
    uint8_t cn,note_idx;
    
    // If no new event for drums, nothing to do
    if (midi_event.status != DRUM_EVENT)
        return;

    // Handle the new event
    switch (midi_event.type)
    {
    case MIDI_NOTE_ON:
        if (midi_event.note.nr >= MIN_NOTE && midi_event.note.nr <= MAX_NOTE)
        {
            note_idx = NOTE_INDEX(midi_event.note.nr);
            if (inst_data[note_idx].group != NOT_USED)
            {
                // An instrument exists for this note nr

                // When a HH closed is played, cut HH open
                if (note_idx == NOTE_IDX_HHCL)
                    cut_hhop();

                // allocate a sample play channel for the note and trig it
                cn = alloc_spchannel(note_idx);
                trig_spchannel(cn,inst_data[note_idx].group);
            }
            //printf("N on  (%d) %d %d\r\n",midi_event.chn+1,midi_event.note.nr,midi_event.note.vel);
        }
        break;

    case MIDI_CTRL_CHANGE:
        switch (midi_event.ctrl.nr)
        {
        case 79:     // Sound controller #10, custom controller for DRX-70
            // Shorten HH open sample according to controller value.
            // < 32 = normal length, 32...127 = end position 0x157...0xF8
            // HH open = instrument 8
            if (midi_event.ctrl.val < 32)
                drumkit.s_end[8] = 0x157;
            else
                drumkit.s_end[8] = 375 - midi_event.ctrl.val;
            update_ss_bytes(8);
            break;
        }
        break;

    case MIDI_PROG_CHANGE:
        request_drumkit_from_ui(midi_event.prog.nr);
        //printf("Prog  (%d) %d\r\n",midi_event.chn+1,midi_event.prog.nr);
        break;

    default:
        break;
    }
    
    // Mark event handled
    midi_event.status = NO_EVENT;
}

/* =======================================================================
Allocate a sample play channel. Return its index in spchannelX[] table. 

Try to avoid sample data loading to RYP4 by reusing channels. Sample data
is loaded in two cases:
- there is no channel which already has this sample data
- sample data has been changed by UI
----------------------------------------------------------------------- */

uint8_t alloc_spchannel( uint8_t note_idx )
{
    uint8_t cn,oldest_cn = 0;
    uint32_t oldest_ts;
    bool found_channel = false;
    uint8_t group = inst_data[note_idx].group;

    // Search for a channel which has this sample already loaded 
    // (which has been playing this sample)
    if (group == GROUP_A)
    {
        for (cn = 0; cn < SPCHANNELS; cn++)
        {
            if (spchanA[cn].notenr == midi_event.note.nr)
            {
                found_channel = true;
                break;
            }
        }
    }
    else
    {
        for (cn = 0; cn < SPCHANNELS; cn++)
        {
            if (spchanB[cn].notenr == midi_event.note.nr)
            {
                found_channel = true;
                break;
            }
        }
    }

    // If no ready channel for this note, allocate a channel: 
    // search for a channel that has been longest out of use
    if (!found_channel)
    {
        oldest_ts = MAX_TSTAMP;
        if (group == GROUP_A)
        {
            for (cn = 0; cn < SPCHANNELS; cn++)
            {
                if (spchanA[cn].tstamp < oldest_ts)
                {
                    oldest_ts = spchanA[cn].tstamp;
                    oldest_cn = cn;
                }
            }
        }
        else
        {
            for (cn = 0; cn < SPCHANNELS; cn++)
            {
                if (spchanB[cn].tstamp < oldest_ts)
                {
                    oldest_ts = spchanB[cn].tstamp;
                    oldest_cn = cn;
                }
            }
        }

        cn = oldest_cn;
    }

    // If instrument data has changed (via UI or midi),
    // or channel was jus allocated, load new sample data:
    // - cut the old voice
    // - set the pan value according to the new sample
    // - set the 3 sample select bytes
    if (inst_data[note_idx].changed || !found_channel)
    {
        if (group == GROUP_A)
        {
            // Cut the old voice by rising the channel playback rate
            // This is how the PSR-70 does this
            write_RYP_reg(RYP_RB_CH_LEVEL_A+cn,0x60 | spchanA[cn].level);

            // Set the pan value without affecting the other channel
            panreg[cn] = (panreg[cn] & 0xF0) | inst_data[note_idx].pan;
            write_RYP_reg(RYP_RB_CH_PAN+cn,panreg[cn]);

            // Write sample select bytes to load the new sample
            write_RYP_reg(RYP_RB_CH_SS1_A+cn,inst_data[note_idx].ss1);
            write_RYP_reg(RYP_RB_CH_SS2_A+cn,inst_data[note_idx].ss2);
            write_RYP_reg(RYP_RB_CH_SS3_A+cn,inst_data[note_idx].ss3);
            //printf("SS:%02X %02X %02X\r\n",inst_data[note_idx].ss1,inst_data[note_idx].ss2,inst_data[note_idx].ss3);
        }
        else
        {
            // Cut the old voice by rising the channel playback rate
            // This is how the PSR-70 does this
            write_RYP_reg(RYP_RB_CH_LEVEL_B+cn,0x60 | spchanB[cn].level);
            
            // Set the pan value without affecting the other channel
            panreg[cn] = (panreg[cn] & 0x0F) | (inst_data[note_idx].pan << 4);
            write_RYP_reg(RYP_RB_CH_PAN+cn,panreg[cn]);

            // Write sample select bytes to load the new sample
            write_RYP_reg(RYP_RB_CH_SS1_B+cn,inst_data[note_idx].ss1);
            write_RYP_reg(RYP_RB_CH_SS2_B+cn,inst_data[note_idx].ss2);
            write_RYP_reg(RYP_RB_CH_SS3_B+cn,inst_data[note_idx].ss3);
            //printf("SS:%02X %02X %02X\r\n",inst_data[note_idx].ss1,inst_data[note_idx].ss2,inst_data[note_idx].ss3);
        }

        inst_data[note_idx].changed = false;    // data loaded to RYP4
    }

    return cn;
}

/* =====================================================================
Trigger the given channel's sample
--------------------------------------------------------------------- */

void trig_spchannel( uint8_t cn, uint8_t group )
{
    uint8_t level;
    
    level = midi_event.note.vel >> 2;
    
    if (group == GROUP_A)
    {
        // set channel level
        write_RYP_reg(RYP_RB_CH_LEVEL_A+cn,level);

        // trigger
        write_RYP_reg(RYP_REG_TRIG_A,trig_mask[cn]);
        
        spchanA[cn].tstamp = cur_timestamp();
        spchanA[cn].notenr = midi_event.note.nr;
        spchanA[cn].level = level;
    }
    else  // group B
    {
        // set channel level
        write_RYP_reg(RYP_RB_CH_LEVEL_B+cn,level);

        // trigger
        write_RYP_reg(RYP_REG_TRIG_B,trig_mask[cn]);
        
        spchanB[cn].tstamp = cur_timestamp();
        spchanB[cn].notenr = midi_event.note.nr;
        spchanB[cn].level = level;
    }

}

/* =====================================================================
Cut HH open immediately by rising the channel playback rate
--------------------------------------------------------------------- */

void cut_hhop( void )
{
    uint8_t cn;

    for (cn = 0; cn < SPCHANNELS; cn++)
    {
        if (spchanA[cn].notenr == NOTENR_HHOP)
        {
            write_RYP_reg(RYP_RB_CH_LEVEL_A+cn,0x60 | spchanA[cn].level);
            break;
        }
    }
}

/* =====================================================================
Activate drumkit
--------------------------------------------------------------------- */

void activate_drumkit( void )
{
    uint8_t i;
    
    for (i = 0; i < 21; i++)
    {
        update_pan(i);
        update_ss_bytes(i);
    }
}

/* =====================================================================
Update functions for drumkit structure fields -> inst_data-table
--------------------------------------------------------------------- */

void update_pan( uint8_t inst_nr )
{
    uint8_t note_idx = inst_nr_to_note_idx[inst_nr];
    
    inst_data[note_idx].pan = drumkit.pan[inst_nr];
    inst_data[note_idx].changed = true;
    //printf("UP:%d,%d,%d\r\n",note_idx,inst_nr,drumkit.pan[inst_nr]);
}
/* =====================================================================
--------------------------------------------------------------------- */

void update_ss_bytes( uint8_t inst_nr )
{
    uint8_t note_idx = inst_nr_to_note_idx[inst_nr];
    
    // calculate SS-bytes from sample start and end 
    inst_data[note_idx].ss1 = (drumkit.s_start[inst_nr] >> 4) & 0xFF;
    inst_data[note_idx].ss2 = ((drumkit.s_start[inst_nr] << 4) & 0xF0) | ((drumkit.s_end[inst_nr] >> 8) & 0x0F);
    inst_data[note_idx].ss3 = drumkit.s_end[inst_nr] & 0xFF;
    inst_data[note_idx].changed = true;
    //printf("US:%d,%d,%03X,%03X\r\n",note_idx,inst_nr,drumkit.s_start[inst_nr],drumkit.s_end[inst_nr]);
}

/* =====================================================================
--------------------------------------------------------------------- */

void update_chorus( void )
{
    uint8_t chorus_val = CHORUS_3;

    switch (settings.chorus_type)
    {
    case 0: chorus_val = CHORUS_0; break;
    case 1: chorus_val = CHORUS_1; break;
    case 2: chorus_val = CHORUS_2; break;
    case 3: chorus_val = CHORUS_3; break;
    case 4: chorus_val = CHORUS_4; break;
    }
    
    write_RYP_reg(RYP_REG_TMR_OUT_CTL,chorus_val);
}

/* =====================================================================
Write one register to YM2154
--------------------------------------------------------------------- */

void write_RYP_reg( uint8_t regnr, uint8_t regval )
{
    // Don't know a way to do indirect OUT in SDCC,
    // so this is done in assembly
    regnr; regval;
    
    __asm

    ; regnr from stack
    ld    hl,#2
    add   hl,sp
    ld    a,(hl)
    or    a,#0x80
    ld    c,a             ; C = regnr | 80H

    ; regval from stack
    inc   hl
    ld    a,(hl)          ; A = regval
    
    ; out to RYP chip
    out   (c),a
    
    __endasm;
    
    //printf("%02X=%02X ",regnr,regval);
}

/* =====================================================================
Read one register from YM2154
--------------------------------------------------------------------- */

// disable: warning 59: function 'read_RYP_reg' must return value 
#pragma save
#pragma disable_warning 59

uint8_t read_RYP_reg( uint8_t regnr )
{
    regnr;
    
    // Don't know a way to do indirect IN in SDCC,
    // so this is done in assembly
    __asm
    
    ; regnr from stack
    ld    hl,#2
    add   hl,sp
    ld    a,(hl)
    or    a,#0x80
    ld    c,a         ; C = regnr | 80H

    in    l,(c)       ; return value in L
    
    __endasm;

    return;
}
#pragma restore

/* ============================ EOF ====================================== */
