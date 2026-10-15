/***************************************************************************
 
 rom2.h
 
 PSR-70 ROM2 definitions.
 
 3.3.2021 
         
 ***************************************************************************/

// Patch structure offsets

#define PSO_FEEDB_ALG       0x00
#define PSO_DETUNE          0x05
#define PSO_FACTOR          0x09
#define PSO_ATTEN_BASE      0x0D
#define PSO_KSL_LO_TBL      0x11
#define PSO_KSL_HI_TBL      0x15
#define PSO_ATTACK          0x19
#define PSO_DECAY           0x1D
#define PSO_SUSTAIN         0x21
#define PSO_SUSL_REL        0x25

// Patch pointer table in ROM2
__at 0x833A uint8_t *rpp_table[];

// Keyboard scale tables for levels (attenuate values)
__at 0x83C2 int8_t ksl_lo_tbl[13][19];
__at 0x84B9 int8_t ksl_hi_tbl[22][19];
