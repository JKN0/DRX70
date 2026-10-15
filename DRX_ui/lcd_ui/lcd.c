/***************************************************************************

 lcd.c

 CC40040B LCD display driver

 4*40 chars alphanumeric, 2*HD44780

 10.5.2021  - First version

 ***************************************************************************/

/* --------------------------------------------------------------------------

CC40040B display contains two HD44780's, one for upper two rows (2*40 chars)
one for lower two. RS and R/W are common, E-pins are separate.

HD44780 executes commands slowly: most commands take about 40 us and some
1.6 ms. To avoid busy waiting, this module implements interrupt based
writing to LCD: timer 0 interrupts at 100 us intervals and writes
one byte to HD44780 at each interrupt.

Messages are written to ring buffer lcdbuf without timing limitations,
interrupt service picks them up and sends to LCD with proper intervals.

Messages in lcdbuf are formatted with two byte header and 0...63 bytes
of data:

  Byte 1: header  DEnn nnnn
    D = delay
        0 = normal LCD command
        1 = delay only, don't send anything to LCD
    E = which of the HD44780's this command is intended?
        0 = upper half controller
        1 = lower half controller
    nnnnnn = data length, 0...63 bytes

  Byte 2: command code / delay length
    For normal commands, this is the command code to send to the HD44780
    For delay, this is the delay length in 100 us steps, max delay 25.5 ms

  Bytes 3...65:
    Data to be sent to HD44780

There is no need to use explicit delay messages normally, this driver handles
needed delays automatically. These are used only for extra delays, see LCD_init().


---------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <datatypes.h>
#include <compiler_defs.h>
#include <C8051F340_defs.h>
#include "../portdefs.h"
#include "../silabs_gen.h"
#include "lcd.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

/* --- CPU timer constants */
/* SYSCLK = 48 MHz, cycle 20 ns
   Tmr0 counts SYSCLK/12 = 4 MHz, 100 us interrupt rate => reload -400  */
#define TMR0_RELOAD         (WORD)(-400)

// Delays required by HD44780, one NOP = 20 ns
#define DELAY_40NS           __asm__ ("  nop\n  nop\n")
#define DELAY_100NS          __asm__ ("  nop\n  nop\n  nop\n  nop\n  nop\n")
#define DELAY_240NS          __asm__ ("  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n  nop\n")

#define UPPER                0
#define LOWER                1

#define LCDBUF_LEN           500

#define TO_HEX(d)            ((d|0x30) + (d>9 ? 7 : 0))

/* =======================================================================
------------------------ I/O ------------------------------------------ */

// LCD interface
// R/W tied to GND
#define LCD_RS               P3_6       // RS, common for halves
#define LCD_E_UP             P3_7       // E for upper half
#define LCD_E_LO             P3_5       // E for lower half
#define LCD_DATA             P4

// RS values
#define CMD                  0
#define DAT                  1

#define TP1                  P3_4
#define TP2                  P3_3
#define TP3                  P3_2
#define TP4                  P1_7

/* =====================================================================
------------------------ Structures --------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

XDATA BYTE lcdbuf[LCDBUF_LEN];
XDATA BYTE *lcdbuf_top = lcdbuf;
XDATA BYTE *lcdbuf_bot = lcdbuf;
BIT lcdbuf_full = FALSE;

BYTE delay_ctr = 0;
BYTE data_len = 0;
BIT lcd_controller = UPPER;

extern BIT scrolling_bank;

// Custom characters used in drawing the algorithm schemas
CODE BYTE custom_chars[] =
{
    0x00, 0x00, 0x00, 0x07, 0x04, 0x04, 0x04, 0x00,  // 0: GC_CORNER_LU (,-)
    0x00, 0x00, 0x00, 0x1c, 0x04, 0x04, 0x04, 0x00,  // 1: GC_CORNER_RU (-,)
    0x04, 0x04, 0x04, 0x1c, 0x00, 0x00, 0x00, 0x00,  // 2: GC_CORNER_RD (-')
    0x04, 0x04, 0x04, 0x07, 0x00, 0x00, 0x00, 0x00,  // 3: GC_CORNER_LD (L)
    0x04, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x04, 0x00,  // 4: GC_CROSS (+)
    0x00, 0x07, 0x08, 0x08, 0x07, 0x00, 0x00, 0x00,  // 5: GC_FB (c)
    0x04, 0x04, 0x04, 0x1f, 0x00, 0x00, 0x00, 0x00,  // 6: GC_INV_T (-'-)
};


/* =====================================================================
------------------------ Function prototypes ------------------------ */

void write_byte_to_upper_lcd( BIT rs, BYTE dat ) __using (1);
void write_byte_to_lower_lcd( BIT rs, BYTE dat ) __using (1);
BIT put_lcdbuf( BYTE dat );
BYTE get_lcdbuf( void ) __using (1);

// Get rid of Evelyn
#pragma disable_warning 110

/* ======================================================================= */
/* ========================       DRIVER API     ========================= */
/* ======================================================================= */

/* Most of the functions are declared __reentrant, not because they need 
   be re-entrant, but because SDCC does not want to overlay the static
   call parameters when the function calls another function (in this case, 
   put_lcdbuf()). Declaring __reentrant saves significant amount of internal
   RAM, which is always scarce in 8051.
*/

/* =======================================================================
LCD display initialization. Initialize both HD44780's at the same time.
----------------------------------------------------------------------- */

void LCD_init( void )
{
    // Reset sequence
    LCD_delay(200);                // delay greater than 15 ms
    LCD_command(LCDC_BOTH,0x30);
    LCD_delay(50);                 // delay greater than 4.1 ms
    LCD_command(LCDC_BOTH,0x30);
    LCD_delay(3);                  // delay greater than 100 us
    LCD_command(LCDC_BOTH,0x30);

    // Initialize
    LCD_command(LCDC_BOTH,0x38);    // 8-bit, 2 lines, 5*8 font
    LCD_command(LCDC_BOTH,0x08);    // display off
    LCD_command(LCDC_BOTH,0x01);    // clear display
    LCD_command(LCDC_BOTH,0x06);    // increment ON, shift OFF
    LCD_command(LCDC_BOTH,0x0C);    // display ON, cursor OFF, blink OFF

    // Load custom characters
    LCD_load_custom_chars(LCDC_BOTH);
}

/* =======================================================================
Write custom character load commands to LCD ringbuffer. 
Write to one or both of the controllers.
----------------------------------------------------------------------- */

void LCD_load_custom_chars( BYTE lcdc ) __reentrant
{
    BYTE i;

    DISABLE(ET0);

    // Write to lower half
    if ((lcdc & LCDC_LOWER) != 0)
    {
        // Header byte: 0Enn nnnn
        if (put_lcdbuf(0x40 | sizeof(custom_chars))) goto exit;

        // Command byte: 01aa aaaa, CGRAM address = 0
        if (put_lcdbuf(0x40)) goto exit;

        for (i = 0; i < sizeof(custom_chars); i++)
            if (put_lcdbuf(custom_chars[i])) goto exit;
    }

    // Write to upper half
    if ((lcdc & LCDC_UPPER) != 0)
    {
        // Header byte: 0Enn nnnn
        if (put_lcdbuf(0x00 | sizeof(custom_chars))) goto exit;

        // Command byte: 01aa aaaa, CGRAM address = 0
        if (put_lcdbuf(0x40)) goto exit;

        for (i = 0; i < sizeof(custom_chars); i++)
            if (put_lcdbuf(custom_chars[i])) goto exit;
    }

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write one character to LCD ringbuffer. Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_write_char( BYTE row, BYTE col, BYTE ch ) __reentrant
{
    // Header byte: D=0, E=from row, datalen=1
    if (put_lcdbuf(((row << 5) & 0x40) | 1)) goto exit;

    // Command byte: 1aaa aaaa, DDRAM address from col
    if (put_lcdbuf(((row << 6) & 0x40) | col | 0x80)) goto exit;

    // Data byte
    if (put_lcdbuf(ch)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write text to LCD ringbuffer. Text ends to '\0'. Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_write_text( BYTE row, BYTE col, BYTE *text ) __reentrant
{
    BYTE len;
    BYTE *p;

    len = strlen(text);

    DISABLE(ET0);

    // Row 0 => upper,addr=00H...27H
    // Row 1 => upper,addr=40H...67H
    // Row 2 => lower,addr=00H...27H
    // Row 3 => lower,addr=40H...67H

    // Header byte: D=0, E=from row, datalen=text len
    if (put_lcdbuf(((row << 5) & 0x40) | len)) goto exit;

     // Command byte: 1aaa aaaa, DDRAM address from row+col
    if (put_lcdbuf(((row << 6) & 0x40) | col | 0x80)) goto exit;

    // Data bytes
    for (p = text; *p != '\0'; p++)
        if (put_lcdbuf(*p)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write text field to LCD ringbuffer. Text ends to '\0'.
Row = 0...3, col = 0...39.
This (as all LCD_field_xxxx()'s do) reserves one character position for
the focus marker [ to the left side of the field.
----------------------------------------------------------------------- */

void LCD_field_text( BYTE row, BYTE col, BYTE *text ) __reentrant
{
   LCD_write_text(row,col+1,text);
}

/* =======================================================================
Write 1-digit integer field to LCD ringbuffer. Value 0...9.
Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_field_int1( BYTE row, BYTE col, BYTE val ) __reentrant
{
    val |= 0x30;

    DISABLE(ET0);

    // Header byte: 0Enn nnnn, datalen=1
    if (put_lcdbuf(((row << 5) & 0x40) | 1)) goto exit;

     // Command byte: 1aaa aaaa
    if (put_lcdbuf(((row << 6) & 0x40) | (col+1) | 0x80)) goto exit;

    // Data bytes
    if (put_lcdbuf(val)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write 2-digit integer field to LCD ringbuffer. Value 0...99.
Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_field_int2( BYTE row, BYTE col, BYTE val ) __reentrant
{
    BYTE msd,lsd;

    lsd = (val % (BYTE)10) | 0x30;
    msd = (val / (BYTE)10) | 0x30;
    if (msd == '0') msd = ' ';

    DISABLE(ET0);

    // Header byte: 0Enn nnnn, datalen=2
    if (put_lcdbuf(((row << 5) & 0x40) | 2)) goto exit;

     // Command byte: 1aaa aaaa
    if (put_lcdbuf(((row << 6) & 0x40) | (col+1) | 0x80)) goto exit;

    // Data bytes
    if (put_lcdbuf(msd)) goto exit;
    if (put_lcdbuf(lsd)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write signed 2-digit integer field to LCD ringbuffer. Value -99...+99,
always with sign. Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_field_signed2( BYTE row, BYTE col, INT8 val ) __reentrant
{
    BYTE msd,lsd,sign;

    if (val == 0)
        sign = ' ';
    else if (val < 0)
    {
        sign = '-';
        val = -val;
    }
    else
        sign = '+';

    lsd = (val % (BYTE)10) | 0x30;
    msd = (val / (BYTE)10) | 0x30;
    if (msd == '0')
    {
        msd = sign;
        sign = ' ';
    }

    DISABLE(ET0);

    // Header byte: 0Enn nnnn, datalen=3
    if (put_lcdbuf(((row << 5) & 0x40) | 3)) goto exit;

     // Command byte: 1aaa aaaa
    if (put_lcdbuf(((row << 6) & 0x40) | (col+1) | 0x80)) goto exit;

    // Data bytes
    if (put_lcdbuf(sign)) goto exit;
    if (put_lcdbuf(msd)) goto exit;
    if (put_lcdbuf(lsd)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write integer field as 3 hex digits to LCD ringbuffer. Value 0...FFFH.
Row = 0...3, col = 0...39.
----------------------------------------------------------------------- */

void LCD_field_hex3( BYTE row, BYTE col, WORD val ) __reentrant
{
    BYTE digit;

    DISABLE(ET0);

    // Header byte: 0Enn nnnn, datalen=3
    if (put_lcdbuf(((row << 5) & 0x40) | 3)) goto exit;

     // Command byte: 1aaa aaaa
    if (put_lcdbuf(((row << 6) & 0x40) | (col+1) | 0x80)) goto exit;

    // Data bytes
    digit = (val >> 8) & 0x0F;
    if (put_lcdbuf(TO_HEX(digit))) goto exit;
    digit = (val >> 4) & 0x0F;
    if (put_lcdbuf(TO_HEX(digit))) goto exit;
    digit = val & 0x0F;
    if (put_lcdbuf(TO_HEX(digit))) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Set/remove focus marker [] to field. Row = 0...3, col = 0...39.
width = width of the field data.
focus: TRUE=set markers, FALSE=remove markers.
----------------------------------------------------------------------- */

void LCD_field_focus( BYTE row, BYTE col, BYTE width, BIT focus ) __reentrant
{
    DISABLE(ET0);

    // '['
    if (put_lcdbuf(((row << 5) & 0x40) | 1)) goto exit;
    if (put_lcdbuf(((row << 6) & 0x40) | col | 0x80)) goto exit;
    if (put_lcdbuf(focus ? '[' : ' ')) goto exit;

    // ']'
    if (put_lcdbuf(((row << 5) & 0x40) | 1)) goto exit;
    if (put_lcdbuf(((row << 6) & 0x40) | (col+width+1) | 0x80)) goto exit;
    if (put_lcdbuf(focus ? ']' : ' ')) goto exit;

    // Hide cursor always when losing focus
    if (!focus)
    {
        if (put_lcdbuf((row << 5) & 0x40)) goto exit;
        if (put_lcdbuf(0x0C)) goto exit;    // display ON, cursor OFF, blink OFF

        scrolling_bank = FALSE;         // ugly hack
    }

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write page nr field to LCD ringbuffer. Value 0...99.
Page number is always at (0,36) and two digits wide.
focus: TRUE=set markers, FALSE=remove markers.
----------------------------------------------------------------------- */

void LCD_write_pagenr( BYTE pgnr, BIT focus ) __reentrant
{
    BYTE msd,lsd;

    lsd = (pgnr % (BYTE)10) | 0x30;
    msd = (pgnr / (BYTE)10) | 0x30;
    if (msd == '0') msd = ' ';

    DISABLE(ET0);

    // Header byte: 0Enn nnnn, datalen=4 (includes focus markers)
    if (put_lcdbuf(0x04)) goto exit;

     // Command byte: 1aaa aaaa, address=36
    if (put_lcdbuf(0x80 | 36)) goto exit;

    // Data bytes
    if (put_lcdbuf(focus ? '>' : ' ')) goto exit;
    if (put_lcdbuf(msd)) goto exit;
    if (put_lcdbuf(lsd)) goto exit;
    if (put_lcdbuf(focus ? '<' : ' ')) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write one character to (row,col) showing cursor at that place.
If ch==0, don't write anything, just set cursor at (row,col).
----------------------------------------------------------------------- */

void LCD_write_char_cursor( BYTE row, BYTE col, BYTE ch ) __reentrant
{
    DISABLE(ET0);

    if (ch != 0)        
    {
        // --- Write char at (row,col)

        // Header byte: D=0, E=from row, datalen=1
        if (put_lcdbuf(((row << 5) & 0x40) | 1)) goto exit;

         // Command byte: 1aaa aaaa, DDRAM address from col
        if (put_lcdbuf(((row << 6) & 0x40) | col | 0x80)) goto exit;

        // Data byte
        if (put_lcdbuf(ch)) goto exit;

        // Cursor is now at (row,col+1)
    }

    // --- Return cursor to (row,col)

    // Header byte: D=0, E=from row, datalen=0
    if (put_lcdbuf((row << 5) & 0x40)) goto exit;

     // Command byte: 1aaa aaaa, DDRAM address from col
    if (put_lcdbuf(((row << 6) & 0x40) | col | 0x80)) goto exit;

    // --- Show cursor

    // Header byte: D=0, E=from row, datalen=0
    if (put_lcdbuf((row << 5) & 0x40)) goto exit;

     // Command byte: display ON, cursor ON, blink OFF
    if (put_lcdbuf(0x0E)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Hide cursor from indicated controller.
----------------------------------------------------------------------- */

void LCD_hide_cursor( BYTE lcdc ) __reentrant
{
    DISABLE(ET0);

    // Write to lower half
    if ((lcdc & LCDC_LOWER) != 0)
    {
        // Header byte: 0E00 0000
        if (put_lcdbuf(0x40)) goto exit;

        // Command byte: display ON, cursor OFF, blink OFF
        if (put_lcdbuf(0x0C)) goto exit;
    }

    // Write to upper half
    if ((lcdc & LCDC_UPPER) != 0)
    {
        // Header byte: 0E00 0000
        if (put_lcdbuf(0x00)) goto exit;

        // Command byte: display ON, cursor OFF, blink OFF
        if (put_lcdbuf(0x0C)) goto exit;
    }

    scrolling_bank = FALSE;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write command to LCD ringbuffer. Write to one or both of the controllers.
----------------------------------------------------------------------- */

void LCD_command( BYTE lcdc, BYTE cmd ) __reentrant
{
    DISABLE(ET0);

    // Write to lower half
    if ((lcdc & LCDC_LOWER) != 0)
    {
        // Header byte: 0E00 0000
        if (put_lcdbuf(0x40)) goto exit;

         // Command byte: cmd
        if (put_lcdbuf(cmd)) goto exit;
    }

    // Write to upper half
    if ((lcdc & LCDC_UPPER) != 0)
    {
        // Header byte: 0E00 0000
        if (put_lcdbuf(0x00)) goto exit;

         // Command byte: cmd
        if (put_lcdbuf(cmd)) goto exit;
    }

exit:
    ENABLE(ET0);
}

/* =======================================================================
Write delay message to LCD ringbuffer.
----------------------------------------------------------------------- */

void LCD_delay( BYTE dlen ) __reentrant
{
    DISABLE(ET0);

    // Header byte: D=1, E=0, datalen=0
    if (put_lcdbuf(0x80)) goto exit;

     // Command byte: delay length
    if (put_lcdbuf(dlen)) goto exit;

exit:
    ENABLE(ET0);
}

/* =======================================================================
Return TRUE if LCD ringbuffer has enough room (over 50%).
----------------------------------------------------------------------- */

BIT LCD_buffer_has_room( void )
{
    if (lcdbuf_bot == lcdbuf_top)
        return !lcdbuf_full;

    if (lcdbuf_bot < lcdbuf_top)
        return (lcdbuf_top - lcdbuf_bot) < LCDBUF_LEN/2;
    else
        return (lcdbuf_bot - lcdbuf_top) > LCDBUF_LEN/2;
    
}

/* ======================================================================= */
/* =====================       INTERNAL FUNCTIONS     ==================== */
/* ======================================================================= */

/* =====================================================================
Put one byte of data to LCD ring buffer. Return TRUE if full.
--------------------------------------------------------------------- */

BIT put_lcdbuf( BYTE dat )
{
    *lcdbuf_top = dat;

    lcdbuf_top++;
    if (lcdbuf_top >= (lcdbuf+LCDBUF_LEN))
        lcdbuf_top = lcdbuf;

    if (lcdbuf_bot == lcdbuf_top)
        lcdbuf_full = TRUE;

    return lcdbuf_full;
}

/* =====================================================================
Get one byte of data from LCD ring buffer. Assume it is available.
Called only from lcd_isr.
--------------------------------------------------------------------- */

BYTE get_lcdbuf( void ) __using (1)
{
    BYTE dat;

    dat = *lcdbuf_bot;

    lcdbuf_bot++;
    if (lcdbuf_bot >= (lcdbuf+LCDBUF_LEN))
        lcdbuf_bot = lcdbuf;
    lcdbuf_full = FALSE;

    return dat;
}

/* =====================================================================
Write 1 byte of data to upper half LCD controller.
--------------------------------------------------------------------- */

void write_byte_to_upper_lcd( BIT rs, BYTE dat ) __using (1)
{
    LCD_RS = rs;
    DELAY_40NS;
    LCD_E_UP = 1;
    LCD_DATA = dat;
    DELAY_240NS;
    LCD_E_UP = 0;
}

/* =====================================================================
Write 1 byte of data to lower half LCD controller.
--------------------------------------------------------------------- */

void write_byte_to_lower_lcd( BIT rs, BYTE dat ) __using (1)
{
    LCD_RS = rs;
    DELAY_40NS;
    LCD_E_LO = 1;
    LCD_DATA = dat;
    DELAY_240NS;
    LCD_E_LO = 0;
}

/* ======================================================================= */
/* =====================       INTERRUPT SERVICES     ==================== */
/* ======================================================================= */

/* =======================================================================
LCD interrupt
Timer 0, 100 us interval, when enabled.
Writes one byte data/command to LCD each time. This ensures 100 us
write intervals, so no need to poll the Busy Flag.
Implements delay, if longer waits between writes needed.
----------------------------------------------------------------------- */

void lcd_isr(void) __interrupt(TF0_VECTOR) __using (1)
{
    BYTE dat;
    BYTE delay_frame;

    //TP2 = 1;
    TF0 = 0;
    TMR0 = TMR0_RELOAD;

    // If delay on, count it and nothing else
    if (delay_ctr != 0)
    {
        delay_ctr--;
        return;
    }

    // If lcd message buffer empty, suppress the timer 0 interrupts
    if ((lcdbuf_bot == lcdbuf_top) && !lcdbuf_full)
    {
        DISABLE(ET0);
        //TP2 = 0;
        return;
    }

    // Get next byte from buffer
    dat = get_lcdbuf();

    if (data_len == 0)
    {
        // Not sending data: this is the header byte of a new message
        // Extract the fields: DEnn nnnn
        delay_frame = dat & 0x80;
        lcd_controller = dat & 0x40;
        data_len = dat & 0x3F;
        //TP2 = 0;

        // Get next byte from buffer
        // assume it exists, i.e. messages are valid
        dat = get_lcdbuf();
        //TP2 = 1;

        if (delay_frame)
        {
            // Delay frame: second byte is the length
            delay_ctr = dat;
        }
        else
        {
            // Normal frame: second byte is the LCD command
            if ((dat & 0xFC) == 0)
            {
                // Clear or home: long execution time:
                // set delay counter 1.8 ms
                delay_ctr = 18;
            }

            // Write command to LCD
            if (lcd_controller == UPPER)
                write_byte_to_upper_lcd(CMD,dat);
            else
                write_byte_to_lower_lcd(CMD,dat);
        }
    }
    else
    {
        // Sending data: this is a data byte
        // Write data to LCD
        if (lcd_controller == UPPER)
            write_byte_to_upper_lcd(DAT,dat);
        else
            write_byte_to_lower_lcd(DAT,dat);

        data_len--;
    }
    //TP2 = 0;
}

/* ============================ EOF ====================================== */
