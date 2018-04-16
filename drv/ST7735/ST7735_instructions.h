/*
 * ST7735_instructions.h
 *
 *  Created on: Jan 21, 2018
 *      Author: chris
 */

#ifndef DRV_ST7735_ST7735_INSTRUCTIONS_H_
#define DRV_ST7735_ST7735_INSTRUCTIONS_H_

//
// System Function Commands
//
// #define INSTRUCTION CODE /* Description (bytes in/out) (is default) */

#define ST7735_NOP     0x00 /* No Operation (0) */
#define ST7735_SWRESET 0x01 /* Software Reset (0) */
#define ST7735_RDDID   0x04 /* Read ID (3 out) */
#define ST7735_RDDST   0x09 /* Read Status (4 out) */
#define ST7735_RDDPM   0x0A /* Read Display Power Mode (1 out) */
#define ST7735_RDDMADCTL 0x0B /* Read Display MADCTL (1 out) */
#define ST7735_RDDCOLMOD 0x0C /* Read Display Pixel Format (1 out) */
#define ST7735_RDDIM   0x0D /* Read Display Image Mode (1 out) */
#define ST7735_RDDSM   0x0E /* Read Display Signal Mode (1 out) */
#define ST7735_RDDSDR  0x0F /* Read Display Self-diagnostic Result (1 out) */

#define ST7735_SLPIN   0x10 /* Sleep In & Booster Off (0)*/
#define ST7735_SLPOUT  0x11 /* Sleep Out & Booster On (0)*/
#define ST7735_PTLON   0x12 /* Partial Mode On (0) */
#define ST7735_NORON   0x13 /* Partial Off (0) (Default) */

#define ST7735_INVOFF  0x20 /* Display Inversion Off (0) (Default) */
#define ST7735_INVON   0x21 /* Display Inversion On (0) */
#define St7735_GAMSET  0x26 /* Gamma Curve Select (1 in) */

#define ST7735_DISPOFF 0x28 /* Display Off (0) */
#define ST7735_DISPON  0x29 /* Display On (0) */
#define ST7735_CASET   0x2A /* Column Address Set (4 in) */
#define ST7735_RASET   0x2B /* Row Address Set (4 in) */
#define ST7735_RAMWR   0x2C /* Memory Write (1 in) */
#define ST7735_RGBSET  0x2D /* LUT for 4k, 65k, 262k Color Display (3*N in) */
#define ST7735_RAMRD   0x2E /* Memory Read (1 out) */

#define ST7735_PTLAR   0x30 /* Partial Start/End Address Set (4 in) */
#define ST7735_SCRLAR  0x33 /* Scroll area set (6 in) */

#define ST7735_TEOFF   0x34 /* Tearing effect line off (0) */
#define ST7735_TEON    0x35 /* Tearing Effect Mode Set & on (1 in) */
#define ST7735_MADCTL  0x36 /* Memory Data Access Control (1 in) */

#define ST7735_VSCSAD  0x37 /* Scroll Start Address of RAM (2 in) */

#define ST7735_IDMOFF  0x38 /* Idle Mode Off (0) */
#define ST7735_IDMON   0x39 /* Idle Mode On (0) */
#define ST7735_COLMOD  0x3A /* Interface Pixel Format (1 in) */

#define ST7735_RDID1   0xDA /* Read ID1 (1 out) */
#define ST7735_RDID2   0xDB /* Read ID2 (1 out) */
#define ST7735_RDID3   0xDC /* Read ID3 (1 out) */



//
// Panel Function Commands
//

#define ST7735_FRMCTR1 0xB1 /* Normal/Full Color Mode (3 in) */
#define ST7735_FRMCTR2 0xB2 /* Idle Mode (3 in) */
#define ST7735_FRMCTR3 0xB3 /* Partial Mode + Full Colors (6 in) */
#define ST7735_INVCTR  0xB4 /* Display Inversion Control (1 in) */

#define ST7735_PWCTR1  0xC0 /* Power Control Setting (3 in) */
#define ST7735_PWCTR2  0xC1 /* Power Control Setting (1 in) */
#define ST7735_PWCTR3  0xC2 /* In Normal/Full Color Mode (2 in) */
#define ST7735_PWCTR4  0xC3 /* In Idle Mode (2 in) */
#define ST7735_PWCTR5  0xC4 /* In Partial Mode + Full colors (2 in) */
#define ST7735_VMCTR1  0xC5 /* VCOM Control 1 (1 in) */
#define ST7735_VMOFCTR 0xC7 /* Set VCOM Offset Control (1 in) */

#define ST7735_WRID2   0xD1 /* Set LCM Version Code (1 in) */
#define ST7735_WRID3   0xD2 /* Customer Project Code (1 in) */

#define ST7735_NVCTR1  0xD9 /* NVM Control Status (1 in) */
#define ST7735_NVCTR2  0xDE /* NVM Read Command (2 in) */
#define ST7735_NVCTR3  0xDF /* NVM Write Command Action Code (2 in) */

#define ST7735_GMCTRP1 0xE0 /* Gamma Adjustment (+ Polarity) (16 in) */
#define ST7735_GMCTRN1 0xE1 /* Gamma Adjustment (- Polarity) (16 in) */

#define ST7735_GCV     0xFC /* Gate Clock Variable (1 in) */


#endif /* DRV_ST7735_ST7735_INSTRUCTIONS_H_ */
