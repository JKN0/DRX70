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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "DRX_synth.h"
#include "ioregs.h"
#include "parcom_slave.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

/* =====================================================================
--------------------------  Typedefs  ------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

static uint8_t pcbuf[PCBUF_LEN];
static uint8_t *rx_bufp = NULL;
static uint8_t *tx_bufp = NULL;
static uint8_t send_bytecnt = 0;
static uint8_t rcv_bytecnt = 0;
static uint8_t i2c_buf;
static uint8_t cmd_type = CMD_NONE;
static uint8_t send_type = SEND_NONE;

bool pc_error = false;

/* =====================================================================
------------------------ Function prototypes ------------------------ */

static void pc_slave_tx_task( void );
static void pc_slave_rx_task( void );

/* ======================================================================= */
/* =====================            API               ==================== */
/* ======================================================================= */

/* =======================================================================
Init parcom module
----------------------------------------------------------------------- */

void PC_init( void )
{
    start_timer2(RX_TIMEOUT);
    start_timer3(TX_TIMEOUT);
}

/* =======================================================================
Start sending to master. 
If stype=SEND_WITH_REQ, request for sending permission first
If stype=SEND_NOW start right away (as a response to query from host)
----------------------------------------------------------------------- */

void PC_send_to_master( uint8_t stype, uint8_t *buf, uint8_t buflen )
{
    buflen &= 0x7F;
    pcbuf[0] = 0x7E;
    pcbuf[1] = buflen;
    memcpy(pcbuf+2,buf,buflen);
    tx_bufp = pcbuf;
    send_bytecnt = buflen + 2;
    send_type = stype;
}

/* =======================================================================
Return true, if communicating
----------------------------------------------------------------------- */

bool PC_comm_busy( void )
{
    return send_type != SEND_NONE;
}

/* =======================================================================
Return buffer of received data or NULL, if no data.
Buffer lengeht in *len and command type (read or write) in *cmd.
----------------------------------------------------------------------- */

uint8_t *PC_get_rx_buf( uint8_t *len, uint8_t *cmd )
{
    if (rcv_bytecnt != 0)
    {   
        *len = rcv_bytecnt;
        *cmd = cmd_type;
        return pcbuf;
    }
    else    
        return NULL;
}

/* =======================================================================
Acknowledge receive buffer as free.
----------------------------------------------------------------------- */

void PC_ack_rx( void )
{
    rcv_bytecnt = 0;
}


/* =======================================================================
Run parcom-module tasks
----------------------------------------------------------------------- */

void PC_run_tasks( void )
{
    pc_slave_tx_task();
    pc_slave_rx_task();
}

/* ======================================================================= */
/* ==================    Module internal functions    ==================== */
/* ======================================================================= */

/* =======================================================================
----------------------------------------------------------------------- */

#define P_START_DELAY       0
#define P_START             1
#define P_WAIT_WDR          2
#define P_WAIT_WDR_OFF      3

#define M_MAGIC             0
#define M_LEN               1
#define M_PAYLOAD           2
#define M_COMPLETE          3
#define M_INVALID           4

void pc_slave_rx_task( void )
{
    static uint8_t state = P_START_DELAY;
    static uint8_t msg_state = M_MAGIC;
    static uint8_t msg_len = 0;
    static uint8_t rcv_cnt = 0;
    uint8_t rbyte;

    switch (state)
    {
    case P_START_DELAY:
        // Wait for master signals to go off-state,
        // and at least start delay time
        if (timeout2 && WDR_OFF && RACK_OFF)
            state = P_START;
        break;

    case P_START:
        // Wait until receive buffer free
        // and reset receiving
        if (rcv_bytecnt == 0)
        {
            rx_bufp = pcbuf;
            msg_state = M_MAGIC;
            state = P_WAIT_WDR;
        }
        break;

    case P_WAIT_WDR:
        // wait for WDR and store the received byte to buffer
        if (WDR_ON)
        {
            rbyte = PPI_PB;

            // Interpret the byte according to state
            switch (msg_state)
            {
            case M_MAGIC:
                if (rbyte == 0x7E)
                    msg_state = M_LEN;      // Magic byte ok
                else
                    msg_state = M_INVALID;  // Not magic byte
                break;
                
            case M_LEN:
                // length byte
                msg_len = rbyte & 0x7F;
                cmd_type = rbyte & 0x80;
                rcv_cnt = 0;
                msg_state = M_PAYLOAD;
                break;
                
            case M_PAYLOAD:
                // Handling payload: byte to buffer
                *rx_bufp = rbyte;
                rx_bufp++;
                
                rcv_cnt++;
                if (rcv_cnt == msg_len)
                {
                    rcv_bytecnt = msg_len; // all bytes received
                    msg_state = M_COMPLETE;
                }
                break;
                
            default:    // M_INVALID / M_COMPLETE
                break;
            }
            
            WACK_SET;
            
            start_timer2(RX_TIMEOUT);
            state = P_WAIT_WDR_OFF;
        }
        break;

    case P_WAIT_WDR_OFF:
        // wait for WDR off
        if (WDR_OFF)
        {
            WACK_RESET;
            
            if (msg_state == M_COMPLETE || msg_state == M_INVALID)
                state = P_START;        // message ready (complete or failed)
            else
                state = P_WAIT_WDR;     // message not ready yet
        }
        break;
    }
    
    // if timeout, reset receive and set error flag
    if ((state == P_WAIT_WDR_OFF) && timeout2)
    {
        pc_error = true;

        WACK_RESET;
        RDR_RESET;
        start_timer2(RX_TIMEOUT);
        state = P_START_DELAY;
    }
}

/* =======================================================================
----------------------------------------------------------------------- */

#define P_IDLE           0
#define P_WAIT_REQ_RACK  1
#define P_START_SENDING  2
#define P_SEND_BYTE      3
#define P_WAIT_RACK      4

void pc_slave_tx_task( void )
{
    static uint8_t state = P_IDLE;
    uint8_t pc;
    
    switch (state)
    {
    case P_IDLE:
        // wait for send start, PC_send_to_master() sets this
        switch (send_type)
        {
        case SEND_WITH_REQ:
            // Set RDR as a request for sending
            RDR_SET;
            start_timer3(TX_TIMEOUT);
            state = P_WAIT_REQ_RACK;
            break;
            
        case SEND_NOW:
            start_timer3(TX_TIMEOUT);
            state = P_START_SENDING;
            break;
            
        default:  // SEND_NONE
            break;
        }
        break;

    case P_WAIT_REQ_RACK:
        // wait for master acknowledging the request
        if (RACK_ON)
        {
            RDR_RESET;
            state = P_START_SENDING;
        }
        break;
        
    case P_START_SENDING:
        // prepare for sending
        // writing control byte to 8255 resets outputs
        // this will cause a short glitch to C-port lines which were high
        pc = PPI_PC;
        PPI_control = PPI_MODE_B_OUT;
        PPI_PC = pc;
        state = P_SEND_BYTE;
        break;

    case P_SEND_BYTE:
        // Wait for RACK off
        if (RACK_OFF)
        {
            if (send_bytecnt == 0)
            {
                // all sent
                send_type = SEND_NONE;
                tx_bufp = NULL;
                state = P_IDLE;
            }
            else
            {
                // send next byte
                PPI_PB = *tx_bufp;
                RDR_SET;

                tx_bufp++;
                send_bytecnt--;
                state = P_WAIT_RACK;
            }
        }
        break;

    case P_WAIT_RACK:
        // wait for master acknowledgment
        if (RACK_ON)
        {
            RDR_RESET;
            
            // If all bytes sent, stop driving data port
            if (send_bytecnt == 0)
            {
                // writing control byte to 8255 resets outputs
                // this will cause a short glitch to C-port lines which were high
                pc = PPI_PC;
                PPI_control = PPI_MODE_B_IN;
                PPI_PC = pc;
            }
            
            state = P_SEND_BYTE;
        }
        break;
    }    

    if (send_type != SEND_NONE && timeout3)
    {
        pc_error = true;
        
        WACK_RESET;
        RDR_RESET;
        tx_bufp = NULL;
        send_type = SEND_NONE;
        state = P_IDLE;
    }
}

/* ============================ EOF ====================================== */
