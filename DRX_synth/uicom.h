/******************************************************************************

  DRX-70 FM synth
  ===============

  Communication with UI
  
  SDCC Z80

  17.3.2021

*****************************************************************************/

#ifndef UICOM_H
#define UICOM_H

void uicom_task( void );
void init_uicom( void );

void request_patch_from_ui( uint16_t patch_nr );
void request_drumkit_from_ui( uint8_t drumkit_nr );

#endif  // UICOM_H
