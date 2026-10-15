/*-------------------------------------------------------------------------

silabs_gen.h

M‰‰rittelee SiLabsin kontrollerien erikoisuudet geneerisiksi (RI0 -> RI, jne.)
Ei tarvita F330:n kanssa.

JK 19.6.2009

-------------------------------------------------------------------------*/

#ifndef SILABS_GEN_H
#define SILABS_GEN_H

#define RI          RI0
#define TI          TI0
#define SBUF        SBUF0
#define ES          ES0
#define PS          PS0

#define LEGACY_PAGE 0x00
#define CONFIG_PAGE 0x0F

#define WDTE        0x40

/* Keskeytysvektorien (uudelleen)m‰‰rittely. F330.h m‰‰rittelee n‰m‰ eri
   nimille kuin F360.h. N‰m‰ nimet ovat F330.h:n mukaiset, joten koodin
   siirtely F330 <-> F361 on helpompaa. */

/* Interrupt numbers: address = (number * 8) + 3 */
#define IE0_VECTOR      0       /* 0x03 external interrupt 0 */
#define TF0_VECTOR      1       /* 0x0b timer 0 */
#define IE1_VECTOR      2       /* 0x13 external interrupt 1 */
#define TF1_VECTOR      3       /* 0x1b timer 1 */
#define UART0_VECTOR    4       /* 0x23 serial port 0 */
#define TF2_VECTOR      5       /* 0x2b timer 2 */
#define SPI_VECTOR      6       /* 0x33 SPI */
#define SMB0_VECTOR     7       /* 0x3B SMB/I2C */
#define ADC0W_VECTOR    9       /* 0x4B ADC0 Window compare*/
#define ADC0_VECTOR    10       /* 0x53 ADC0 */
#define PCA0_VECTOR    11       /* 0x5B PCA0 */
#define CMP0_VECTOR    12       /* 0x63 Comparator0 */
#define CMP1_VECTOR    13       /* 0x6B Comparator1 */
#define TF3_VECTOR     14       /* 0x73 timer 3 */
#define MAT_VECTOR     16       /* 0x83 port match */

#endif
