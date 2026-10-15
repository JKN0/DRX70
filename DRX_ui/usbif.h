/***************************************************************************

 usbif.h

 USB midi interface for DRX-70.

 13.12.2021  - First version

 ***************************************************************************/

#ifndef USBIF_H
#define USBIF_H

#define SAVE_IDLE       0

extern IDATA BYTE saving_state;

void usb_if_init( void );
void usb_if_task( void );

#endif  // USBIF_H
