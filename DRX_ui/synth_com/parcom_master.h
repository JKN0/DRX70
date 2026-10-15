/***************************************************************************
 
 parcom_master.h
 
 Master for DRX-70 internal parallel port communication
 
 5.9.2021

 ***************************************************************************/

/* --------------------------------------------------------------------------
 
 Usage from main program:
 
 - #include "parcom_slave.h"
 - call PC_init()
 - main loop: call PC_run_tasks()
 
 - sending data to master (when master has requested):
    - PC_send_to_slave()
    - sending is ready, when PC_comm_busy() returns false
 - receiving data from master:
    - command reading with PC_send_to_slave()
    - data available, when PC_get_rx_buf() returns != NULL
    - data is in the returned buffer
    - buffer is acknowledged by calling PC_ack_rx()

 ---------------------------------------------------------------------------- */

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#ifndef _PARCOM_MASTER_H_
#define _PARCOM_MASTER_H_

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#define MSGBUF_LEN          40      // ring buffer len in structs (40*4 bytes)
#define PCBUF_LEN           150

#define RTC_TIME            5
#define TX_TIMEOUT          (200/RTC_TIME)
#define RX_TIMEOUT          (200/RTC_TIME)

extern BIT pc_error;

/* =======================================================================
------------------------ I/O ------------------------------------------ */

#define WDR                  P1_3       // Write Data Ready (out)
#define WACK                 P1_1       // Write Ack        (in)
#define RDR                  P1_2       // Read Data Ready  (in)
#define RACK                 P1_4       // Read Ack         (out)

#define CMD_WRITE            0x00
#define CMD_READ             0x80
#define CMD_NONE             0xFF

/* =======================================================================
----------------------------------------------------------------------- */

void PC_init( void );
void PC_send_patch_to_synth( void ) __reentrant;
void PC_send_drumkit_to_synth( void ) __reentrant;
void PC_send_settings_to_synth( void ) __reentrant;
void PC_send_byte_param_to_synth( BYTE scmd, BYTE op_inst, BYTE parval ) __reentrant;
void PC_send_word_param_to_synth( BYTE scmd, BYTE op_inst, WORD parval ) __reentrant;
void PC_send_midi_msg_to_synth( XDATA BYTE *midibuf, BYTE len ) __reentrant;
BIT PC_comm_busy( void );
XDATA BYTE *PC_get_rx_buf( BYTE *len );
void PC_ack_rx( void );
void PC_update_timeouts( void )  __using (1);
void PC_run_tasks( void );

#endif

/* ============================ EOF ====================================== */
