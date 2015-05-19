/*
 * bitbang_spi.h
 *
 *  Created on: May 18, 2015
 *      Author: IEUser
 */

#ifndef BITBANG_SPI_H_
#define BITBANG_SPI_H_

void spiWriteReg(const unsigned char regAddr, const unsigned char regData);
unsigned char spiReadReg (const unsigned char regAddr);

void spiWriteAddr(const unsigned char regAddr);
void spiWriteData(const unsigned char regData);
unsigned char spiReadData ();

#endif /* BITBANG_SPI_H_ */
