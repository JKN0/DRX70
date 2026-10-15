/***************************************************************************

 DRX_ui.c

 DRX-70 UI, main module for the 8051-part.

 3.10.2021  - First version, derived from UItest
 
 ***************************************************************************/

/* --------------------------------------------------------------------------

 Controller SiLabs C8051F340

 Timers:
    T0 = LCD write timer, 100 us
    T1 = I2C clock, 100 kHz
    T2 = RTC, 5 ms
    T3 = -

---------------------------------------------------------------------------- */

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
#include "lcd_ui/ui.h"
#include "synth_com/parcom_master.h"
#include "USB_Mididrv/USB_Mididrv.h"
#include "usbif.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

/* --- CPU timer constants */
/* SYSCLK = 48 MHz, cycle 20 ns, real time interrupt 5 ms
   Tmr2 counts SYSCLK/12, real time interrupt 5 ms => reload = -20000 */
#define TMR2_RELOAD         (WORD)(-20000)

#define RTC_TIME            5

#define TIME_50MS           (50/RTC_TIME)
#define TIME_100MS          (100/RTC_TIME)
#define TIME_150MS          (150/RTC_TIME)
#define TIME_200MS          (200/RTC_TIME)
#define TIME_300MS          (300/RTC_TIME)
#define TIME_500MS          (500/RTC_TIME)
#define TIME_1S             (1000/RTC_TIME)
#define TIME_3S             (3000/RTC_TIME)

#define INBUF_LEN            20
#define OUTBUF_LEN           300

#define TEXT_RAM             0
#define TEXT_ROM             1

#define OK                   0
#define FAIL                 1

#define BUTTON_NONE          0
#define BUTTON_PF_SEL        1
#define BUTTON_VAL_SEL       2
#define BUTTON_EDIT          3
#define BUTTON_IGNORE        4

#define TESTCLEAR(s,d)       if (s) {s=0; d=1;} else d=0;
#define CYCLIC_INC(p)        { p++; if (p >= (outbuf+OUTBUF_LEN)) p = outbuf; }

#define STEP_BY_ENC_PF(v)    { DISABLE(EX0); v += enc_pf_ctr; enc_pf_ctr = 0; ENABLE(EX0); }
#define STEP_BY_ENC_VAL(v)   { DISABLE(EX1); v += enc_val_ctr; enc_val_ctr = 0; ENABLE(EX1); }


#define RI1_MASK             0x01
#define TI1_MASK             0x02

#define printf               printf_fast

/* =======================================================================
------------------------ I/O ------------------------------------------ */

#define run_led              P3_0       // sanity led

#define TP1                  P3_4
#define TP2                  P3_3
#define TP3                  P3_2

#define LED_OFF              1
#define LED_ON               0

/* =====================================================================
------------------------ Structures --------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

BYTE timeout_ctr1;
BYTE timeout_ctr2;

BIT volatile timeout1;
BIT volatile timeout2;

BIT volatile rtc_flag;
BIT tick = FALSE;

BIT text_sent = FALSE;
BIT cr_received = FALSE;
BIT xmit_rom = FALSE;
BIT echo_flag = FALSE;
BIT tx_on = FALSE;

XDATA BYTE inbuf[INBUF_LEN];

XDATA BYTE outbuf[OUTBUF_LEN];
XDATA BYTE *outbuf_top = outbuf;
XDATA BYTE *outbuf_bot = outbuf;
BIT outbuf_full = FALSE;

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void ui_task( void );
BYTE ui_mode_main( BIT restart );
BYTE ui_mode_sound( BIT restart );
BYTE ui_mode_drums( BIT restart );
BYTE ui_mode_settings( BIT restart );
void button_task( void );
void set_timeout1( BYTE delay );
void set_timeout2( BYTE delay );
void init_hw( void );
void Init_Device(void);

/* =====================================================================
Main
--------------------------------------------------------------------- */

void main( void )
{
    //PCA0MD &= ~0x40;                   // WDTE = 0 (clear watchdog timer enable)

    init_hw();
    ENABLE(EA);          // Enable interrupts

    ui_init();
    patch_man_init();
    PC_init();
    usb_if_init();
    
    printf("\r\n*** DRX-70 UI ***\r\n");

    set_timeout1(TIME_50MS);
    set_timeout2(TIME_100MS);

    /* --- Main loop --- */
    while (TRUE)
    {
        // Copy the rtc_flag (set by RTC interrupt) to tick-flag in one atomic operation.
        // This makes sure the tick won't change during the main loop.
        TESTCLEAR(rtc_flag,tick);

        // Run tasks
        ui_task();
        patch_man_task();
        PC_run_tasks();
        button_task();
        usb_if_task();

        // Blink sanity led
        if (timeout1)
        {
            run_led = !run_led;
            set_timeout1(TIME_500MS);
        }

        // tick-flag valid only for one main loop iteration
        tick = FALSE;
    }
}

/* =======================================================================
Start timeout 1
----------------------------------------------------------------------- */

void set_timeout1( BYTE delay )
{
    DISABLE(ET2);
    timeout1 = FALSE;
    timeout_ctr1 = delay;
    ENABLE(ET2);
}

/* =======================================================================
Start timeout 2
----------------------------------------------------------------------- */

void set_timeout2( BYTE delay )
{
    DISABLE(ET2);
    timeout2 = FALSE;
    timeout_ctr2 = delay;
    ENABLE(ET2);
}

/* =======================================================================
putchar. printf() calls this.
----------------------------------------------------------------------- */

int putchar( int ch )
{
    DISABLE(ES);

    if (!outbuf_full)
    {
        *outbuf_top = ch;

        CYCLIC_INC(outbuf_top);
        if (outbuf_bot == outbuf_top)
            outbuf_full = TRUE;

        if (!tx_on)
        {
            tx_on = TRUE;
            SCON1 |= TI1_MASK;
        }
    }

    ENABLE(ES);

    return ch;
}

/* =======================================================================
putch_intr. putchar() to be called from the serial interrupt.
----------------------------------------------------------------------- */

void putch_intr( BYTE ch ) __using (1)
{
    if (!outbuf_full)
    {
        *outbuf_top = ch;

        CYCLIC_INC(outbuf_top);
        if (outbuf_bot == outbuf_top)
            outbuf_full = TRUE;

        if (!tx_on)
        {
            tx_on = TRUE;
            SCON1 |= TI1_MASK;
        }
    }
}

/* =======================================================================
Init hardware
----------------------------------------------------------------------- */

void init_hw(void)
{
    /* Init routine generated by ConfigWizard */
    Init_Device();

    /* Timers */
    TMR2      = TMR2_RELOAD;
    TMR2RL    = TMR2_RELOAD;
}

/* ======================================================================= */
/* =====================       INTERRUPT SERVICES     ==================== */
/* ======================================================================= */

/* =======================================================================
Real time interrupt: Timer2, 5 ms interval
- Run timeouts
----------------------------------------------------------------------- */

void rtc_isr(void) __interrupt(TF2_VECTOR) __using (1)
{
    TF2H = 0;

    // Decrement timeout counters and set the flag, if counter == 0
    timeout_ctr1--;
    if (timeout_ctr1 == 0)
        timeout1 = TRUE;

    timeout_ctr2--;
    if (timeout_ctr2 == 0)
        timeout2 = TRUE;

    PC_update_timeouts();

    // Set the rtc_flag every 5 ms
    rtc_flag = TRUE;
}

/* =======================================================================
Serial interrupt. UART1, 115200 bps
----------------------------------------------------------------------- */

void serial_isr(void) __interrupt(INTERRUPT_UART1) __using (1)
{
    static BYTE idx = 0;
    BYTE ch;

    // --- UART receive
    if (SCON1 & RI1_MASK)       // if (RI1)
    {
        SCON1 &= ~RI1_MASK;     // RI1 = 0;

        ch = SBUF1 & 0x7F;

        if (ch == '\r')                 // if CR -> line ready
        {
            inbuf[idx] = '\0';
            idx = 0;

            putch_intr('\r');              // echo CR
            putch_intr('\n');

            cr_received = TRUE;         // full line received
        }
        else if (ch == '\b' && idx > 0) // if BS -> remove char from buf
        {
            idx--;
            putch_intr('\b');              // echo BS
            putch_intr(' ');
            putch_intr('\b');
        }
        else if (ch >= ' ')             // normal char
        {
            ch |= 0x20;                 // to lowercase

            if (idx < INBUF_LEN-1)      // to buf, if fits
            {
                inbuf[idx] = ch;
                idx++;

                putch_intr(ch);          // echo
           }
        }
    }

    // --- UART send
    if (SCON1 & TI1_MASK)       // if (TI1)
    {
        SCON1 &= ~TI1_MASK;     // TI1 = 0;

        // Anything to send?
        if (outbuf_bot != outbuf_top || outbuf_full)
        {
            SBUF1 = *outbuf_bot;          // Transmit the character
            CYCLIC_INC(outbuf_bot);
            outbuf_full = FALSE;
        }
        else
            tx_on = FALSE;
    }
}

/* =======================================================================
Include generated initialization file
----------------------------------------------------------------------- */

#include "init.c"

/* =======================================================================
Disable watchdog before initializations
----------------------------------------------------------------------- */

_sdcc_external_startup()
{
    PCA0MD &= ~0x40;                   // WDTE = 0 (clear watchdog timer enable)
    return 0;
}

/* ============================ EOF ====================================== */
