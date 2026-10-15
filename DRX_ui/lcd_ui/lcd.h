/***************************************************************************

 lcd.h

 CC40040B LCD display driver

 4*40 chars alphanumeric, 2*HD44780

 10.5.2021  - First version

 ***************************************************************************/

#ifndef LCD_H
#define LCD_H

#define LCDC_LOWER           1
#define LCDC_UPPER           2
#define LCDC_BOTH            (LCDC_LOWER | LCDC_UPPER)

#define FOCUS_OFF            0
#define FOCUS_ON             1

// Commands
#define LCD_CMD_CLEAR        0x01

void LCD_init( void );
void LCD_load_custom_chars( BYTE lcdc ) __reentrant;
void LCD_write_char( BYTE row, BYTE col, BYTE ch ) __reentrant;
void LCD_write_text( BYTE row, BYTE col, BYTE *text ) __reentrant;
void LCD_field_text( BYTE row, BYTE col, BYTE *text ) __reentrant;
void LCD_field_int1( BYTE row, BYTE col, BYTE val ) __reentrant;
void LCD_field_int2( BYTE row, BYTE col, BYTE val ) __reentrant;
void LCD_field_signed2( BYTE row, BYTE col, INT8 val ) __reentrant;
void LCD_field_hex3( BYTE row, BYTE col, WORD val ) __reentrant;
void LCD_field_focus( BYTE row, BYTE col, BYTE width, BIT focus_on ) __reentrant;
void LCD_write_pagenr( BYTE pgnr, BIT focus ) __reentrant;
void LCD_write_char_cursor( BYTE row, BYTE col, BYTE ch ) __reentrant;
void LCD_hide_cursor( BYTE lcdc ) __reentrant;
void LCD_command( BYTE lcdc, BYTE cmd ) __reentrant;
void LCD_delay( BYTE dlen ) __reentrant;
BIT LCD_buffer_has_room( void );

#endif  // LCD_H
