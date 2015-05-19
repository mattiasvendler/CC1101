/*
/**************************************************************************************
 *
 * spi.c
 *
 * Contains the source code for communicating
 * with the MAX7456 using a bit-banged SPI port
**************************************************************************************/

// INCLUDES ***********************************************

//#include "bitbanging_iomaxq200x.h"        // bit-banging definitions not in standard <iomaxq200x.h>
//#include <intrinsics.h>
//#include "spi.h"
//#include "max7456.h"
#include <csl_gpio.h>
#include "gpio.h"

// SPI ports defined using macros to enable pins to be changed around as required
//#define SPI_CS      PO5_bit.bit4          // PO5_bit.bit4 = /CS - chip select
//#define SPI_MOSI    PO5_bit.bit5          // PO5_bit.bit5 = MOSI - master out slave in, data to MAX7456
//#define SPI_MISO    PI5_bit.bit7          // PO5_bit.bit7 = MISO - master in slave out, data from MAX7456
//#define SPI_CK      PO5_bit.bit6          // PO5_bit.bit6 = SCK - SPI clock

#define SPI_CK(x)		GPIO_pinWrite(hGpio,GPIO_PIN13,x)
#define SPI_MOSI(x)		GPIO_pinWrite(hGpio,GPIO_PIN3,x)
#define SPI_MISO		GPIO_pinRead(hGpio, GPIO_PIN8)


// VARIABLES **********************************************

//extern volatile union {
//  unsigned char flags;
//  struct
//  {
//    unsigned char sci0RxMsg    : 1;
//    unsigned char sci0RxEsc    : 1;
//    unsigned char sci0RxLenLSB : 1;
//    unsigned char sci0RxLenMSB : 1;
//    unsigned char error        : 1;
//  } flags_bit;
//};

//extern volatile unsigned char data[DATA_BUF_LENGTH];

extern GPIO_Handle    hGpio; /* GPIO handle */

/**************************************************************************************
 * spiWriteReg
 *
 * Writes to an 8-bit register with the SPI port
 **************************************************************************************/

void spiWriteReg(const unsigned char regAddr, const unsigned char regData)
{

  unsigned char SPICount;                               // Counter used to clock out the data

  unsigned char SPIData;                                // Define a data structure for the SPI data.

  //SPI_CS = 1;                                           // Make sure we start with /CS high
  SPI_CK(0);                                           // and CK low (idle low)

  SPIData = regAddr;                                    // Preload the data to be sent with Address
  //SPI_CS = 0;                                           // Set /CS low to start the SPI cycle 25nS
                                                        // Although SPIData could be implemented as an "int", resulting in one
                                                        // loop, the routines run faster when two loops are implemented with
                                                        // SPIData implemented as two "char"s.

  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Address byte
  {
    if (SPIData & 0x80)                                 // Check for a 1
      SPI_MOSI(1);                                     // and set the MOSI line appropriately
    else
      SPI_MOSI(0);
    SPI_CK(1);                                         // Toggle the clock line
    SPI_CK(0);
    SPIData <<= 1;                                      // Rotate to get the next bit

  }                                                     // and loop back to send the next bit
                                                        // Repeat for the Data byte
  SPIData = regData;                                    // Preload the data to be sent with Data
  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Data
  {
    if (SPIData & 0x80)
      SPI_MOSI(1);
    else
      SPI_MOSI(0);
    SPI_CK(1);
    SPI_CK(0);
    SPIData <<= 1;
  }
  //SPI_CS = 1;
  SPI_MOSI(0);

}

/**************************************************************************************
 * spiWriteAddr
 *
 * Writes to an 8-bit register with the SPI register address
 **************************************************************************************/

void spiWriteAddr(const unsigned char regAddr)
{
	  unsigned char SPICount;                               // Counter used to clock out the data

	  unsigned char SPIData;                                // Define a data structure for the SPI data.

	  //SPI_CS = 1;                                           // Make sure we start with /CS high
	  SPI_CK(0);                                           // and CK low (idle low)

	  SPIData = regAddr;                                    // Preload the data to be sent with Address
	  //SPI_CS = 0;                                           // Set /CS low to start the SPI cycle 25nS
	                                                        // Although SPIData could be implemented as an "int", resulting in one
	                                                        // loop, the routines run faster when two loops are implemented with
	                                                        // SPIData implemented as two "char"s.

	  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Address byte
	  {
	    if (SPIData & 0x80)                                 // Check for a 1
	      SPI_MOSI(1);                                     // and set the MOSI line appropriately
	    else
	      SPI_MOSI(0);
	    SPI_CK(1);                                         // Toggle the clock line
	    SPI_CK(0);
	    SPIData <<= 1;                                      // Rotate to get the next bit

	  }
}

/**************************************************************************************
 * spiWriteData
 *
 * Writes to an 8-bit register with the SPI register data
 **************************************************************************************/

void spiWriteData(const unsigned char regData)
{
	  unsigned char SPICount;                               // Counter used to clock out the data

	  unsigned char SPIData;                                // Define a data structure for the SPI data.

	  //SPI_CS = 1;                                           // Make sure we start with /CS high
	  SPI_CK(0);                                           // and CK low (idle low)

	  SPIData = regData;                                    // Preload the data to be sent with Data
	  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Data
	  {
	    if (SPIData & 0x80)
	      SPI_MOSI(1);
	    else
	      SPI_MOSI(0);
	    SPI_CK(1);
	    SPI_CK(0);
	    SPIData <<= 1;
	  }
	  //SPI_CS = 1;
	  SPI_MOSI(0);
}

///**************************************************************************************
// * spiWriteRegAutoIncr
// *
// * Writes to an 8-bit register with the SPI port
// * using the MAX7456's auto-increment mode
// **************************************************************************************/
//
//void spiWriteRegAutoIncr(const unsigned char regData)
//{
//  unsigned char SPICount;                               // Counter used to clock out the data
//
//  unsigned char SPIData;                                // Define a data structure for the SPI data.
//
//  SPI_CS = 1;                                           // Make sure we start with /CS high
//  SPI_CK = 0;                                           // and CK low
//  SPIData = regData;                                    // Preload the data to be sent with Address & Data
//
//  SPI_CS = 0;                                           // Set /CS low to start the SPI cycle
//  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Address & Data
//  {
//    if (SPIData & 0x80)
//      SPI_MOSI = 1;
//    else
//      SPI_MOSI = 0;
//    SPI_CK = 1;
//    SPI_CK = 0;
//    SPIData <<= 1;
//  }                                                     // and loop back to send the next bit
//  SPI_MOSI = 0;                                         // Reset the MOSI data line
//}

/**************************************************************************************
 * spiReadReg
 *
 * Reads an 8-bit register with the SPI port.
 * Data is returned.
 **************************************************************************************/

unsigned char spiReadReg (const unsigned char regAddr)
{

  unsigned char SPICount;                               // Counter used to clock out the data

  unsigned char SPIData;

  //SPI_CS = 1;                                           // Make sure we start with /CS high
  SPI_CK(0);                                           // and CK low
  SPIData = regAddr;                                    // Preload the data to be sent with Address & Data

  //SPI_CS = 0;                                           // Set /CS low to start the SPI cycle
  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock out the Address & Data
  {
    if (SPIData & 0x80)
      SPI_MOSI(1);
    else
      SPI_MOSI(0);
    SPI_CK(1);
    SPI_CK(0);
    SPIData <<= 1;
  }                                                     // and loop back to send the next bit
  SPI_MOSI(0);                                         // Reset the MOSI data line

  SPIData = 0;
  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock in the data to be fread
  {
    SPIData <<=1;                                       // Rotate the data
    SPI_CK(1);                                         // Raise the clock to clock the data out of the MAX7456
    SPIData += SPI_MISO;                                // Read the data bit
    SPI_CK(0);                                         // Drop the clock ready for th enext bit
  }                                                     // and loop back
  //SPI_CS = 1;                                           // Raise CS

  return ((unsigned char)SPIData);                      // Finally return the read data
}

/**************************************************************************************
 * spiReadData
 *
 * Reads an 8-bit register with the SPI port, without send the address register first.
 * Data is returned.
 **************************************************************************************/

unsigned char spiReadData ()
{

  unsigned char SPICount;                               // Counter used to clock out the data

  unsigned char SPIData;

  //SPI_CS = 1;                                           // Make sure we start with /CS high
  SPI_CK(0);                                           // and CK low

  SPI_MOSI(0);                                         // Reset the MOSI data line

  SPIData = 0;
  for (SPICount = 0; SPICount < 8; SPICount++)          // Prepare to clock in the data to be fread
  {
    SPIData <<=1;                                       // Rotate the data
    SPI_CK(1);                                         // Raise the clock to clock the data out of the MAX7456
    SPIData += SPI_MISO;                                // Read the data bit
    SPI_CK(0);                                         // Drop the clock ready for th enext bit
  }                                                     // and loop back
  //SPI_CS = 1;                                           // Raise CS

  return ((unsigned char)SPIData);                      // Finally return the read data
}

///**************************************************************************************
// * spiWriteCM
// *
// * Writes to the Display Memory (960 bytes) from "data" extern.
// * 960 = 16 rows * 30 columns * 2 planes {char vs attr} screen-position-indexed memory
// **************************************************************************************/
//
//void spiWriteCM()                                       // On entry: global data[1..960] contains char+attr bytes
//                                                        // (optionally terminated by 0xFF data)
//                                                        // First, write data[1,3,5,...] Character plane;
//                                                        // MAX7456 WriteReg(0x05,0x41)
//                                                        //"Character Memory Address High"; 0x02:Attribute bytes;
//                                                        // 0x01:character memory address msb
//{
//   volatile unsigned int Index = 0x0001;                // Index for lookup into data[1..960]
//   spiWriteReg(DM_ADDRH_WRITE,0x00);                    // initialise the Display Memory high-byte
//   spiWriteReg(DM_ADDRL_WRITE,0x00);                    // and the low-byte
//   spiWriteReg(DM_MODE_WRITE ,0x41);                    // MAX7456 WriteReg(0x04,0x41) "Display Memory Mode";
//                                                        // 0x40:Perform 8-bit operation; 0x01:Auto-Increment
//
//   do                                                   // Loop to write the character data
//   {
//      if (data[Index] == 0xFF) {                        // Check for the break character
//           break; }                                     // and finish if found
//      spiWriteRegAutoIncr(data[Index]);                 // Write the character
//      Index += 2;                                       // Increment the index to the next character, skipping over
//                                                        // the attribute.
//   } while(Index < 0x03C1); // 0x03C1 = 961             // And loop back to send the next character
//
//   spiWriteRegAutoIncr(0xFF);                           // "Escape Character" to end Auto-Increment mode
//
//   spiWriteReg(DM_ADDRH_WRITE,0x02);                    // Second, write data[2,4,6,...] Attribute plane; MAX7456
//                                                        // WriteReg(0x05,0x41) "Character Memory Address High";
//                                                        // 0x02:Attribute bytes; 0x01:character memory address msb
//   spiWriteReg(DM_ADDRL_WRITE,0x00);
//   spiWriteReg(DM_MODE_WRITE ,0x41);                    // MAX7456 WriteReg(0x04,0x41) "Character Memory Mode";
//                                                        // 0x40:Perform 8-bit operation; 0x01:Auto-Increment
//   Index = 0x0002;
//   do
//   {
//      if (data[Index] == 0xFF)
//         break;
//      spiWriteRegAutoIncr(data[Index]);
//      Index += 2;
//   } while(Index < 0x03C1);
//   spiWriteRegAutoIncr(0xFF);
//}
//
///**************************************************************************************
// * spiWriteFM
// *
// * Writes to the Character Memory (54 bytes) from "data" extern
//**************************************************************************************/
//void spiWriteFM()
//{
//     unsigned char Index;
//
//     spiWriteReg(VIDEO_MODE_0_WRITE,spiReadReg
//                 (VIDEO_MODE_0_READ)&0xF7);             // Clear bit 0x08 to DISABLE the OSD display
//     spiWriteReg(FM_ADDRH_WRITE,data[1]);               // Write the address of the character to be written
//                                                  	// MAX7456 glyph tile definition length = 0x36 = 54 bytes
//                                                        // MAX7456 64-byte Shadow RAM accessed
//                                                        // through FM_DATA_.. FM_ADDR.. contains a single
//                                                        // character/glyph-tile shape
//     for(Index=0x00; Index<0x36; Index++)
//     {
//          spiWriteReg(FM_ADDRL_WRITE,Index);            // Write the address within the shadow RAM
//          spiWriteReg(FM_DATA_IN_WRITE,data[Index+2]);  // Write the data to the shaddow RAM
//     }
//
//     spiWriteReg(FM_MODE_WRITE,0xA0);                   // MAX7456 "Font Memory Mode" write 0xA0 triggers
//                                                        // copy from 64-byte Shadow RAM to NV array.
//
//     while ((spiReadReg(STATUS_READ) & 0x20) != 0x00);  // Wait while NV Memory status is BUSY
//                                                	// MAX7456 0xA0 status bit 0x20: NV Memory Status Busy/~Ready
//
//}

/* bitbang_spi.c
 *
 *  Created on: May 18, 2015
 *      Author: IEUser
 */


