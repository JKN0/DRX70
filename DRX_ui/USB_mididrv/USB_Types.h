//-----------------------------------------------------------------------------
// Renamed to USB_Types.h by JK 4.4.2021
//-----------------------------------------------------------------------------

#ifndef USB_TYPES_H
#define USB_TYPES_H

#include <compiler_defs.h>
#include <C8051F340_defs.h>

// Compiler specific definition

#if defined(__C51__) || defined(__CX51__)

    #define KEIL
    #define BIG_ENDIAN          // high byte first

#elif defined __SDCC

    #define LITTLE_ENDIAN       // low byte first
#endif

// basic type definition
typedef unsigned char BYTE;
typedef unsigned int  UINT;
typedef unsigned long ULONG;

// WORD type definition
typedef union {UINT i; BYTE c[2];} WORD_UN;

#if defined BIG_ENDIAN

    #define MSB     0
    #define LSB     1

#else                   // little endian - low byte first

    #define MSB     1
    #define LSB     0

#endif // end of BIG_ENDIAN

#define FALSE   0
#define TRUE    1

#endif  /* USB_TYPES_H */

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
