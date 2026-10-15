/*-------------------------------------------------------------------------

portdefs.h

Define I/O-port names (Px_y).
Include compiler_defs.h first.

JK 19.7.2013

-------------------------------------------------------------------------*/

#ifndef PORTDEFS_H
#define PORTDEFS_H

#ifdef C8051F300_DEFS_H
#define INC_P0
#endif

#ifdef C8051F310_DEFS_H
#define INC_P0
#define INC_P1
#define INC_P2
#define INC_P3_LO
#endif

#ifdef C8051F340_DEFS_H
#define INC_P0
#define INC_P1
#define INC_P2
#define INC_P3

// P4 is not bit-addressable

#endif

#ifdef C8051F360_DEFS_H
#define INC_P0
#define INC_P1
#define INC_P2
#define INC_P3_LO
#endif

#ifdef C8051F800_DEFS_H
#define INC_P0
#define INC_P1_LO
#endif

#if defined (INC_P0)
SBIT (P0_0,SFR_P0,0);
SBIT (P0_1,SFR_P0,1);
SBIT (P0_2,SFR_P0,2);
SBIT (P0_3,SFR_P0,3);
SBIT (P0_4,SFR_P0,4);
SBIT (P0_5,SFR_P0,5);
SBIT (P0_6,SFR_P0,6);
SBIT (P0_7,SFR_P0,7);
#endif

#if defined (INC_P1_LO) || defined (INC_P1)
SBIT (P1_0,SFR_P1,0);
SBIT (P1_1,SFR_P1,1);
SBIT (P1_2,SFR_P1,2);
SBIT (P1_3,SFR_P1,3);
#endif

#if defined (INC_P1_HI) || defined (INC_P1)
SBIT (P1_4,SFR_P1,4);
SBIT (P1_5,SFR_P1,5);
SBIT (P1_6,SFR_P1,6);
SBIT (P1_7,SFR_P1,7);
#endif

#if defined (INC_P2)
SBIT (P2_0,SFR_P2,0);
SBIT (P2_1,SFR_P2,1);
SBIT (P2_2,SFR_P2,2);
SBIT (P2_3,SFR_P2,3);
SBIT (P2_4,SFR_P2,4);
SBIT (P2_5,SFR_P2,5);
SBIT (P2_6,SFR_P2,6);
SBIT (P2_7,SFR_P2,7);
#endif

#if defined (INC_P3_LO) || defined (INC_P3)
SBIT (P3_0,SFR_P3,0);
SBIT (P3_1,SFR_P3,1);
SBIT (P3_2,SFR_P3,2);
SBIT (P3_3,SFR_P3,3);
SBIT (P3_4,SFR_P3,4);
#endif

#if defined (INC_P3_HI) || defined (INC_P3)
SBIT (P3_5,SFR_P3,5);
SBIT (P3_6,SFR_P3,6);
SBIT (P3_7,SFR_P3,7);
#endif

#if defined (INC_P4)
SBIT (P4_0,SFR_P4,0);
SBIT (P4_1,SFR_P4,1);
SBIT (P4_2,SFR_P4,2);
SBIT (P4_3,SFR_P4,3);
SBIT (P4_4,SFR_P4,4);
SBIT (P4_5,SFR_P4,5);
SBIT (P4_6,SFR_P4,6);
SBIT (P4_7,SFR_P4,7);
#endif

#endif  // ifndef PORTDEFS_H
