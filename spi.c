/**
 * Copyright (c) 2011 panStamp <contact@panstamp.com>
 *
 * This file is part of the panStamp project.
 *
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version.
 *
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 *
 * Author: Daniel Berenguer
 * Creation date: 03/03/2011
 */

#include "spi.h"
//#include "bitbang_spi.h"
#include <csl_mcbsp.h>

MCBSP_Handle extern hMcbsp;
/**
 * init
 *
 * SPI initialization
 */
void spi_init()
{
//  digitalWrite(SPI_SS, HIGH);
//
//  // Configure SPI pins
//  pinMode(SPI_SS, OUTPUT);
//  pinMode(SPI_MOSI, OUTPUT);
//  pinMode(SPI_MISO, INPUT);
//  pinMode(SPI_SCK, OUTPUT);
//
//  digitalWrite(SPI_SCK, HIGH);
//  digitalWrite(SPI_MOSI, LOW);
//
//  // SPI speed = clk/4
//  SPCR = _BV(SPE) | _BV(MSTR);

// add SPI init for C6713 here

}

/**
 * send
 *
 * Send byte via SPI
 *
 * 'value'      Value to be sent
 *
 * Return:
 *      Response received from SPI slave
 */
byte spi_send(byte value)
{
//  SPDR = value;                          // Transfer byte via SPI
//  wait_Spi();                            // Wait until SPI operation is terminated
//
//  return SPDR;

//	/* wait until the transmitter is ready for a sample then write to it */
//	while (!MCBSP_xrdy(hMcbsp));
//	MCBSP_write(hMcbsp,value);
//	/* wait for data to shift out */
//	while (!MCBSP_xempty(hMcbsp));
//
//	/* now wait until the value is received then read it */
//	while (!MCBSP_rrdy(hMcbsp));
//	return MCBSP_read(hMcbsp);

	char byte_read=-1;

	/* wait until the transmitter is ready for a sample then write to it */
	//while (!MCBSP_xrdy(hMcbsp));
	MCBSP_write(hMcbsp,value);
//	DSK6713_waitusec(10);
//	MCBSP_write(hMcbsp,0x00);
//	DSK6713_waitusec(20);
//	byte_read = MCBSP_read(hMcbsp);
	byte_read = MCBSP_read(hMcbsp);
	/* wait for data to shift out */
	//while (!MCBSP_xempty(hMcbsp));


	//cc1101_Sel(GPIO_PIN0,1);

//
//
//	//cc1101_Sel(GPIO_PIN0,0);
//	//wait_Miso();
//	/* wait until the transmitter is ready for a sample then write to it */
//	while (!MCBSP_xrdy(hMcbsp));
//	MCBSP_write(hMcbsp,0x00);
//	byte_read = MCBSP_read(hMcbsp);
//	/* wait for data to shift out */
//	while (!MCBSP_xempty(hMcbsp));
//	DSK6713_waitusec(15);

	return byte_read;

}


void spi_bb_init(){

//	gpioInit(); //turn on GPIO pins 0-10
//
////use 433-915 MHz TI modules
//#ifdef USE_CC1101
//
//	/* Set GPIO Pin directions according to pin defs in spi.h */
//	//todo set up pin directions here
//	GPIO_pinDirection(hGpio, SPI_SS_BB, SPI_SS_BB_DIR);
//	GPIO_pinDirection(hGpio, SPI_SCLK_BB, SPI_SCLK_BB_DIR);
//	GPIO_pinDirection(hGpio, SPI_SDI_BB, SPI_SDI_BB_DIR);
//	GPIO_pinDirection(hGpio, SPI_SDO_BB, SPI_SDO_BB_DIR);
//	GPIO_pinDirection(hGpio, GDO0_BB, GDO0_BB_DIR);
//
//
//	/* Set GPIO pins to default states for the system */
//	//todo write default states here
//
//	//bitbang stuff is ready to go
//
//
//#endif
//
////use 2.4GHz nordic semi modules
//#ifdef USE_NRF24L01
//	/* Set GPIO Pin directions according to pin defs in spi.h */
//	//todo set up pin directions here
////	GPIO_pinDirection(hGpio, SPI_SS_BB, SPI_SS_BB_DIR);
////	GPIO_pinDirection(hGpio, SPI_SCLK_BB, SPI_SCLK_BB_DIR);
////	GPIO_pinDirection(hGpio, SPI_SDI_BB, SPI_SDI_BB_DIR);
////	GPIO_pinDirection(hGpio, SPI_SDO_BB, SPI_SDO_BB_DIR);
////	GPIO_pinDirection(hGpio, GDO0_BB, GDO0_BB_DIR);
//
//
//	/* Set GPIO pins to default states for the system */
//	//todo write default states here
//
//	//bitbang stuff is ready to go
//#endif

}

byte spi_read_write_bb(byte value){


}

byte spi_read_bb(){

}

//sets the chip select line
void Radio_Select_bb(){

}
//unsets the chip select line
void Radio_UnSelect_bb(){

}

//gets status of GD0 line (1 or 0)
byte CC1101_GD0_status_huh(){
}
byte CC1101_GD1_status_huh(){

}
