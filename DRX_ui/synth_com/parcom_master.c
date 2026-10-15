/***************************************************************************
 
 parcom_master.c
 
 Master for DRX-70 internal parallel port communication
 
 5.9.2021

 ***************************************************************************/

// Sending:
// - PC_send_xxx_to_synth() functions write data to ring buffer msgbuf
// - pc_msg_task() reads messages from ring buffer and copies them to pcbuf one by one
// - pc_master_tx_task() sends message from pcbuf to slave

// Receiving:
// - pc_master_rx_task() receives message to pcbuf from slave 
// - PC_get_rx_buf() polls for received message
// - PC_ack_rx() acknowledges pcbuf as free

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <datatypes.h>
#include <compiler_defs.h>
#include <C8051F340_defs.h>
#include "../portdefs.h"
#include "../silabs_gen.h"
#include "../DRX_ui.h"
#include "../patchman.h"
#include "parcom_master.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define CYCLIC_INC(p)    { p++; if (p >= (msgbuf+MSGBUF_LEN)) p = msgbuf; }

/* =======================================================================
------------------------ I/O ------------------------------------------ */

/* =====================================================================
------------------------ Structures --------------------------------- */

typedef struct pcmsg_t {
    BYTE cmd;
    BYTE buflen;
    XDATA BYTE *buf;
} PCMSG_T;

/* =====================================================================
------------------------  Global variables  ------------------------- */

static BYTE pc_tx_timeout_ctr;
static BYTE pc_rx_timeout_ctr;

static BIT volatile pc_tx_timeout;
static BIT volatile pc_rx_timeout;

// ring buffer for messages to msg_task
static XDATA PCMSG_T msgbuf[MSGBUF_LEN];
static XDATA PCMSG_T *msgbuf_top = msgbuf;
static XDATA PCMSG_T *msgbuf_bot = msgbuf;
static BIT msgbuf_full = FALSE;

// buffer for parcom layer message (rx and tx)
static XDATA BYTE pcbuf[PCBUF_LEN];
static XDATA BYTE *tx_bufp = NULL;
static XDATA BYTE *rx_bufp = NULL;

// 32 buffers for app layer messages for byte/word parameter/midi sending
static XDATA BYTE databuf_tbl[32][4];
static BYTE dbt_idx = 0;

static BYTE send_bytecnt = 0;
static BYTE rcv_bytecnt = 0;
static XDATA BYTE i2c_buf;
static IDATA BYTE cmd_type = CMD_NONE;
static BIT rcv_allowed = FALSE;

BIT pc_error = FALSE;

/* =====================================================================
------------------------ Function prototypes ------------------------ */

static void put_to_ringbuf( XDATA BYTE *buf, BYTE buflen, BYTE cmd );
static void pc_msg_task( void );
static void pc_master_tx_task( void );
static void pc_master_rx_task( void );
static void pc_set_tx_timeout( BYTE delay );
static void pc_set_rx_timeout( BYTE delay );

/* ======================================================================= */
/* =====================            API               ==================== */
/* ======================================================================= */

/* =======================================================================
Parcom module init
----------------------------------------------------------------------- */

void PC_init( void )
{
    WDR = 0;
    RACK = 0;
    
    pc_set_tx_timeout(TX_TIMEOUT);
    pc_set_rx_timeout(RX_TIMEOUT);
}

// These functions must be __reentrant because they are called from UI-functions,
// which are __reentrant

/*=====================================================================
Send contents of patch struct to synth.
---------------------------------------------------------------------*/
 
void PC_send_patch_to_synth( void ) __reentrant
{
    // Don't send empty patch
    if (patch.name[0] == 0xFF)
        return;

    // Send the struct without name
    patch.scmd = SCMD_NEW_PATCH;
    put_to_ringbuf((XDATA BYTE *)patch+NAMELEN,PATCHDEF_SIZE,CMD_WRITE);
}

/*=====================================================================
Send contents of drumkit struct to synth.
---------------------------------------------------------------------*/
 
void PC_send_drumkit_to_synth( void ) __reentrant
{
    // Don't send empty drumkit
    if (drumkit.name[0] == 0xFF)
        return;

    // Send the struct without name and fill
    drumkit.scmd = SCMD_NEW_DRUMKIT;
    put_to_ringbuf((XDATA BYTE *)drumkit+NAMELEN,DRUMKITDEF_SIZE,CMD_WRITE);
}

/*=====================================================================
Send contents of settings struct to synth.
---------------------------------------------------------------------*/
 
void PC_send_settings_to_synth( void ) __reentrant
{
    settings.scmd = SCMD_NEW_SETTINGS;
    put_to_ringbuf((XDATA BYTE *)settings,SETTINGSDEF_SIZE,CMD_WRITE);
}

/* =======================================================================
Send parameter to synth (byte)
----------------------------------------------------------------------- */

void PC_send_byte_param_to_synth( BYTE scmd, BYTE op_inst, BYTE parval ) __reentrant
{
    XDATA BYTE *dbuf;

    TP4 = 1;
    // use next databuffer
    dbt_idx = (dbt_idx + 1) & 0x1F;
    dbuf = databuf_tbl[dbt_idx];

    // build the app layer message to buffer
    dbuf[0] = scmd;
    dbuf[1] = op_inst;
    dbuf[2] = parval;

    // send
    put_to_ringbuf(dbuf,3,CMD_WRITE);
    TP4 = 0;
}

/* =======================================================================
Send parameter to synth (word)
----------------------------------------------------------------------- */

void PC_send_word_param_to_synth( BYTE scmd, BYTE op_inst, WORD parval ) __reentrant
{
    XDATA BYTE *dbuf;

    TP4 = 1;
    // use next databuffer
    dbt_idx = (dbt_idx + 1) & 0x1F;
    dbuf = databuf_tbl[dbt_idx];

    // build the app layer message to buffer
    dbuf[0] = scmd;
    dbuf[1] = op_inst;
    dbuf[2] = parval & 0xFF;
    dbuf[3] = parval >> 8;

    // send
    put_to_ringbuf(dbuf,4,CMD_WRITE);
    TP4 = 0;
}

/* =======================================================================
Send midi message to synth
----------------------------------------------------------------------- */

void PC_send_midi_msg_to_synth( XDATA BYTE *midibuf, BYTE len ) __reentrant
{
    XDATA BYTE *dbuf;

    TP4 = 1;
    // use next databuffer
    dbt_idx = (dbt_idx + 1) & 0x1F;
    dbuf = databuf_tbl[dbt_idx];

    // build the app layer message to buffer
    dbuf[0] = SCMD_MIDI_MSG;
    dbuf[1] = midibuf[0];
    dbuf[2] = midibuf[1];
    dbuf[3] = midibuf[2];

    // send
    put_to_ringbuf(dbuf,len+1,CMD_WRITE);
    TP4 = 0;
}

/* =======================================================================
Return TRUE if still communicating
----------------------------------------------------------------------- */

BIT PC_comm_busy( void )
{
    if ((tx_bufp != NULL) || rcv_allowed)
        return TRUE;
    else
        return FALSE;
}

/* =======================================================================
Return pointer to receive buffer or NULL, if no data. 
Buffer length in *len.
----------------------------------------------------------------------- */

XDATA BYTE *PC_get_rx_buf( BYTE *len )
{
    if (rcv_bytecnt != 0)
    {   
        *len = rcv_bytecnt;
        return pcbuf;
    }
    else    
        return NULL;
}

/* =======================================================================
Acknowledge the returned buffer as free.
----------------------------------------------------------------------- */

void PC_ack_rx( void )
{
    rcv_bytecnt = 0;
}

/* =======================================================================
Parcom module timeouts (called from RTC interrupt).
----------------------------------------------------------------------- */

void PC_update_timeouts( void )  __using (1)
{
    pc_tx_timeout_ctr--;
    if (pc_tx_timeout_ctr == 0)
        pc_tx_timeout = TRUE;
        
    pc_rx_timeout_ctr--;
    if (pc_rx_timeout_ctr == 0)
        pc_rx_timeout = TRUE;
}


/* =======================================================================
Parcom module tasks
----------------------------------------------------------------------- */

void PC_run_tasks( void )
{
    pc_msg_task();
    pc_master_tx_task();
    pc_master_rx_task();
}

/* ======================================================================= */
/* ===================    Module internal functions    =================== */
/* ======================================================================= */

/* =======================================================================
Put message in ring buffer. cmd is CMD_WRITE or CMD_READ
----------------------------------------------------------------------- */

static void put_to_ringbuf( XDATA BYTE *buf, BYTE buflen, BYTE cmd )
{
    if (msgbuf_full)
        return;

    msgbuf_top->cmd = cmd;
    msgbuf_top->buflen = buflen;
    msgbuf_top->buf = buf;

    CYCLIC_INC(msgbuf_top);
    if (msgbuf_bot == msgbuf_top)
        msgbuf_full = TRUE;
}

/* =======================================================================
Takes messages from ring buffer and copies them to pcbuf one by one.
Waits until pc_master_tx_task has sent the whole message, before taking 
a new one from ring buffer.
----------------------------------------------------------------------- */

static void pc_msg_task( void )
{
    static BIT idle = TRUE;

    if (idle)
    {
        // Wait for messages to appear in the ring buffer
        if ((msgbuf_bot != msgbuf_top) || msgbuf_full)
        {
            // build the parcom layer message to pcbuf
            pcbuf[0] = 0x7E;                                    // magic byte
            pcbuf[1] = msgbuf_bot->cmd | msgbuf_bot->buflen;    // R/W + length
            memcpy(pcbuf+2,msgbuf_bot->buf,msgbuf_bot->buflen); // data

            tx_bufp = pcbuf;
            send_bytecnt = msgbuf_bot->buflen + 2;
            cmd_type = msgbuf_bot->cmd;

            CYCLIC_INC(msgbuf_bot);
            msgbuf_full = FALSE;

            idle = FALSE;
        }
    }
    else
    {
        // Wait for message sent, received and handled
        if ((tx_bufp == NULL) && !rcv_allowed && rcv_bytecnt == 0)
        {
            idle = TRUE;
        }
    }
}

/* =======================================================================
Parcom send task. Sends one message from tx_bufp to slave.
----------------------------------------------------------------------- */

#define P_IDLE          0
#define P_SEND_I2C_BYTE 1
#define P_SET_WDR       2
#define P_WAIT_WACK     3
#define P_WAIT_I2C_FF_T 4

static void pc_master_tx_task( void )
{
    static BYTE state = P_IDLE;

    switch (state)
    {
    case P_IDLE:
        // Wait for parcom layer message to send
        if (tx_bufp != NULL)
        {
            pc_set_tx_timeout(TX_TIMEOUT);
            state = P_SEND_I2C_BYTE;
        }
        break;

    case P_SEND_I2C_BYTE:
        // Wait for WACK off
        if (!WACK)
        {
            if (send_bytecnt == 0)
            {
                // All bytes sent: if a read, allow rx task to run
                if (cmd_type == CMD_READ)
                    rcv_allowed = TRUE;

                tx_bufp = NULL;
                state = P_IDLE;
            }
            else
            {   // Not all bytes sent: send next
                parcom_data_write(tx_bufp);
                tx_bufp++;
                send_bytecnt--;
                state = P_SET_WDR;
            }
        }
        break;

    case P_SET_WDR:
        // Wait for I2C transaction (data byte write)
        if (!i2c_active())
        {
            // inform slave about the new data byte
            WDR = 1;
            state = P_WAIT_WACK;
        }
        break;

    case P_WAIT_WACK:
        // Wait for ack from slave
        if (WACK)
        {
            if ((send_bytecnt == 0) && (cmd_type == CMD_READ))
            {
                // We are acknowledging the last byte in read command:
                // set the PCF8574 port as input before acking with WDR off
                i2c_buf = 0xFF;
                parcom_data_write(&i2c_buf);
                state = P_WAIT_I2C_FF_T;
            }
            else
            {
                // Normal byte
                WDR = 0;
                state = P_SEND_I2C_BYTE;
            }
        }
        break;

    case P_WAIT_I2C_FF_T:
        // Wait for I2C transaction (port direction change)
        if (!i2c_active())
        {
            WDR = 0;
            state = P_SEND_I2C_BYTE;
        }
        break;
    }    

    // error, if the whole message is not sent in TX_TIMEOUT time
    if (tx_bufp != NULL && pc_tx_timeout)
    {
        pc_error = TRUE;

        WDR = 0;  
        RACK = 0;
        tx_bufp = NULL;
        state = P_IDLE;
    }
}

/* =======================================================================
Parcom receive task. Receives one message from slave to pcbuf.
----------------------------------------------------------------------- */

#define P_START_DELAY       0
#define P_START             1
#define P_WAIT_I2C_FF_R     2
#define P_WAIT_RDR          3
#define P_WAIT_I2C_RCV      4
#define P_WAIT_RDR_OFF      5

#define M_MAGIC             0
#define M_LEN               1
#define M_PAYLOAD           2
#define M_COMPLETE          3
#define M_INVALID           4

static void pc_master_rx_task( void )
{
    static BYTE state = P_START_DELAY;
    static BYTE msg_state = M_MAGIC;
    static BYTE msg_len = 0;
    static BYTE rcv_cnt = 0;
    BYTE rbyte;

    switch (state)
    {
    case P_START_DELAY:
        // Wait for slave signals to go off-state,
        // and at least start delay time
        if (pc_rx_timeout && !RDR && !WACK)
            state = P_START;
        return;

    case P_START:
        // wait until previous message has been processed
        if (rcv_bytecnt == 0)
        {
            // wait until receive starts; two cases:
            // - we have sent a read request to slave
            // - slave is requesting a permission to send data
            if (rcv_allowed)
            {
                // prepare for receiving
                rx_bufp = pcbuf;
                msg_state = M_MAGIC;
                pc_set_rx_timeout(RX_TIMEOUT);
                state = P_WAIT_RDR;
            }
            else if (RDR)
            {
                // spontaneous RDR: slave is requesting send permission
                // set the PCF8574 as input port
                i2c_buf = 0xFF;
                parcom_data_write(&i2c_buf);
                pc_set_rx_timeout(RX_TIMEOUT);
                state = P_WAIT_I2C_FF_R;
            }
        }
        break;

    case P_WAIT_I2C_FF_R:
        // wait for I2C transaction and acknowledge to slave
        if (!i2c_active())
        {
            RACK = 1;

            // prepare for receiving
            rx_bufp = pcbuf;
            msg_state = M_MAGIC;
            state = P_WAIT_RDR_OFF;
        }
        break;

    case P_WAIT_RDR:
        // Wait for RDR and get the received byte
        if (RDR)
        {
            parcom_data_read(&i2c_buf);
            state = P_WAIT_I2C_RCV;
        }
        break;
        
    case P_WAIT_I2C_RCV:
        // Wait for I2C transaction (receive data byte)
        if (!i2c_active())
        {
            rbyte = i2c_buf;

            // Interpret the received byte according to the message
            // receiving state
            switch (msg_state)
            {
            case M_MAGIC:
                if (rbyte == 0x7E)
                    msg_state = M_LEN;      // Magic byte ok
                else
                    msg_state = M_INVALID;  // Not magic byte
                break;
                
            case M_LEN:
                // Length byte
                msg_len = rbyte & 0x7F;
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
            
            RACK = 1;   // ack to slave
            
            state = P_WAIT_RDR_OFF;
        }
        break;

    case P_WAIT_RDR_OFF:
        // Wait for RDR off
        if (!RDR)
        {
            RACK = 0;
            
            if (msg_state == M_COMPLETE || msg_state == M_INVALID)
            {
                rcv_allowed = FALSE;    // message ready (complete or failed)
                state = P_START;        
            }
            else
                state = P_WAIT_RDR;     // message not ready yet
        }
        break;
    }
    
    // error, if whole message is not received in RX_TIMEOUT time
    if ((state != P_START) && pc_rx_timeout)
    {
        pc_error = TRUE;

        WDR = 0;
        RACK = 0;
        rcv_allowed = FALSE;
        pc_set_rx_timeout(RX_TIMEOUT);
        state = P_START_DELAY;
    }
}

/* =======================================================================
Start timeout for tx
----------------------------------------------------------------------- */

static void pc_set_tx_timeout( BYTE delay )
{
    DISABLE(ET2);
    pc_tx_timeout = FALSE;
    pc_tx_timeout_ctr = delay;
    ENABLE(ET2);
}

/* =======================================================================
Start timeout for rx
----------------------------------------------------------------------- */

static void pc_set_rx_timeout( BYTE delay )
{
    DISABLE(ET2);
    pc_rx_timeout = FALSE;
    pc_rx_timeout_ctr = delay;
    ENABLE(ET2);
}

/* ============================ EOF ====================================== */
