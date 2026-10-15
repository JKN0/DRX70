//-----------------------------------------------------------------------------
// USB_Mididrv.c
//
// This is the main module of the F340 USB midi driver.
//
// This has been combined from the Silabs CDC example's USB_CDC_UART.c and 
// USB_Class_Requests.c, removing unnecessary functions and and adding 
// midi-specific functions.
//
// JK 5.4.2021
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "USB_Types.h"
#include "USB_Register.h"
#include "USB_Descriptor.h"
#include "USB_Standard_Requests.h"
#include "USB_Mididrv.h"
#include "FIFO_RW.h"

//-----------------------------------------------------------------------------
// Constant
//-----------------------------------------------------------------------------

// size of TX and RX ring buffer (must be greater than EP2_PACKET_SIZE)
#define TXBUFSIZE       250
#define RXBUFSIZE       250

// Memory space allocation for TX and RX buffer
//#define TXBUF_IDATA
//#define TXBUF_PDATA
#define TXBUF_XDATA

//#define RXBUF_IDATA
//#define RXBUF_PDATA
#define RXBUF_XDATA

#if defined TXBUF_IDATA
    typedef BYTE __idata Ttxbuffer;
    #define FIFO_READ_FUNC  FIFO_Read_idata
#elif defined TXBUF_PDATA
    typedef BYTE __pdata Ttxbuffer;
    #define FIFO_READ_FUNC  FIFO_Read_pdata
#elif defined TXBUF_XDATA
    typedef BYTE __xdata Ttxbuffer;
    #define FIFO_READ_FUNC  FIFO_Read_xdata
#endif // TXBUF_IDATA

#if defined RXBUF_IDATA
    typedef BYTE __idata Trxbuffer;
    #define FIFO_WRITE_FUNC FIFO_Write_idata
#elif defined RXBUF_PDATA
    typedef BYTE __pdata Trxbuffer;
    #define FIFO_WRITE_FUNC FIFO_Write_pdata
#elif defined RXBUF_XDATA
    typedef BYTE __xdata Trxbuffer;
    #define FIFO_WRITE_FUNC FIFO_Write_xdata
#endif // RXBUF_IDATA

//-----------------------------------------------------------------------------
// Externs
//-----------------------------------------------------------------------------

extern __pdata Tsetup_buffer Setup;     // Buffer for current device request
extern __bit   setup_handled;           // flag that indicates setup stage is handled or not
extern __idata UINT  DataSize;
extern BYTE* DataPtr;

extern BYTE USB_State;                  // Holds the current USB State
                                        // def. in USB_Main.h
extern BYTE Ep_Status0;
extern __bit Ep_Status1;                    // Contains status bytes for EP 0-2
extern __bit Ep_StatusOUT2;
extern __bit Ep_StatusIN2;

BYTE __xdata midi_in_msg[MIDIBUFSIZE];
BYTE __xdata midi_out_msg[MIDIBUFSIZE];
__bit in_sysex = FALSE;

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

Ttxbuffer TXBuffer[ TXBUFSIZE ];            // Ring buffer for TX and RX
Trxbuffer RXBuffer[ RXBUFSIZE ];
Ttxbuffer * TXRDPtr;                        // pointers for ring buffer
Ttxbuffer * TXWTPtr;
Trxbuffer * RXRDPtr;
Trxbuffer * RXWTPtr;
BYTE TXcount;                               // number of data to transmit from UART
BYTE RXcount;                               // number of data to be recieved by host

#if defined BIG_ENDIAN

void swap_endian_long( ULONG __idata *lptr );

#endif // end of BIG_ENDIAN

// Midi message lengths for channel messages
// index = (statusbyte >> 4) & 0x07
__code BYTE chn_length_lut[] = {
    3,      // 8X = note off
    3,      // 9X = note on
    3,      // AX = poly aftertouch
    3,      // BX = controller
    2,      // CX = program change
    2,      // DX = channel aftertouch
    3,      // EX = pitch bend
    1       // FX = system message
};

// Midi message lengths for system messages
// index = statusbyte & 0x0F
__code BYTE sys_length_lut[] = {
    0,      // F0 = SysEx start (not applicable)
    2,      // F1 = MTC
    3,      // F2 = SPP
    2,      // F3 = Song Select
    0,      // F4 = undefined
    0,      // F5 = undefined
    1,      // F6 = Tune Request
    0,      // F7 = SysEx end (not applicable)
    1,      // F8 = Timing Clock
    0,      // F9 = undefined
    1,      // FA = Start
    1,      // FB = Continue
    1,      // FC = Stop
    0,      // FD = undefined
    1,      // FE = Active Sensing
    1,      // FF = System Reset
};

//-----------------------------------------------------------------------------
// Function prototypes
//-----------------------------------------------------------------------------

void Flush_COMbuffers( void );
BYTE COMGetByte( void );
void COMPutByte( BYTE dt );

void Handle_In2(void);                  // Handle in packet on Endpoint 2
void Handle_Out2(void);                 // Handle out packet on Endpoint 2
void Usb0_Init(void);

//=============================================================================
// Midi driver API functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// init_usb_mididrv
//-----------------------------------------------------------------------------
//  Initialize USB midi driver
//  - USB0 pheripharal
//  - ring buffers
//-----------------------------------------------------------------------------

void init_usb_mididrv( void )
{
    Usb0_Init();
    EIE1 |= 0x02;       // Enable USB0 interrupt

    Flush_COMbuffers(); // Initialize COM ring buffer
}
    
//-----------------------------------------------------------------------------
// run_usb_mididrv
//-----------------------------------------------------------------------------
//  Run USB midi endpoint handling tasks
//
//  This routine is called from mainloop
//-----------------------------------------------------------------------------

void run_usb_mididrv( void )
{
    Handle_In2();
    Handle_Out2();
}
    

//-----------------------------------------------------------------------------
// get_midi_message
//-----------------------------------------------------------------------------
//  Get one complete midi message from USB host
//
//  Message is returned in global midi_in_msg[] buffer.
//  Return value is length of the midi message or 0 if no message available.
//-----------------------------------------------------------------------------

BYTE get_midi_message( void )
{
    static BYTE sysex_idx = 0;
    BYTE cin;
    BYTE i,sxlen;

    // If no new Midi Event Packet, nothing to do
    if (TXcount < 4) return 0;

    // --- New 4-byte event packet in ring buffer, handle it
    
    // Get event packet header
    cin = COMGetByte() & 0x0F;  // ignore midi cable number

    if (!in_sysex)
    {
        // -- Normal case (non-SysEx)
        
        // Copy rest of the event packet to midi_in_msg
        for (i = 0; i < 3; i++)
            midi_in_msg[i] = COMGetByte();

        switch (cin)
        {
    	case 0x5:   // not in SysEx, so this is single-byte SystemCommon
    	case 0xF:
    		return 1;

    	case 0x2:
    	case 0xC:
    	case 0xD:
    		return 2;

    	case 0x3:
    	case 0x8:
    	case 0x9:
    	case 0xA:
    	case 0xB:
    	case 0xE:
    		return 3;

    	case 0x4:
            // SysEx starts and will continue
            in_sysex = TRUE;
            sysex_idx = 3;
            return 0;   // message not ready yet
            
    	case 0x6:
    	case 0x7:
            // SysEx starts and ends
            in_sysex = FALSE;
            sysex_idx = 0;
            return cin - 4; // 0x6 -> 2 bytes, 0x7 -> 3 bytes
        
    	default: // Unhandled 0x0 and 0x1
    		return 0;
        }
    }
    else
    {
        // -- SysEx continuation
        
        // Copy rest of the event packet to midi_in_msg
        for (i = 0; i < 3; i++)
            midi_in_msg[sysex_idx+i] = COMGetByte();

        switch (cin)
        {
    	case 0x4:
            // SysEx continues
            sysex_idx += 3;
            return 0;   // message not ready yet
            
    	case 0x5:
    	case 0x6:
    	case 0x7:
            // SysEx ends: 0x5 -> 1 byte, 0x6 -> 2 bytes, 0x7 -> 3 bytes
            sxlen = sysex_idx + cin - 4;
            in_sysex = FALSE;
            sysex_idx = 0;
            return sxlen;

    	default:
            // Error: should not come inside SysEx
            in_sysex = FALSE;
            sysex_idx = 0;
            return 0;
        }
    }
}
    
//-----------------------------------------------------------------------------
// put_midi_message
//-----------------------------------------------------------------------------
//  Send one complete midi message to USB host
//
//  Message is in global midi_out_msg[] buffer. sxlen is the sysex message length,
//  it is ignored for other message types.
//  Return FALSE if message does not fit in the ring buffer or is invalid.
//  No support for running status.
//-----------------------------------------------------------------------------

__bit put_midi_message( BYTE sxlen )
{
    BYTE cin,len;
    BYTE i,msgidx;

    // Message must start with status byte
    if ((midi_out_msg[0] & 0x80) == 0)
        return FALSE;

    if (midi_out_msg[0] == 0xF0)
    {
        // --- SysEx message

        // Empty messages (only F0 F7) not accepted
        if (sxlen < 3)
            return FALSE;

        // While loop sends all packets except the last one
        msgidx = 0;
        while (sxlen > 3)
        {
            // If no room in RXBuffer, return failure
            if (RXcount > RXBUFSIZE-4) 
                return FALSE;

            COMPutByte(0x04);
            for (i = 0; i < 3; i++)
                COMPutByte(midi_out_msg[msgidx++]);

            sxlen -= 3;
        }
        
        // now sxlen is 1...3, never 0

        // If no room in RXBuffer, return failure
        if (RXcount > RXBUFSIZE-4) 
            return FALSE;

        // Send the last packet  
        cin = sxlen | 0x04;   // 1 -> 0x05, 2 -> 0x06, 3 -> 0x07

        COMPutByte(cin);
        for (i = 0; i < 3; i++)
        {
            if (i < sxlen)
                COMPutByte(midi_out_msg[msgidx++]);
            else
                COMPutByte(0);
        }
    }
    else
    {   // --- Other than SysEx messages

        // These will always produce one Event Packet (4 bytes)
        // If no room in RXBuffer, return failure
        if (RXcount > RXBUFSIZE-4) 
            return FALSE;

        // Calculate CIN-byte and length of the other bytes (1...3)
        if ((midi_out_msg[0] & 0xF0) == 0xF0)
        {
            // System messages
            len = sys_length_lut[midi_out_msg[0] & 0x0F];

            switch (len)
            {
            case 1: cin = 0x05; break;
            case 2: cin = 0x02; break;
            case 3: cin = 0x03; break;
            default: return FALSE;      // don't accept undefined messages
            }
        }
        else
        {   // Channel messages
            cin = midi_out_msg[0] >> 4;         // always cable nr 0
            len = chn_length_lut[cin & 0x07];
        }

        // Send 4 bytes to USB
        COMPutByte(cin);
        for (i = 0; i < 3; i++)
        {
            if (i < len)
                COMPutByte(midi_out_msg[i]);
            else
                COMPutByte(0);
        }
    }

    return TRUE;
}

//=============================================================================
// Functions originating from the Silabs USB CDC example
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// class specific request handler
//-----------------------------------------------------------------------------

void Class_Request( void )
{
}

//-----------------------------------------------------------------------------
// Vendor request handler
//-----------------------------------------------------------------------------

void Vendor_Request( void )
{
}

//-----------------------------------------------------------------------------
// Flush_COMbuffers, COMGetByte, COMPutByte
//-----------------------------------------------------------------------------
// Ring buffers for TX (host --> UART) and RX (UART --> host) over bulk EP
//
//  Flush_COMbuffers:   initialize COM ring buffer
//  COMGetByte:         read a byte from COM TX buffer
//  COMPutByte:         write a byte to COM RX buffer
//-----------------------------------------------------------------------------

//---------------------------
// SDCC support
//---------------------------
#if defined __SDCC
#pragma save
#pragma nooverlay
#endif // __SDCC

void Flush_COMbuffers( void )
{
    TXRDPtr = TXBuffer;
    TXWTPtr = TXBuffer;
    RXRDPtr = RXBuffer;
    RXWTPtr = RXBuffer;
    TXcount = 0;
    RXcount = 0;
}

//---------------------------
// SDCC support
//---------------------------
#if defined __SDCC
#pragma restore
#endif // __SDCC


// TX buffer (host --> UART), UART side
BYTE COMGetByte( void )
{
    BYTE dt;

    dt = *TXRDPtr;
    if ( TXRDPtr == (TXBuffer + (TXBUFSIZE - 1)) )  // at the end of the buffer
        TXRDPtr = TXBuffer;                 // go to the head of the buffer
    else
        TXRDPtr++;

    TXcount--;

    return dt;
}

// RX buffer (UART --> host), UART side
void COMPutByte( BYTE dt )
{
    *RXWTPtr = dt;
    if ( RXWTPtr == (RXBuffer + (RXBUFSIZE - 1)) )  // at the end of the buffer
        RXWTPtr = RXBuffer;                 // go to the head of the buffer
    else
        RXWTPtr++;

    RXcount++;
}

//-----------------------------------------------------------------------------
// Handle_In2
//-----------------------------------------------------------------------------
//  Handle IN EP2
//  - Endpoint HALT - STALL
//  - RX data stream
//-----------------------------------------------------------------------------

void Handle_In2( void )
{
    BYTE ControlReg;
    BYTE send_cnt, first_cnt, second_cnt;
    UINT pos;
    __bit turnover;

    if (Ep_StatusIN2 == EP_IDLE)                        // If endpoint is currently halted, 
    {
        EIE1 &= ~0x02;                                  // disable USB interrupt

        POLL_WRITE_BYTE(INDEX, 2);                      // Set index to endpoint 2 registers
        POLL_READ_BYTE(EINCSR1, ControlReg);            // Read contol register for EP

                                                        // RX buffer (UART --> host), USB side
        if ( !(ControlReg & rbInINPRDY) && RXcount )    // FIFO is empty and must send?
        {
            if ( RXcount > EP2_PACKET_SIZE )            // determine the bytes to send in this transaction
                send_cnt = EP2_PACKET_SIZE;
            else
                send_cnt = (BYTE)RXcount;

            pos = RXBUFSIZE - (RXRDPtr - RXBuffer);     // consider on wrap around of ring buffer
            if ( send_cnt < pos )
            {
                first_cnt = send_cnt;
                second_cnt = 0;
                turnover = FALSE;
            }
            else
            {
                first_cnt = (BYTE)pos;
                second_cnt = send_cnt - first_cnt;
                turnover = TRUE;
            }

            FIFO_WRITE_FUNC( FIFO_EP2, first_cnt, RXRDPtr );    // send bytes to FIFO
            if ( second_cnt != 0 )
                FIFO_WRITE_FUNC( FIFO_EP2, second_cnt, RXBuffer );
            POLL_WRITE_BYTE(EINCSR1, rbInINPRDY);

            if ( turnover )
                RXRDPtr = RXBuffer + second_cnt;
            else
                RXRDPtr += send_cnt;

            RXcount -= send_cnt;
        }

        EIE1 |= 0x02;                                   // enable USB interrupt
    }
}

//-----------------------------------------------------------------------------
// Handle_Out2
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Take the received packet from the host off the fifo and put it into
// the Out_Packet array
//-----------------------------------------------------------------------------

void Handle_Out2( void )
{
    BYTE ControlReg;
    BYTE FIFO_Cnt, first_cnt, second_cnt;
    UINT pos;
    __bit turnover;

    if (Ep_StatusOUT2 == EP_IDLE)                       // If endpoint is ready
    {
        EIE1 &= ~0x02;                                  // disable USB interrupt

        POLL_WRITE_BYTE(INDEX, 2);                      // Set index to endpoint 2 registers
        POLL_READ_BYTE(EOUTCSR1, ControlReg);
                                                        // TX buffer (host --> UART), USB side
        if ( ControlReg & rbOutOPRDY )                  // FIFO has data?
        {
            POLL_READ_BYTE(EOUTCNTL, FIFO_Cnt);         // read out number of bytes on the FIFO

            if ( (TXBUFSIZE - TXcount) >= FIFO_Cnt )    // if any room on the buffer to read out the FIFO
            {
                pos = TXBUFSIZE - (TXWTPtr - TXBuffer); // consider on wrap around of ring buffer
                if ( FIFO_Cnt < pos )
                {
                    first_cnt = FIFO_Cnt;
                    second_cnt = 0;
                    turnover = FALSE;
                }
                else
                {
                    first_cnt = (BYTE)pos;
                    second_cnt = FIFO_Cnt - first_cnt;
                    turnover = TRUE;
                }

                FIFO_READ_FUNC( FIFO_EP2, first_cnt, TXWTPtr ); // read bytes from FIFO
                if ( second_cnt != 0 )
                    FIFO_READ_FUNC( FIFO_EP2, second_cnt, TXBuffer );
                POLL_WRITE_BYTE(EOUTCSR1, 0);           // Clear Out Packet ready bit

                if ( turnover )
                    TXWTPtr = TXBuffer + second_cnt;
                else
                    TXWTPtr += FIFO_Cnt;

                TXcount += FIFO_Cnt;
            }
        }

        EIE1 |= 0x02;                                   // enable USB interrupt
    }
}

/* =======================================================================
Usb0_Init

 Return Value : None
 Parameters   : None

 - Initialize USB0
 - Enable USB0 interrupts
 - Enable USB0 transceiver
 - Enable USB0 with suspend detection
----------------------------------------------------------------------- */

void Usb0_Init(void)
{
    POLL_WRITE_BYTE(POWER,  0x08);      // Force Asynchronous USB Reset
                                        // USB Interrupt enable flags are reset by USB bus reset
                                        // 
    POLL_WRITE_BYTE(IN1IE,  0x01);      // Enable EP 0 interrupt, disable EP1-3 IN interrupt
    POLL_WRITE_BYTE(OUT1IE, 0x00);      // Disable EP 1-3 OUT interrupts
    POLL_WRITE_BYTE(CMIE,   0x05);      // Enable Reset and Suspend interrupts

    USB0XCN = 0xE0;                     // Enable transceiver; select full speed
    POLL_WRITE_BYTE(CLKREC, 0x80);      // Enable clock recovery, single-step mode disabled
                                        // Enable USB0 by clearing the USB Inhibit bit
    POLL_WRITE_BYTE(POWER,  0x01);      // and enable suspend detection
}

//-----------------------------------------------------------------------------
// swap_endian_long
//-----------------------------------------------------------------------------
// swap endian for long varialbe
//-----------------------------------------------------------------------------

#if defined BIG_ENDIAN

void swap_endian_long( ULONG __idata *lptr )
{
/*
    BYTE tmp;
    BYTE __idata * ptr = (BYTE __idata *)lptr;

    tmp = ptr[0];
    ptr[0] = ptr[3];
    ptr[3] = tmp;
    tmp = lptr[1];
    ptr[1] = ptr[2];
    ptr[2] = tmp;
*/
    BYTE tmp[4];
    BYTE __idata * ptr = (BYTE __idata *)lptr;

    tmp[3] = ptr[0];
    tmp[2] = ptr[1];
    tmp[1] = ptr[2];
    tmp[0] = ptr[3];

    ptr[0] = tmp[0];
    ptr[1] = tmp[1];
    ptr[2] = tmp[2];
    ptr[3] = tmp[3];
}

#endif // end of BIG_ENDIAN

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
