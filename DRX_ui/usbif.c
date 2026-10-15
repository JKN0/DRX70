/***************************************************************************

 usbif.h

 USB midi interface for DRX-70.

 13.12.2021  - First version

 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <datatypes.h>
#include <compiler_defs.h>
#include <C8051F340_defs.h>
#include "portdefs.h"
#include "silabs_gen.h"
#include "DRX_ui.h"
#include "patchman.h"
#include "synth_com/parcom_master.h"
#include "USB_Mididrv/USB_Mididrv.h"
#include "usbif.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define SYSEX_START          0xF0

#define printf               printf_fast

/* =======================================================================
------------------------ I/O ------------------------------------------ */

#define TP1                  P3_4
#define TP2                  P3_3

/* =====================================================================
------------------------ Structures --------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

extern DATA BYTE inbuf[];
extern BIT cr_received;
extern BIT saving_from_ui;
extern IDATA int cur_patch;
extern IDATA INT8 cur_drumkit;

BIT last_save_was_patch = FALSE;

XDATA BYTE * CODE patch_p[] = {
    NULL,
    NULL,
    NULL,
    &patch.algorithm,       // SCMD_SET_ALGORITHM   
    &patch.feedback,        // SCMD_SET_FEEDBACK    
    &patch.outchan,         // SCMD_SET_OUTCHAN     
    &patch.echo,            // SCMD_SET_ECHO        
    patch.op_en,            // SCMD_SET_OP_EN       
    &patch.lfo_control,     // SCMD_SET_LFO_CONTROL 
    &patch.lfo_fm_delay,    // SCMD_SET_LFO_FM_DELAY
    &patch.lfo_fm_rate,     // SCMD_SET_LFO_FM_RATE 
    &patch.lfo_fm_depth,    // SCMD_SET_LFO_FM_DEPTH
    &patch.lfo_am_depth,    // SCMD_SET_LFO_AM_DEPTH
    patch.lfo_am_en,        // SCMD_SET_LFO_AM_EN   
    patch.factor,           // SCMD_SET_FACTOR      
    patch.waveform,         // SCMD_SET_WAVEFORM    
    patch.vsl_sel,          // SCMD_SET_VSL_SEL     
    patch.kse,              // SCMD_SET_KSE         
    patch.ksl_hi_sel,       // SCMD_SET_KSL_HI_SEL  
    patch.ksl_lo_sel,       // SCMD_SET_KSL_LO_SEL  
    patch.level,            // SCMD_SET_LEVEL       
    patch.detune,           // SCMD_SET_DETUNE      
    patch.attack,           // SCMD_SET_ATTACK      
    patch.decay,            // SCMD_SET_DECAY       
    patch.sustain,          // SCMD_SET_SUSTAIN     
    patch.sus_level,        // SCMD_SET_SUS_LEVEL   
    patch.release           // SCMD_SET_RELEASE     
};

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void handle_patch_sysex( void );
void handle_drumkit_sysex( void );
void handle_patch_param_sysex( void );
void handle_drumkit_param_sysex( void );
void handle_settings_param_sysex( void );         
void handle_patch_req_sysex( void );         
void handle_drumkit_req_sysex( void );         
WORD decode_11bits( BYTE idx );
void encode_11bits( WORD val, BYTE idx );
void print_midi_in_msg( BYTE len );

/* =======================================================================
USB interface task initialization.
----------------------------------------------------------------------- */

void usb_if_init( void )
{
    init_usb_mididrv();
}

/* =======================================================================
USB interface task.
----------------------------------------------------------------------- */

void usb_if_task( void )
{
    BYTE msglen;
    BYTE ch;

    // Run USB driver
    run_usb_mididrv();

    // --- Receive from USB

    msglen = get_midi_message();
    if (msglen != 0)
    {
//        if (midi_in_msg[0] != 0xFE)
//            print_midi_in_msg(msglen);

        if ((midi_in_msg[0] & 0xF0) != 0xF0)
        {
            // All non-system messages to synth

            // No need to send messages that would be dropped 
            // anyway by the synth
            ch = midi_in_msg[0] & 0x0F;
            if (settings.sound_ch == MIDI_CHANNEL_OMNI ||
                ch == settings.sound_ch || ch == settings.drum_ch)
            {
                PC_send_midi_msg_to_synth(midi_in_msg,msglen);
            }
        }
        else if (midi_in_msg[0] == SYSEX_START)
        {
            // Sysex messages
            
            // Use experimental manufacturer id 7D
            if (midi_in_msg[1] == 0x7D)
            {
                switch (midi_in_msg[2])     // Main type
                {
                case 1: 
                    if (msglen == 87)
                        handle_patch_sysex();         
                    break;

                case 2:
                    if (msglen == 122)
                        handle_drumkit_sysex();         
                    break;

                case 3:
                    if (msglen == 7)
                        handle_patch_param_sysex();         
                    break;

                case 4:
                    if (msglen == 8)
                        handle_drumkit_param_sysex();         
                    break;

                case 5:
                    if (msglen == 6)
                        handle_settings_param_sysex();         
                    break;

                case 6:
                    if (msglen == 4)
                        handle_patch_req_sysex();         
                    break;

                case 7:
                    if (msglen == 4)
                        handle_drumkit_req_sysex();         
                    break;

                }
            }
        }
        // else: drop other system messages
    }

    // Handle save state
    if (!saving_from_ui && saving_state == SAVE_DONE)
    {
        saving_state = SAVE_IDLE;

        if (last_save_was_patch)
            reload_patch = TRUE;
        else
            reload_drumkit = TRUE;
     }
}

/* =======================================================================
Handle patch dump
----------------------------------------------------------------------- */

void handle_patch_sysex( void )
{
    int patchnr = decode_11bits(3);

    if (patchnr < 448)
    {
        memcpy(&patch,midi_in_msg+5,sizeof(PATCH_T));

        cur_save_pos = patchnr;
        cur_patch = patchnr;
        saving_state = SAVE_PATCH;
        saving_from_ui = FALSE;
        last_save_was_patch = TRUE;
    }
}

/* =======================================================================
Handle drumkit dump
----------------------------------------------------------------------- */

void handle_drumkit_sysex( void )
{
    BYTE dknr = midi_in_msg[3];
    BYTE i;

    if (dknr < 30)
    {
        // Copy name, scmd, pan-values to drumkit struct
        memcpy(&drumkit,midi_in_msg+4,33);

        // Decode sample start and end values 11-bit -> 16-bit
        for (i = 0; i < 21; i++)
        {
            drumkit.s_start[i] = decode_11bits(37+2*i);
            drumkit.s_end[i] = decode_11bits(79+2*i);
        }

        cur_save_pos = dknr;
        cur_drumkit = dknr;
        saving_state = SAVE_DRUMKIT;
        saving_from_ui = FALSE;
        last_save_was_patch = FALSE;
    }
}

/* =======================================================================
Handle patch parameter (single parameter change)
----------------------------------------------------------------------- */

void handle_patch_param_sysex( void )
{
    BYTE scmd = midi_in_msg[3];
    BYTE op   = midi_in_msg[4];
    BYTE pval = midi_in_msg[5];

    if (scmd >= SCMD_SET_ALGORITHM && scmd <= SCMD_SET_RELEASE && op <= 3)
    {
        *(patch_p[scmd] + op) = pval;
        PC_send_byte_param_to_synth(scmd,op,pval);   
    }
}

/* =======================================================================
Handle drumkit parameter (single parameter change)
----------------------------------------------------------------------- */

void handle_drumkit_param_sysex( void )
{
    BYTE scmd = midi_in_msg[3];
    BYTE inst = midi_in_msg[4];
    WORD pval = decode_11bits(5);

    if (inst <= 20)
    {
        switch (scmd)
        {
        case SCMD_SET_PAN:
            drumkit.pan[inst] = (BYTE) pval;
            PC_send_byte_param_to_synth(scmd,inst,(BYTE)pval);   
            break;
            
        case SCMD_SET_S_START:
            drumkit.s_start[inst] = pval;
            PC_send_word_param_to_synth(scmd,inst,pval);   
            break;
            
        case SCMD_SET_S_END:
            drumkit.s_end[inst] = pval;
            PC_send_word_param_to_synth(scmd,inst,pval);   
            break;
        }
    }
}

/* =======================================================================
Handle settings parameter (single parameter change)
----------------------------------------------------------------------- */

void handle_settings_param_sysex( void )
{
    BYTE scmd = midi_in_msg[3];
    BYTE pval = midi_in_msg[4];

    switch (scmd)
    {
    case SCMD_SET_SOUND_CH:    settings.sound_ch = pval;    break;
    case SCMD_SET_DRUM_CH:     settings.drum_ch = pval;     break;
    case SCMD_SET_CHORUS_TYPE: settings.chorus_type = pval; break;
    default: return;
    }

    PC_send_byte_param_to_synth(scmd,0,pval);   
}

/* =======================================================================
Handle patch request sysex: send current patch to USB host as sysex
----------------------------------------------------------------------- */

void handle_patch_req_sysex( void )
{
    midi_out_msg[0] = 0xF0;     // Sysex start
    midi_out_msg[1] = 0x7D;     // Manuf id
    midi_out_msg[2] = 0x11;     // message type: upload patch
    encode_11bits(cur_patch,3); // patch nr
    memcpy(midi_out_msg+5,(XDATA BYTE *)patch,sizeof(PATCH_T));
    midi_out_msg[86] = 0xF7;    // Sysex end

    put_midi_message(87);       // to USB send queue
}

/* =======================================================================
Handle drumkit request sysex: send current drumkit to USB host as sysex
----------------------------------------------------------------------- */

void handle_drumkit_req_sysex( void )
{
    BYTE i;

    midi_out_msg[0] = 0xF0;         // Sysex start
    midi_out_msg[1] = 0x7D;         // Manuf id
    midi_out_msg[2] = 0x12;         // message type: upload drumkit
    midi_out_msg[3] = cur_drumkit;  // drumkit nr
    memcpy(midi_out_msg+4,(XDATA BYTE *)drumkit,33); // name + pan-values
    
    for (i = 0; i < 21; i++)
    {
        encode_11bits(drumkit.s_start[i],37+2*i); // s_start-values
        encode_11bits(drumkit.s_end[i],79+2*i);   // s_end-values
    }
    
    midi_out_msg[121] = 0xF7;       // Sysex end

    put_midi_message(122);          // to USB send queue
}

/* =======================================================================
Decode over 7-bit values from midi_in_msg. The value can be max 11 bits.
0abc defg 0000 hijk --> 0000 0abc defg hijk
----------------------------------------------------------------------- */

WORD decode_11bits( BYTE idx )
{
    WORD msb = midi_in_msg[idx] << 4;
    BYTE lsb = midi_in_msg[idx+1];

    return msb | lsb;
}

/* =======================================================================
Encode over 7-bit values to midi_out_msg. The value can be max 11 bits.
0000 0abc defg hijk --> 0abc defg 0000 hijk
This is a bit weird way of encoding, but it retains the readability of 
the values in hex (like 0x7AB -> 0x7A 0x0B)
----------------------------------------------------------------------- */

void encode_11bits( WORD val, BYTE idx )
{
    midi_out_msg[idx] = val >> 4;
    midi_out_msg[idx+1] = val & 0x0F;
}

/* =======================================================================
Print midi in message
----------------------------------------------------------------------- */

void print_midi_in_msg( BYTE len )
{
    BYTE i;

    printf("Midi from USB: ");

    for (i = 0; i < len; i++)
        printf("%x ",midi_in_msg[i]);

    printf("\r\n");
}

/* ============================ EOF ====================================== */
