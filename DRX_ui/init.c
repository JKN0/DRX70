/////////////////////////////////////
//  Generated Initialization File  //
/////////////////////////////////////

#include "C8051F340.h"

// Peripheral specific initialization functions,
// Called from the Init_Device() function
void PCA_Init()
{
    PCA0MD    &= ~0x40;
    PCA0MD    = 0x00;
}

void Timer_Init()
{
    TCON      = 0x55;
    TMOD      = 0x21;
    CKCON     = 0x08;
    TH1       = 0x60;
    TMR2CN    = 0x04;
    TMR3RLL   = 0x5F;
    TMR3RLH   = 0x79;
}

void UART_Init()
{
    SBRLL1    = 0x30;
    SBRLH1    = 0xFF;
    SCON1     = 0x10;
    SBCON1    = 0x43;
}

void SMBus_Init()
{
    SMB0CF    = 0xCD;
}

void Port_IO_Init()
{
    // P0.0  -  Skipped,     Open-Drain, Digital
    // P0.1  -  Skipped,     Open-Drain, Digital
    // P0.2  -  Skipped,     Open-Drain, Digital
    // P0.3  -  Skipped,     Open-Drain, Digital
    // P0.4  -  Skipped,     Open-Drain, Digital
    // P0.5  -  Skipped,     Open-Drain, Digital
    // P0.6  -  Skipped,     Open-Drain, Analog
    // P0.7  -  Skipped,     Open-Drain, Analog

    // P1.0  -  Skipped,     Open-Drain, Digital
    // P1.1  -  Skipped,     Open-Drain, Digital
    // P1.2  -  Skipped,     Open-Drain, Digital
    // P1.3  -  Skipped,     Push-Pull,  Digital
    // P1.4  -  Skipped,     Push-Pull,  Digital
    // P1.5  -  SDA (SMBus), Open-Drain, Digital
    // P1.6  -  SCL (SMBus), Open-Drain, Digital
    // P1.7  -  Skipped,     Push-Pull,  Digital

    // P2.0  -  Skipped,     Open-Drain, Digital
    // P2.1  -  Skipped,     Open-Drain, Digital
    // P2.2  -  Skipped,     Open-Drain, Digital
    // P2.3  -  Skipped,     Open-Drain, Digital
    // P2.4  -  Skipped,     Open-Drain, Digital
    // P2.5  -  Skipped,     Open-Drain, Digital
    // P2.6  -  TX1 (UART1), Push-Pull,  Digital
    // P2.7  -  RX1 (UART1), Open-Drain, Digital

    // P3.0  -  Unassigned,  Open-Drain, Digital
    // P3.1  -  Unassigned,  Open-Drain, Digital
    // P3.2  -  Unassigned,  Push-Pull,  Digital
    // P3.3  -  Unassigned,  Push-Pull,  Digital
    // P3.4  -  Unassigned,  Push-Pull,  Digital
    // P3.5  -  Unassigned,  Push-Pull,  Digital
    // P3.6  -  Unassigned,  Push-Pull,  Digital
    // P3.7  -  Unassigned,  Push-Pull,  Digital

    P0MDIN    = 0x3F;
    P1MDOUT   = 0x98;
    P2MDOUT   = 0x40;
    P3MDOUT   = 0xFC;
    P4MDOUT   = 0xFF;
    P0SKIP    = 0xFF;
    P1SKIP    = 0x9F;
    P2SKIP    = 0x3F;
    XBR0      = 0x04;
    XBR1      = 0x40;
    XBR2      = 0x01;
}

void Oscillator_Init()
{
    int i = 0;
    OSCXCN    = 0x67;
    for (i = 0; i < 3000; i++);  // Wait 1ms for initialization
    while ((OSCXCN & 0x80) == 0);
    FLSCL     = 0x90;
    CLKMUL    = 0x81;
    for (i = 0; i < 20; i++);    // Wait 5us for initialization
    CLKMUL    |= 0xC0;
    while ((CLKMUL & 0x20) == 0);
    CLKSEL    = 0x03;
    OSCICN    = 0x00;
}

void Interrupts_Init()
{
    EIE1      = 0x01;
    EIE2      = 0x02;
    IT01CF    = 0x20;
    IE        = 0x25;
}

// Initialization function for device,
// Call Init_Device() from your main program
void Init_Device(void)
{
    PCA_Init();
    Timer_Init();
    UART_Init();
    SMBus_Init();
    Port_IO_Init();
    Oscillator_Init();
    Interrupts_Init();
}
