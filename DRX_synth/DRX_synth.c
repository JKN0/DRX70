/******************************************************************************

  DRX-70 FM synth
  ===============

  Main module

  SDCC Z80

  28.2.2021

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

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define ENABLE_INTS             __asm EI __endasm;

#define CYCLIC_INC_MIDI(p)      { p++; if (p >= (midi_buf+MIDI_BUF_LEN)) p = midi_buf; }
#define CYCLIC_INC_SERIAL(p)    { p++; if (p >= (outbuf+OUTBUF_LEN)) p = outbuf; }

#define TIME_500MS              (500/RTC_TIME)
#define TIME_30MS               (30/RTC_TIME)

#define OUTBUF_LEN  		    1024

#define MIDI_CHANNEL_OMNI       16

#define NO_MIDI_BYTE  		    0xF4    // unused, does not appear in midi data

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

static volatile uint32_t tick_ctr = 0;

static char outbuf[OUTBUF_LEN];
static char *outbuf_top = outbuf;
static char *outbuf_bot = outbuf;
static bool outbuf_full = false;

static uint8_t midi_buf[MIDI_BUF_LEN];
static bool midi_buf_full = false;
static uint8_t *midi_buf_bot = midi_buf;
static uint8_t *midi_buf_top = midi_buf;

uint8_t midi_msg[3];
MIDI_EVENT_T midi_event = { .status = NO_EVENT };
PATCH_T patch;
DRUMKIT_T drumkit;
SETTINGS_T settings = { .synth_ch = MIDI_CHANNEL_OMNI, .drum_ch = 9, .chorus_type = 3};

// index = (message nr >> 4) & 0x07
const uint8_t midi_length_lut[] = {
    3,      // 0x8X = note off
    3,      // 0x9X = note on
    3,      // 0xAX = poly aftertouch
    3,      // 0xBX = controller
    2,      // 0xCX = program change
    2,      // 0xDX = channel aftertouch
    3,      // 0xEX = pitch bend
    1       // 0xFx = system common/system real time (F1,F2,F3 longer!)
};

bool sanity_led_on = false;
bool midi_led_on = false;

static uint8_t timer1;      // sanity led
static uint8_t timer2;      // parcom rx
static uint8_t timer3;      // parcom tx
static uint8_t timer4;      // vibrato
static uint8_t timer5;      // midi led

bool timeout1;      
bool timeout2;      
bool timeout3;      
bool timeout4;      
bool timeout5;      

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void midi_event_task( void );
bool get_midi_event( void );
bool get_midi_message( void );
uint8_t get_midi_length( uint8_t hdr );
void delayloop( int ms );
void init_hw( void );

/* =====================================================================
Main program
--------------------------------------------------------------------- */

int main()
{
    // Initailizations
    init_hw();
    ENABLE_INTS;

    init_synth();
    init_drum();
    init_uicom();
    
    start_timer1(TIME_500MS);

	printf("\r\n*** DRX_synth ***\r\n");
    
    // --- Main loop
    while(1)
    {
        // Run all tasks
        midi_event_task();
    	synth_play_task();
    	drum_play_task();
        vibrato_task();
        uicom_task();
        
        // Blink sanity led
        if (timeout1)
        {
            start_timer1(TIME_500MS);
            sanity_led_on = !sanity_led_on;
            
            if (sanity_led_on)
                SANITY_LED_ON;
            else
                SANITY_LED_OFF;
        }

        // Turn off midi led after 30 ms
        if (midi_led_on && timeout5)
        {
            MIDI_LED_OFF;
            midi_led_on = false;
        }
    }
}

/* ======================================================================= */
/* =====================      midi_event_task         ==================== */
/* ======================================================================= */

/* =====================================================================
Midi event task main routine
--------------------------------------------------------------------- */

void midi_event_task( void )
{
    // Wait until previous event handled by other tasks
    if (midi_event.status != NO_EVENT)
        return;
    
    // If next midi event not ready, nothing to do
    if (!get_midi_event())
        return;

    // New midi event got:
    // divide it to synth or drum or drop, based on the channel
    if (midi_event.chn == settings.drum_ch)
        midi_event.status = DRUM_EVENT;
    else if (settings.synth_ch == MIDI_CHANNEL_OMNI)
        midi_event.status = SYNTH_EVENT;
    else if (midi_event.chn == settings.synth_ch)
        midi_event.status = SYNTH_EVENT;
    else
        midi_event.status = NO_EVENT;
        // this event was on a channel we are not listening
        //  -> drop the event

    // Blink midi led
    MIDI_LED_ON;
    midi_led_on = true;
    start_timer5(TIME_30MS);
}

/* =======================================================================
Get next midi event. Parse it from midi_msg -> midi_event.
Return true if supported message type.
----------------------------------------------------------------------- */

bool get_midi_event( void )
{
    // If no complete midi message got, nothing to do
    if (!get_midi_message())
        return false;
    
    // New message in midi_msg[]: parse it
    
    midi_event.type = midi_msg[0] & 0xF0;
    midi_event.chn  = midi_msg[0] & 0x0F;  // channels 0...15

    switch (midi_event.type)
    {
    case MIDI_NOTE_ON:
        midi_event.note.nr = midi_msg[1];
        midi_event.note.vel = midi_msg[2];

        // NOTE ON with velocity 0 -> NOTE OFF
        if (midi_event.note.vel == 0)
            midi_event.type = MIDI_NOTE_OFF;
        break;

    case MIDI_NOTE_OFF:
        midi_event.note.nr = midi_msg[1];
        midi_event.note.vel = midi_msg[2];
        break;

    case MIDI_CTRL_CHANGE:
        midi_event.ctrl.nr = midi_msg[1];
        midi_event.ctrl.val = midi_msg[2];
        break;

    case MIDI_PROG_CHANGE:
        midi_event.prog.nr = midi_msg[1];
        break;

    case MIDI_PITCH_BEND:
        // scale bend to -256...+256
        midi_event.bend.val = ((midi_msg[2] << 2) | (midi_msg[1] >> 5)) - 256;
        //midi_event.bend.val = ((midi_msg[2] << 7) | midi_msg[1]) - 8192;
        break;

    default:
        return false;
    }

    return true;
}

/* =====================================================================
Parse next midi message from midi_buf -> midi_msg
Return true, when a complete message is in midi_msg[]
--------------------------------------------------------------------- */

bool get_midi_message( void )
{
    static uint8_t byte_cnt = 0;
    static uint8_t msg_idx = 0;
    static uint8_t prev_hdr = 0;
    static bool in_sysex = false;
    uint8_t midi_byte = NO_MIDI_BYTE;
    
    // Check if new midi bytes received; if yes, get one byte
    // This is enclosed in __critical, because serial interrupt handles the same variables
    __critical {
        if ((midi_buf_bot != midi_buf_top) || midi_buf_full)
        {
            // Consume one byte from ring buffer
            midi_byte = *midi_buf_bot;
            CYCLIC_INC_MIDI(midi_buf_bot);
            midi_buf_full = false;
        }
    }
    
    // If no new byte, nothing to do
    if (midi_byte == NO_MIDI_BYTE) 
        return false;

    // Is this a header byte?
    if ((midi_byte & 0x80) != 0)
    {
        if (midi_byte == MIDI_SYSEX_START)
            in_sysex = true;
        else
        {   
            byte_cnt = get_midi_length(midi_byte);
            msg_idx = 0;
            prev_hdr = midi_byte;
        }
    }

    if (in_sysex)
    {
        // we are inside sysex, ignore everything until sysex end byte
        if (midi_byte == MIDI_SYSEX_END)
            in_sysex = false;
    }
    else
    {
        // Handle running status by filling in the previous header
        if (byte_cnt == 0) 
        {   
            byte_cnt = get_midi_length(prev_hdr) - 1;
            midi_msg[0] = prev_hdr;
            msg_idx = 1;
        }

        // Store the byte 
        midi_msg[msg_idx] = midi_byte;
        msg_idx++;
        byte_cnt--;
        
        if (byte_cnt == 0)
            return true;    // message ready
    }
    
    return false;
}

/* =====================================================================
Retrun length of midi message in bytes for given header byte
--------------------------------------------------------------------- */

uint8_t get_midi_length( uint8_t hdr )
{
    uint8_t hi_nib = hdr >> 4;

    if (hi_nib == 0xF)
    {
        // system message
        switch (hdr & 0x0F)
        {
        case 1:  return 2; break;
        case 2:  return 3; break;
        case 3:  return 2; break;
        default: return 1; break;
        }
    }
    else  // normal message
        return midi_length_lut[hi_nib & 0x07];
}

/* ======================================================================= */
/* =====================      REAL TIME ROUTINES      ==================== */
/* ======================================================================= */

/* =====================================================================
Delay with busy loop
--------------------------------------------------------------------- */

void delayloop( int ms )
{
    int i,j;
    
    for (i = 0; i < ms; i++)
        for (j = 0; j < 158; j++)
            ;
}

/* =====================================================================
Get current timestamp
--------------------------------------------------------------------- */

uint32_t cur_timestamp( void )
{
    uint32_t tc;
    
	__critical { tc = tick_ctr; }
    
    return tc;
}

/* =======================================================================
Start timer 1
----------------------------------------------------------------------- */

void start_timer1( uint8_t delay )
{
    __critical { 
        timeout1 = false;
        timer1 = delay;
    }
}

/* =======================================================================
Start timer 2
----------------------------------------------------------------------- */

void start_timer2( uint8_t delay )
{
    __critical { 
        timeout2 = false;
        timer2 = delay;
    }
}

/* =======================================================================
Start timer 3
----------------------------------------------------------------------- */

void start_timer3( uint8_t delay )
{
    __critical { 
        timeout3 = false;
        timer3 = delay;
    }
}

/* =======================================================================
Start timer 4
----------------------------------------------------------------------- */

void start_timer4( uint8_t delay )
{
    __critical { 
        timeout4 = false;
        timer4 = delay;
    }
}

/* =======================================================================
Start timer 5
----------------------------------------------------------------------- */

void start_timer5( uint8_t delay )
{
    __critical { 
        timeout5 = false;
        timer5 = delay;
    }
}

/* =====================================================================
Interrupt handler. Handles three interrupts:
    - RTC from YM3806, 10 ms interval
    - serial tx from UART
    - serial rx from UART
--------------------------------------------------------------------- */

void intser(void) __critical __interrupt(0)
{
	char ch;
    uint8_t midi_byte;
    
    // -------- RTC ---------------
    if ((OPQ_status & 0x05) != 0)
    {
        OPQ_timer = 0x71;
        tick_ctr++;
        
        timer1--;
        if (timer1 == 0)
            timeout1 = true;

        timer2--;
        if (timer2 == 0)
            timeout2 = true;

        timer3--;
        if (timer3 == 0)
            timeout3 = true;

        timer4--;
        if (timer4 == 0)
            timeout4 = true;

        timer5--;
        if (timer5 == 0)
            timeout5 = true;
    }
    
    // ----- UART ------------
    if ((UART_status & 0x80) != 0)
    {
        // --- UART send (serial debug)
        if ((UART_status & 0x02) != 0)
        {   
            // Anything to send?
            if (outbuf_bot != outbuf_top || outbuf_full)
            {
                ch = *outbuf_bot;
                CYCLIC_INC_SERIAL(outbuf_bot);
                outbuf_full = false;

                UART_data = ch;             // Transmit the character
            }
            else
                UART_control = 0x95;        // Suppress interrupt when buffer empty
        }
        
        // --- UART receive (midi)
        if ((UART_status & 0x01) != 0)
        {
            midi_byte = UART_data;

            // If midi buffer full, cannot do anything
            if (!midi_buf_full)
            {
                // Store the byte in the buffer
                *midi_buf_top = midi_byte;
                
                // Update buffer top
                CYCLIC_INC_MIDI(midi_buf_top);
                if (midi_buf_top == midi_buf_bot)
                    midi_buf_full = true;
            }
        }
    }
}

/* =======================================================================
Output one char to tx buffer
----------------------------------------------------------------------- */

int putchar(int ch)
{
    __critical {
        if (!outbuf_full)
        {
            *outbuf_top = ch;

            CYCLIC_INC_SERIAL(outbuf_top);
            if (outbuf_bot == outbuf_top)
                outbuf_full = true;

            UART_control = 0xB5;        // Enable tx int
        }
    }
    
    return 0;
}

/* =======================================================================
Output one midi message to midi buffer. Used when midi is coming 
through parcom.
----------------------------------------------------------------------- */

void put_midi_msg( uint8_t *midimsg, uint8_t buflen )
{
    uint8_t i;

    __critical {
        
        for (i = 0; i < buflen; i++)
        {
            if (!midi_buf_full)
            {
                *midi_buf_top = *midimsg++;
                
                // Update buffer top
                CYCLIC_INC_MIDI(midi_buf_top);
                if (midi_buf_top == midi_buf_bot)
                    midi_buf_full = true;
            }
        }
    }
}

/* =====================================================================
Hardware init
--------------------------------------------------------------------- */

void init_ppi( void )
{
    PPI_control = 0x92;     // 8255 control: mode 0, PA=in, PB=in PC=out
    
    // This sequence of sound chip resets and mute tries to minimize the 
    // click during reset. The original sequence from PSR-70 does not work
    // because hardware has been changed.
    PPI_PC = 0xA8;          // PC7: mute -> on (high)
                            // PC6: sound chip resets -> off (low)
                            // PC5: midi led -> off (high)
                            // PC4: ROM2 bank -> lower bank (low)
                            // PC3: sanity led -> off (high)
                            // PC2: test point -> off (low)
                            // PC1: RDR -> off (low)
                            // PC0: WACK -> off (low)
    delayloop(100);
    PPI_PC = 0xE8;          // PC6: sound chip resets -> on (high)
    delayloop(20);
    PPI_PC = 0xA8;          // PC6: sound chip resets -> off (low)
    delayloop(50);
    PPI_PC = 0x28;          // PC7: mute -> off (low)
    delayloop(20);
}

/* ------------------------------------------------------------------ */

void init_uart( void )
{
    UART_control = 0x00;
    UART_control = 0x00;
    UART_control = 0x00;
    UART_control = 0x03;
    delayloop(10);
    UART_control = 0x95;
}

/* ------------------------------------------------------------------ */

void init_hw( void )
{
    init_ppi();
    init_uart();
    
    OPQ_timer = 0x71;   // start 10 ms RTC timer
}

/* ============================ EOF ====================================== */
