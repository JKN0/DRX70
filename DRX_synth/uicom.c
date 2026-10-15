/******************************************************************************

  DRX-70 FM synth
  ===============

  Communication with UI
  
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
#include "parcom_slave.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

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

#define SENDBUF_LEN           10

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

uint8_t sendbuf[SENDBUF_LEN];

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void handle_ui_command( uint8_t *cmdbuf, uint8_t buflen );

/* =====================================================================
Init UI com module
--------------------------------------------------------------------- */

void init_uicom( void )
{
    PC_init();
}

/* =====================================================================
UI communication task
--------------------------------------------------------------------- */

void uicom_task( void )
{
    uint8_t blen = 0;
    uint8_t cmd_type = CMD_NONE;
    uint8_t *pcbuf;
    
    PC_run_tasks();
    
    // Wait for a message from master
    pcbuf = PC_get_rx_buf(&blen,&cmd_type);
    if (pcbuf != NULL)
    {
        //printf("Message %d bytes: %X %X %X ...\r\n",blen,pcbuf[0],pcbuf[1],pcbuf[2]);
        
        // Got message from UI controller
        if (cmd_type == CMD_WRITE)
        {
            // Write command: data from master, handle it
            handle_ui_command(pcbuf,blen);
            PC_ack_rx();            // acknowledge
        }
        else
        {
            // Read command: master requests data
            
            // no read commands yet
            //PC_send_to_master(SEND_NOW,test_data,len);
        }
    }
    
    if (pc_error)
    {
        pc_error = false;
        printf("Master comm error\r\n");
    }
}

/* =====================================================================
Handle command from UI
--------------------------------------------------------------------- */

void handle_ui_command( uint8_t *cmdbuf, uint8_t buflen )
{
    uint8_t op_ins = cmdbuf[1];
    uint8_t val = cmdbuf[2];
    
    switch (cmdbuf[0])
    {
    // - Bulk transfers
    case SCMD_NEW_PATCH:
        memcpy((uint8_t *)patch,cmdbuf+1,buflen-1);
        activate_patch();
        break;

    case SCMD_NEW_DRUMKIT:
        memcpy((uint8_t *)drumkit,cmdbuf+1,buflen-1);
        activate_drumkit();
        break;

    case SCMD_NEW_SETTINGS:
        memcpy((uint8_t *)settings,cmdbuf+1,buflen-1);
        update_chorus();
        break;

    // - Single sound parameters
    case SCMD_SET_ALGORITHM:
        patch.algorithm = val;
        update_och_fb_alg();
        break;
        
    case SCMD_SET_FEEDBACK:
        patch.feedback = val;
        update_och_fb_alg();
        break;
        
    case SCMD_SET_OUTCHAN:
        patch.outchan = val;
        update_och_fb_alg();
        break;
        
    case SCMD_SET_ECHO:
        patch.echo = val;
        update_ech_fm_am();
        break;

    case SCMD_SET_OP_EN:
        patch.op_en[op_ins] = val;
        update_act_ops();
        break;

    case SCMD_SET_LFO_CONTROL:
        patch.lfo_control = val;
        update_lfo();
        break;

    case SCMD_SET_LFO_FM_DELAY:
        patch.lfo_fm_delay = val;
        // not done in OPQ
        break;

    case SCMD_SET_LFO_FM_RATE:
        patch.lfo_fm_rate = val;
        // not done in OPQ
        break;

    case SCMD_SET_LFO_FM_DEPTH:
        patch.lfo_fm_depth = val;
        update_ech_fm_am();
        break;

    case SCMD_SET_LFO_AM_DEPTH:
        patch.lfo_am_depth = val;
        update_ech_fm_am();
        break;

    case SCMD_SET_LFO_AM_EN:
        patch.lfo_am_en[op_ins] = val;
        update_ame_wf_decay();
        break;

    case SCMD_SET_FACTOR:
        patch.factor[op_ins] = val;
        update_factor();
        break;

    case SCMD_SET_WAVEFORM:
        patch.waveform[op_ins] = val;
        update_ame_wf_decay();
        break;

    case SCMD_SET_VSL_SEL:
        patch.vsl_sel[op_ins] = val;
        // not done in OPQ
        break;

    case SCMD_SET_KSE:
        patch.kse[op_ins] = val;
        update_kse_attack();
        break;

    case SCMD_SET_KSL_HI_SEL:
        patch.ksl_hi_sel[op_ins] = val;
        // not done in OPQ
        break;

    case SCMD_SET_KSL_LO_SEL:
        patch.ksl_lo_sel[op_ins] = val;
        // not done in OPQ
        break;

    case SCMD_SET_LEVEL:
        patch.level[op_ins] = val;
        update_atten();
        break;

    case SCMD_SET_DETUNE:
        patch.detune[op_ins] = val;
        update_detune();
        break;

    case SCMD_SET_ATTACK:
        patch.attack[op_ins] = val;
        update_kse_attack();
        break;

    case SCMD_SET_DECAY:
        patch.decay[op_ins] = val;
        update_ame_wf_decay();
        break;

    case SCMD_SET_SUSTAIN:
        patch.sustain[op_ins] = val;
        update_sustain();
        break;

    case SCMD_SET_SUS_LEVEL:
        patch.sus_level[op_ins] = val;
        update_susl_rel();
        break;

    case SCMD_SET_RELEASE:
        patch.release[op_ins] = val;
        update_susl_rel();
        break;
        
    // - Single drum parameters
    case SCMD_SET_PAN:
        //printf("PN:%d,%02X\r\n",op_ins,val);
        drumkit.pan[op_ins] = val;
        update_pan(op_ins);
        break;

    case SCMD_SET_S_START:
        //printf("SS:%d,%X%02X\r\n",op_ins,cmdbuf[3],cmdbuf[2]);
        drumkit.s_start[op_ins] = cmdbuf[2] | (cmdbuf[3] << 8);
        update_ss_bytes(op_ins);
        break;
        
    case SCMD_SET_S_END:
        //printf("SE:%d,%X%02X\r\n",op_ins,cmdbuf[3],cmdbuf[2]);
        drumkit.s_end[op_ins] = cmdbuf[2] | (cmdbuf[3] << 8);
        update_ss_bytes(op_ins);
        break;

    // - Single setting parameters
    case SCMD_SET_SOUND_CH:
        settings.synth_ch = val;
        break;
        
    case SCMD_SET_DRUM_CH:
        settings.drum_ch = val;
        break;
        
    case SCMD_SET_CHORUS_TYPE:
        settings.chorus_type = val;
        update_chorus();
        break;

    // - Other messages
    case SCMD_MIDI_MSG:
        put_midi_msg(cmdbuf+1,buflen-1);
        break;
    }
}

/* =====================================================================
Request patch from UI. This will trigger UI sending SCMD_NEW_PATCH.
--------------------------------------------------------------------- */

void request_patch_from_ui( uint16_t patch_nr )
{
    sendbuf[0] = UREQ_NEW_PATCH;
    sendbuf[1] = patch_nr & 0xFF;
    sendbuf[2] = patch_nr >> 8;
    
    PC_send_to_master(SEND_WITH_REQ,sendbuf,3);
}

/* =====================================================================
Request drumkit from UI. This will trigger UI sending SCMD_NEW_DRUMKIT.
--------------------------------------------------------------------- */

void request_drumkit_from_ui( uint8_t drumkit_nr )
{
    sendbuf[0] = UREQ_NEW_DRUMKIT;
    sendbuf[1] = drumkit_nr;
    
    PC_send_to_master(SEND_WITH_REQ,sendbuf,2);
}

/* ============================ EOF ====================================== */
