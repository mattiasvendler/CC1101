/**
 * spi.c
 *
 * Contains the source code for communicating
 * with the CC1101 radio using a bit-banged SPI port
 * However, this SPI implementatio is generic enough for
 * use with other SPI devices.
 *   Created on: May 12, 2015
 *   Author: Stefan
*/

// INCLUDES ***********************************************

#include <csl_gpio.h>
#include "gpio.h"

#define SPI_CK(x)		GPIO_pinWrite(hGpio,GPIO_PIN13,x)
#define SPI_MOSI(x)		GPIO_pinWrite(hGpio,GPIO_PIN3,x)
#define SPI_MISO		GPIO_pinRead(hGpio, GPIO_PIN8)


// VARIABLES **********************************************

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


/* bitbang_spi.c
 *
 *  Created on: May 18, 2015
 *      Author: Stefan Gefa
 */


