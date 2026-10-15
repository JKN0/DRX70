 /***************************************************************************
 
 parcom_slave.c
 
 Slave for DRX-70 internal parallel port communication
 
 28.8.2021

 ***************************************************************************/

/* --------------------------------------------------------------------------
 
 Usage from main program:
 
 - #include "parcom_slave.h"
 - call PC_init()
 - main loop: call PC_run_tasks()
 
 - sending data to master (when master has requested):
    - PC_send_to_master()
    - sending is ready, when PC_comm_busy() returns false
 - receiving data from master:
    - data available, when PC_get_rx_buf() returns != NULL
    - data is in the returned buffer
    - buffer is acknowledged by calling PC_ack_rx()
 
 ---------------------------------------------------------------------------- */

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#ifndef _PARCOM_SLAVE_H_
#define _PARCOM_SLAVE_H_

#define PCBUF_LEN           130

#define TX_TIMEOUT          (200/RTC_TIME)
#define RX_TIMEOUT          (200/RTC_TIME)

extern bool pc_error;

/* =======================================================================
------------------------ I/O ------------------------------------------ */

// PPI modes
#define PPI_MODE_B_IN        0x92   // A in, B in, C out
#define PPI_MODE_B_OUT       0x90   // A in, B out, C out
        
// PPI port A masks
#define WDR_MASK             0x01 
#define RACK_MASK            0x02

#define WDR_ON               (PPI_PA & WDR_MASK)
#define WDR_OFF              (!(PPI_PA & WDR_MASK))
#define RACK_ON              (PPI_PA & RACK_MASK)
#define RACK_OFF             (!(PPI_PA & RACK_MASK))

// PPI port C set/reset commands for WACK/RDR lines
#define PPI_PC0_SET          0x01   // set PC0
#define PPI_PC0_RESET        0x00   // reset PC0
#define PPI_PC1_SET          0x03   // set PC1
#define PPI_PC1_RESET        0x02   // reset PC1

#define WACK_SET             PPI_control = PPI_PC0_SET
#define WACK_RESET           PPI_control = PPI_PC0_RESET
#define RDR_SET              PPI_control = PPI_PC1_SET
#define RDR_RESET            PPI_control = PPI_PC1_RESET

// Command types
#define CMD_WRITE            0x00
#define CMD_READ             0x80
#define CMD_NONE             0xFF

// Send types
#define SEND_NONE            0
#define SEND_WITH_REQ        1
#define SEND_NOW             2

/* =======================================================================
----------------------------------------------------------------------- */

void PC_init( void );
void PC_send_to_master( uint8_t stype, uint8_t *buf, uint8_t buflen );
bool PC_comm_busy( void );
uint8_t *PC_get_rx_buf( uint8_t *len, uint8_t *cmd );
void PC_ack_rx( void );
void PC_run_tasks( void );

#endif

/* ============================ EOF ====================================== */
