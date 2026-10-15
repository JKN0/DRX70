/******************************************************************************

  PSR-70 I/O

  10.2.2020

*****************************************************************************/

// UART (HD6350)
__sfr __at 0x10 UART_control;
__sfr __at 0x10 UART_status;
__sfr __at 0x11 UART_data;

// PPI (82C55)
__sfr __at 0x20 PPI_PA;
__sfr __at 0x21 PPI_PB;
__sfr __at 0x22 PPI_PC;
__sfr __at 0x23 PPI_control;

// RYP4 (YM2154)
__sfr __at 0x8E RYP4_int_ack;

// OPQ (YM3806)
__at 0xC000 unsigned char OPQ_status;
__at 0xC003 unsigned char OPQ_timer;

// Test points and LEDs
#define PPI_PC2_SET          0x05   // set PC2
#define PPI_PC2_RESET        0x04   // reset PC2
#define PPI_PC3_SET          0x07   // set PC3
#define PPI_PC3_RESET        0x06   // reset PC3
#define PPI_PC5_SET          0x0B   // set PC5
#define PPI_PC5_RESET        0x0A   // reset PC5

#define TP_SET               PPI_control = PPI_PC2_SET
#define TP_RESET             PPI_control = PPI_PC2_RESET
#define SANITY_LED_OFF       PPI_control = PPI_PC3_SET
#define SANITY_LED_ON        PPI_control = PPI_PC3_RESET
#define MIDI_LED_OFF         PPI_control = PPI_PC5_SET
#define MIDI_LED_ON          PPI_control = PPI_PC5_RESET
#define TP2_SET              PPI_control = PPI_PC5_SET      // Note: midi led used as TP2 !
#define TP2_RESET            PPI_control = PPI_PC5_RESET

