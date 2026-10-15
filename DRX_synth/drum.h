/******************************************************************************

  DRX-70 FM synth
  ===============

  RYP4 chip controlling module
  
  SDCC Z80

  17.3.2021

*****************************************************************************/

#ifndef DRUM_H
#define DRUM_H

typedef struct inst_data_t {
    uint8_t group; 
    bool changed;
    uint8_t pan;
    uint8_t ss1;
    uint8_t ss2;
    uint8_t ss3;
} INST_DATA_T;

extern INST_DATA_T inst_data[];

void drum_play_task( void );
void init_drum( void );

void activate_drumkit( void );
void update_pan( uint8_t instr );
void update_ss_bytes( uint8_t instr );
void update_chorus( void );

#endif  // DRUM_H
