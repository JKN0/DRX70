/***************************************************************************

 patchman.h

 Patch manager for DRX-70.

 9.6.2021  - First version

 ***************************************************************************/

#ifndef PATCHMAN_H
#define PATCHMAN_H

#define SAVE_IDLE       0
#define SAVE_PATCH      1
#define SAVE_DRUMKIT    2
#define SAVE_SETTINGS   3
#define SAVE_ACTIVE1    4
#define SAVE_ACTIVE2    5
#define SAVE_DONE       6

#define MAX_PATCH       447
#define MAX_DRUMKIT     30

extern IDATA BYTE saving_state;
extern IDATA int cur_save_pos;

extern BIT reload_patch;
extern BIT reload_drumkit;
extern BIT reload_settings;

void patch_man_init( void );
void patch_man_task( void );
void button_task( void );
void parcom_data_write( XDATA BYTE *buf );
void parcom_data_read( XDATA BYTE *buf );
BIT i2c_active( void );

void i2c_isr(void) __interrupt(SMB0_VECTOR) __using (1);
void scl_timeout_isr(void) __interrupt(TF3_VECTOR) __using (1);

#endif  // PATCHMAN_H
