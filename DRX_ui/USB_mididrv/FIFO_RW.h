/*
 *   FIFO_RW.h
 */

#ifndef FIFO_RW_H
#define FIFO_RW_H

extern void FIFO_Read_idata( BYTE fifo_adr, BYTE n, BYTE __idata * ptr );
extern void FIFO_Read_pdata( BYTE fifo_adr, BYTE n, BYTE __pdata * ptr );
extern void FIFO_Read_xdata( BYTE fifo_adr, BYTE n, BYTE __xdata * ptr );
extern void FIFO_Write_idata( BYTE fifo_adr, BYTE n, BYTE __idata * ptr  );
extern void FIFO_Write_pdata( BYTE fifo_adr, BYTE n, BYTE __pdata * ptr  );
extern void FIFO_Write_xdata( BYTE fifo_adr, BYTE n, BYTE __xdata * ptr  );
extern void FIFO_Write_code( BYTE fifo_adr, BYTE n, BYTE __code * ptr  );

#endif	/* FIFO_RW_H */
