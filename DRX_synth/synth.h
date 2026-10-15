/******************************************************************************

  DRX-70 FM synth
  ===============

  OPQ chip controlling module
  
  SDCC Z80

  17.3.2021

*****************************************************************************/

#ifndef SYNTH_H
#define SYNTH_H

void synth_play_task( void );
void vibrato_task( void );
void init_synth( void );

void activate_patch( void );
void update_lfo( void );
void update_och_fb_alg( void );
void update_ech_fm_am( void );
void update_detune( void );
void update_factor( void );
void update_atten( void );
void update_kse_attack( void );
void update_ame_wf_decay( void );
void update_sustain( void );
void update_susl_rel( void );
void update_act_ops( void );

#endif  // SYNTH_H
