/***************************************************************************

 ui.h

 LCD/encoder based UI driver for DRX-70.

 15.5.2021  - First version

 ***************************************************************************/

#ifndef UI_H
#define UI_H

void ui_init( void );
void ui_task( void );
void button_task( void );

void enc_pf_a_isr(void) __interrupt(INTERRUPT_INT0) __using (1);
void enc_val_a_isr(void) __interrupt(INTERRUPT_INT1) __using (1);
void lcd_isr(void) __interrupt(TF0_VECTOR) __using (1);

#endif  // UI_H
