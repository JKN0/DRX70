/***************************************************************************

 patchman.c

 Patch manager for DRX-70.

 9.6.2021  - First version

 ***************************************************************************/

/* --------------------------------------------------------------------------

---------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <datatypes.h>
#include <compiler_defs.h>
#include <C8051F340_defs.h>
#include "DRX_ui.h"
#include "portdefs.h"
#include "silabs_gen.h"
#include "patchman.h"
#include "synth_com/parcom_master.h"

/* =====================================================================
------------------------ Constants & macros ------------------------- */

#define TIME_20MS           (20/RTC_TIME)
#define TIME_30MS           (30/RTC_TIME)
#define TIME_50MS           (50/RTC_TIME)
#define TIME_100MS          (100/RTC_TIME)
#define TIME_700MS          (700/RTC_TIME)

#define OK                   0
#define FAIL                 1

#define DRUMKIT_HALF1        0
#define DRUMKIT_HALF2        1

#define NONE                 0xFF
#define NOT_RUNNING          0xFF

// I2C status vector - top 4 bits only
#define  SMB_MTSTA      0xE0           // (MT) start transmitted
#define  SMB_MTDB       0xC0           // (MT) data byte transmitted
#define  SMB_MRDB       0x80           // (MR) data byte received

#define  WRITE             0           // SMBus WRITE command
#define  READ              1           // SMBus READ command

#define TESTCLEAR(s,d)       if (s) {s=0; d=1;} else d=0;

#define printf               printf_fast

/* =======================================================================
------------------------ I/O ------------------------------------------ */

#define EEPROM_I2C_ADDR      0xA0           // (0x50 << 1) 24LC512
#define PCF_I2C_ADDR         0x70           // (0x38 << 1) PCF8574A

/* =====================================================================
------------------------ Structures --------------------------------- */

/* =====================================================================
------------------------  Global variables  ------------------------- */

extern BIT tick;
extern IDATA BYTE cur_mode;
extern IDATA int cur_patch;
extern IDATA INT8 cur_drumkit;
extern IDATA int cur_save_pos;
extern XDATA BYTE old_save_name[];
extern IDATA BYTE edit_mode;

IDATA BYTE saving_state = SAVE_IDLE;

BIT reload_patch = FALSE;
BIT reload_drumkit = FALSE;
BIT reload_settings = FALSE;
BIT initial_delay = TRUE;

IDATA int prev_patch = 1;
IDATA INT8 prev_drumkit = 1;
IDATA int prev_save_pos = 1;

BIT reloading = FALSE;

BIT i2c_busy = FALSE;

IDATA BYTE slave_addr;                 // Target SMBus slave address
IDATA WORD subaddr;                    // subaddress to be used (for EEPROM)
BIT i2c_rw;                            // Software flag to indicate the direction of the current transfer
BIT send_subaddr;                      // Are we using subaddr?
BIT randomread;                        // Are we reading from subaddr?

BYTE byte_cnt;
XDATA BYTE *i2c_data_buf;

/* =====================================================================
------------------------ Function prototypes ------------------------ */

void read_eeprom_patch( void );
void read_eeprom_patch_name( void );
void read_eeprom_drumkit( void );
void read_eeprom_drumkit_name( void );
void read_eeprom_settings( void );
void save_eeprom_patch( void );
void save_eeprom_drumkit( BIT half );
void save_eeprom_settings( void );
void eeprom_write( WORD eeaddr, XDATA BYTE *buf, BYTE cnt );
void eeprom_read( WORD eeaddr, XDATA BYTE *buf, BYTE cnt );
BIT i2c_active( void );

/* =======================================================================
Patch manager task initialization.
----------------------------------------------------------------------- */

void patch_man_init( void )
{
    reload_settings = TRUE;
}

/* =======================================================================
Patch manager task. 
----------------------------------------------------------------------- */

void patch_man_task( void )
{
    static PDATA BYTE ptimer = TIME_700MS;
    static PDATA BYTE dtimer = NOT_RUNNING;
    static PDATA BYTE stimer = NOT_RUNNING;
    static PDATA BYTE wtimer = NOT_RUNNING;
    BYTE blen = 0;
    XDATA BYTE *pcbuf;
    
    // Wait for a message from synth
    pcbuf = PC_get_rx_buf(&blen);
    if (pcbuf != NULL)
    {
        switch (pcbuf[0])
        {
        case UREQ_NEW_PATCH:
            cur_patch = (pcbuf[2] << 8) | pcbuf[1];
            //printf("\r\nRequest for patch %d\r\n",cur_patch);
            cur_save_pos = cur_patch;
            break;

        case UREQ_NEW_DRUMKIT:
            cur_drumkit = pcbuf[1];
            //printf("\r\nRequest for drumkit %d\r\n",cur_drumkit);
            cur_save_pos = cur_drumkit;
            break;
        }
        PC_ack_rx();   // acknowledge to parcom
    }

    if (pc_error)
    {
        pc_error = FALSE;
        printf("Slave comm error\r\n");
    }


    // --- Run timers
    if (tick)
    {
        if ((ptimer != NOT_RUNNING) && (ptimer != 0))
            ptimer--;
        
        if ((dtimer != NOT_RUNNING) && (dtimer != 0))
            dtimer--;
        
        if ((stimer != NOT_RUNNING) && (stimer != 0))
            stimer--;

        if ((wtimer != NOT_RUNNING) && (wtimer != 0))
            wtimer--;
    }

    // When system starts, give synth time to get up and running
    // before trying to communicate with it
    if (initial_delay)
    {
        if (ptimer == 0)
        {
            ptimer = NOT_RUNNING;
            initial_delay = FALSE;
        }
        else
            return;
    }
          
    // Execute rest of the task only when I2C free and 
    // communication with synth is not going on
    if (i2c_active() || PC_comm_busy())
        return;
    
    if (tick)
    {
        // --- Patch change

        // When user selects new patch from UI or through midi, 
        // read the new patch from EEPROM
        if (cur_patch != prev_patch && saving_state == SAVE_IDLE)
        {
            prev_patch = cur_patch;

            ptimer = TIME_100MS;
            read_eeprom_patch();
            return;
        }

        // When changing edit mode EDITED -> COMPARE,
        // reload the original patch from EEPROM
        if (reload_patch)
        {
            reload_patch = FALSE;
            reloading = TRUE;

            ptimer = TIME_20MS;
            read_eeprom_patch();
            return;
        }

       // If patch has not changed for a while, send to synth
        if (ptimer == 0)
        {
            PC_send_patch_to_synth();
            ptimer = NOT_RUNNING;

            // Change mode after reload done
            if (reloading)
            {
                reloading = FALSE;
                edit_mode = MODE_COMPARE;
            }
        }

        // --- Drum kit change

        // When user selects new drum kit from UI or through midi, 
        // read the new drum kit from EEPROM
        if (cur_drumkit != prev_drumkit && saving_state == SAVE_IDLE)
        {
            prev_drumkit = cur_drumkit;

            dtimer = TIME_100MS;
            read_eeprom_drumkit();
            return;
        }

        // When changing edit mode EDITED -> COMPARE,
        // reload the original drumkit from EEPROM
        if (reload_drumkit)
        {
            reload_drumkit = FALSE;
            reloading = TRUE;

            dtimer = TIME_20MS;
            read_eeprom_drumkit();
            return;
        }

        // If drumkit has not changed for a while, send to synth
        if (dtimer == 0)
        {
            PC_send_drumkit_to_synth();
            dtimer = NOT_RUNNING;
            
            // Change mode after reload done
            if (reloading)
            {
                reloading = FALSE;
                edit_mode = MODE_COMPARE;
            }
        }

        // --- Settings change

        // Reload settings from EEPROM
        if (reload_settings)
        {
            reload_settings = FALSE;

            stimer = TIME_20MS;
            read_eeprom_settings();
            return;
        }

       // When settings have been read from EEPROM, send to synth
        if (stimer == 0)
        {
            PC_send_settings_to_synth();
            stimer = NOT_RUNNING;
        }

        // --- Save position change

        // When user changes save position from UI, 
        // read the old patch/drumkit name from EEPROM
        if (cur_save_pos != prev_save_pos)
        {
            prev_save_pos = cur_save_pos;

            if (cur_mode == MODE_SOUND)
                read_eeprom_patch_name();
            else if (cur_mode == MODE_DRUMS)
                read_eeprom_drumkit_name();
    
            return;
        }
    }

    // --- Patch/drumkit/settings saving to EEPROM
    
    switch (saving_state)
    {
    case SAVE_PATCH:
        // UI wants to save patch, start saving
        save_eeprom_patch();
        wtimer = TIME_30MS;
        saving_state = SAVE_ACTIVE2;
        break;
        
    case SAVE_DRUMKIT:
        // UI wants to save drum kit, start first half
        save_eeprom_drumkit(DRUMKIT_HALF1);
        wtimer = TIME_30MS;
        saving_state = SAVE_ACTIVE1;
        break;

    case SAVE_SETTINGS:
        // UI wants to save settings, start saving
        save_eeprom_settings();
        wtimer = TIME_30MS;
        saving_state = SAVE_ACTIVE2;
        break;
        
    case SAVE_ACTIVE1:
        // Wait while EEPROM does the saving, and start second half
        if (wtimer == 0)
        {
            save_eeprom_drumkit(DRUMKIT_HALF2);
            wtimer = TIME_30MS;
            saving_state = SAVE_ACTIVE2;
        }
        break;

    case SAVE_ACTIVE2:
        // Wait while EEPROM does the saving
        if (wtimer == 0)
        {
            saving_state = SAVE_DONE;
            wtimer = NOT_RUNNING;
        }
        break;
    }
}

/*=====================================================================
Read patch nr 'cur_patch' from EEPROM to 'patch' structure.
EEPROM address is PPPP PPPP Pxxx xxxx, where PPPPPPPPP is the
patch nr 0...447 
---------------------------------------------------------------------*/
 
void read_eeprom_patch( void )
{
    WORD addr = cur_patch << 7;
    
    eeprom_read(addr, (XDATA BYTE *)patch, sizeof(PATCH_T));
}

/*=====================================================================
Read name of patch nr 'cur_save_pos' from EEPROM to 'old_save_name'.
EEPROM address is PPPP PPPP Pxxx xxxx, where PPPPPPPPP is the
patch nr 0...447 
---------------------------------------------------------------------*/
 
void read_eeprom_patch_name( void )
{
    WORD addr = cur_save_pos << 7;
    
    eeprom_read(addr, (XDATA BYTE *)old_save_name, 11);
}

/*=====================================================================
Read drumkit nr 'cur_drumkit' from EEPROM to 'drumkit' structure.
EEPROM address is 111K KKKK xxxx xxxx, where KKKKK is the drumkit nr
0...31
---------------------------------------------------------------------*/
 
void read_eeprom_drumkit( void )
{
    WORD addr = (0xE0 | cur_drumkit) << 8;
    
    eeprom_read(addr, (XDATA BYTE *)drumkit, sizeof(DRUMKIT_T));
}

/*=====================================================================
Read name of drumkit nr 'cur_save_pos' from EEPROM to 'old_save_name'.
EEPROM address is 111K KKKK xxxx xxxx, where KKKKK is the drumkit nr
0...31
---------------------------------------------------------------------*/
 
void read_eeprom_drumkit_name( void )
{
    WORD addr = (0xE0 | cur_save_pos) << 8;
    
    eeprom_read(addr, (XDATA BYTE *)old_save_name, 11);
}

/*=====================================================================
Read settings from EEPROM to 'settings' structure.
---------------------------------------------------------------------*/
 
void read_eeprom_settings( void )
{
    eeprom_read(0xFF00, (XDATA BYTE *)settings, sizeof(SETTINGS_T));
}

/*=====================================================================
Save 'patch' structure to EEPROM as patch nr 'cur_save_pos'.
EEPROM address is PPPP PPPP Pxxx xxxx, where PPPPPPPPP is the
patch nr 0...447 
---------------------------------------------------------------------*/
 
void save_eeprom_patch( void )
{
    WORD addr = cur_save_pos << 7;
    
    eeprom_write(addr, (XDATA BYTE *)patch, sizeof(PATCH_T));
}

/*=====================================================================
Save half of 'drumkit' structure to EEPROM as drumkit nr 'cur_save_pos'.
EEPROM address is 111K KKKK xxxx xxxx, where KKKKK is the drumkit nr
0...30
EEPROM pagesize is 128 bytes and sizeof(DRUMKIT_T) > 128.
---------------------------------------------------------------------*/
 
void save_eeprom_drumkit( BIT half )
{
    WORD addr = (0xE0 | cur_save_pos) << 8;
    
    if (half == DRUMKIT_HALF1)
    {
        eeprom_write(addr, (XDATA BYTE *)drumkit, 128);
    }
    else
    {
        addr |= 0x80;
        eeprom_write(addr, (XDATA BYTE *)drumkit+128, sizeof(DRUMKIT_T)-128);
    }
}

/*=====================================================================
Save 'settings' structure to EEPROM.
---------------------------------------------------------------------*/
 
void save_eeprom_settings( void )
{
    eeprom_write(0xFF00, (XDATA BYTE *)settings, sizeof(SETTINGS_T));
}

/* ======================================================================= */
/* =====================         I2C ROUTINES         ==================== */
/* ======================================================================= */

// Based originally on SiLabs example F34x_SMBus_Master.c

/*=====================================================================
 Writes 'cnt' bytes of data from 'buf' to EEPROM at address 'eeaddr'
---------------------------------------------------------------------*/
 
void eeprom_write( WORD eeaddr, XDATA BYTE *buf, BYTE cnt )
{
    slave_addr = EEPROM_I2C_ADDR;
    subaddr = eeaddr;
    i2c_data_buf = buf;
    byte_cnt = cnt;

    send_subaddr = 1;
    randomread = 0;
    i2c_rw = WRITE;                     // Mark this transfer as a WRITE
    
    i2c_busy = 1;                       // Claim SMBus (set to busy)
    STA = 1;                            // Start transfer
}

/*=====================================================================
 Reads 'cnt' bytes of data to 'buf' from EEPROM starting at address 'eeaddr'
---------------------------------------------------------------------*/
 
void eeprom_read( WORD eeaddr, XDATA BYTE *buf, BYTE cnt )
{
    slave_addr = EEPROM_I2C_ADDR;
    subaddr = eeaddr;
    i2c_data_buf = buf;
    byte_cnt = cnt - 1;

    send_subaddr = 1;
    randomread = 1;
    i2c_rw = WRITE;                     // Mark this transfer as a WRITE because we first write subaddress
    
    i2c_busy = 1;                       // Claim SMBus (set to busy)
    STA = 1;                            // Start transfer
}

/*=====================================================================
 Writes one byte of data from 'buf' to PCF8574 
---------------------------------------------------------------------*/
 
void parcom_data_write( XDATA BYTE *buf )
{
    slave_addr = PCF_I2C_ADDR;
    i2c_data_buf = buf;
    byte_cnt = 1;

    send_subaddr = 0;
    randomread = 0;
    i2c_rw = WRITE;
    
    i2c_busy = 1;                       // Claim SMBus (set to busy)
    STA = 1;                            // Start transfer
}

/*=====================================================================
 Reads one byte of data to 'buf' from PCF8574
---------------------------------------------------------------------*/
 
void parcom_data_read( XDATA BYTE *buf )
{
    slave_addr = PCF_I2C_ADDR;
    i2c_data_buf = buf;
    byte_cnt = 0;

    send_subaddr = 0;
    randomread = 0;
    i2c_rw = READ;
    
    i2c_busy = 1;                       // Claim SMBus (set to busy)
    STA = 1;                            // Start transfer
}

/*=====================================================================
Returns true if I2C is active
---------------------------------------------------------------------*/
 
BIT i2c_active( void )
{
    if (i2c_busy) 
        return TRUE;

    // After I2C interrupt has reset i2c_busy, 
    // generating stop-condition still takes some time
    return STO;
}

/* =======================================================================
I2C-interrupt

// SMBus ISR state machine
// - Master only implementation - no slave or arbitration states defined
// - All incoming data is written to global variable array i2c_data_buf
// - All outgoing data is read from global variable array i2c_data_buf
----------------------------------------------------------------------- */

void i2c_isr(void) __interrupt(SMB0_VECTOR) __using (1) 
{
   static BYTE ctr;
   static BIT send_start = 0;
   static BIT send_subaddr_lo = 0;
   BIT fail = 0;                       // Used by the ISR to flag failed transfers

   if (ARBLOST == 0)                   // Check for errors
   {
      // Normal operation
      switch (SMB0CN & (BYTE)0xF0)           // Status vector
      {
         // Master Transmitter/Receiver: START condition transmitted.
         case SMB_MTSTA:
            SMB0DAT = (slave_addr & 0xFE) | i2c_rw; // Load address of the target slave and R/W-bit
            STA = 0;                                // Manually clear START bit
            ctr = 0;                                // Reset the counter
            break;

         // Master Transmitter: Data byte transmitted
         case SMB_MTDB:
            if (ACK)                   // Slave ACK?
            {
               // Send start condition
               if (send_start)
               {
                   send_start = 0;
                   STA = 1;
                   break;
               }
               
               // Send subaddress hi byte
               if (send_subaddr)
               {
                   send_subaddr = 0;
                   SMB0DAT = subaddr >> 8;
                   send_subaddr_lo = 1;
                   break;
               }
               
               // Send subaddress lo byte
               if (send_subaddr_lo)
               {
                   send_subaddr_lo = 0;
                   SMB0DAT = subaddr & 0xFF;
                   
                   if (randomread)
                   {
                       send_start = 1;  // repeated start
                       i2c_rw = READ;
                   }
                   break;
               }
               
               if (i2c_rw == WRITE)    // If this transfer is a WRITE,
               {
                  if (ctr < byte_cnt)
                  {
                     // send data byte
                     SMB0DAT = i2c_data_buf[ctr];
                     ctr++;
                  }
                  else
                  {
                     STO = 1;          // Set STO to terminate transfer
                     i2c_busy = 0;     // And free SMBus interface
                  }
               }
               else {}                 // If this transfer is a READ, proceed with transfer 
                                       // without writing to SMB0DAT (switch to receive mode)
            }
            else                       // If slave NACK,
            {
               fail = 1;
               //STO = 1;                // Send STOP condition, followed
               //STA = 1;                // By a START
            }
            break;

         // Master Receiver: byte received
         case SMB_MRDB:
            i2c_data_buf[ctr] = SMB0DAT;   // Store received byte

            if (ctr < byte_cnt)
            {
               ACK = 1;                        // Send ACK to indicate byte received
               ctr++;                        // Increment the byte counter
            }
            else
            {
               i2c_busy = 0;            // Free SMBus interface
               ACK = 0;                 // Send NACK to indicate last byte of this transfer
               STO = 1;                 // Send STOP to terminate transfer
            }
            break;

         default:
            fail = 1;                  // Indicate failed transfer and handle at end of ISR
            break;

      } // end switch
   }
   else // ARBLOST 
   {
      fail = 1;
   } 

   if (fail)                           // If the transfer failed,
   {
      SMB0CF &= ~0x80;                 // Reset communication
      SMB0CF |= 0x80;
      STA = 0;
      STO = 0;
      ACK = 0;

      i2c_busy = 0;                    // Free SMBus
   }

   SI = 0;                             // Clear interrupt flag
}

//-----------------------------------------------------------------------------
// Timer3 Interrupt Service Routine (ISR)
//-----------------------------------------------------------------------------
//
// A Timer3 interrupt indicates an SMBus SCL low timeout.
// The SMBus is disabled and re-enabled here
//

void scl_timeout_isr(void) __interrupt(TF3_VECTOR) __using (1) 
{
   SMB0CF &= ~0x80;                    // Disable SMBus
   SMB0CF |= 0x80;                     // Re-enable SMBus
   TMR3CN &= ~0x80;                    // Clear Timer3 interrupt-pending flag
}

/* ============================ EOF ====================================== */

