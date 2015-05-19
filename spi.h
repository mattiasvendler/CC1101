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

typedef char byte;
typedef char boolean;

#ifndef _SPI_H
#define _SPI_H

//#include "Arduino.h"
#include "serial_loop.h"
#include <csl_gpio.h>
#include "gpio.h"
#include <stdint.h>

/**
 * SPI pins
 */
#define SPI_SS   10     // PB2 = SPI_SS
#define SPI_MOSI 11     // PB3 = MOSI
#define SPI_MISO 12     // PB4 = MISO
#define SPI_SCK  13     // PB5 = SCK
#define GDO0     2        // PD2 = INT0

#define PORT_SPI_MISO  PINB
#define BIT_SPI_MISO  4

#define PORT_SPI_SS  PORTB
#define BIT_SPI_SS   2

#define PORT_GDO0  PIND
#define BIT_GDO0  2

//current PIN DEFINITONS ARE FILLER! Double check and then wipe this comment
#ifdef USE_CC1101
#define SPI_SS_BB		GPIO_PIN0
#define SPI_SCLK_BB		GPIO_PIN0
#define SPI_SDI_BB		GPIO_PIN0
#define SPI_SDO_BB		GPIO_PIN0
#define GDO0_BB			GPIO_PIN0

#define SPI_SS_BB_DIR	GPIO_OUTPUT
#define SPI_SCLK_BB_DIR	GPIO_OUTPUT
#define SPI_SDI_BB_DIR	GPIO_INPUT
#define SPI_SDO_BB_DIR	GPIO_OUTPUT
#define GD0_BB_DIR		GPIO_INPUT
#endif

#ifdef USE_NRF24L01
#define SPI_SS_BB		GPIO_PIN0
#define SPI_SCLK_BB		GPIO_PIN0
#define SPI_SDI_BB		GPIO_PIN0
#define SPI_SDO_BB		GPIO_PIN0
#define CE_BB			GPIO_PIN0
#define IRQ_BB			GPIO_PIN0

#define SPI_SS_BB_DIR	GPIO_OUTPUT
#define SPI_SCLK_BB_DIR	GPIO_OUTPUT
#define SPI_SDI_BB_DIR	GPIO_INPUT
#define SPI_SDO_BB_DIR	GPIO_OUTPUT
#define CE_BB_DIR		GPIO_OUTPUT
#define IRQ_BB_DIR		GPIO_INPUT
#endif

/**
 * Macros
 */
// Wait until SPI operation is terminated
#define wait_Spi()  while(!(SPSR & _BV(SPIF)))

byte spi_send(byte value);

/*
 * Bit bang operation version of the SPI system
 */
void spi_bb_init(); //sets up pins and sets em to default values
byte spi_read_write_bb(byte value);
byte spi_read_bb();

void Radio_Select_bb();	//sets the chip select line
void Radio_UnSelect_bb(); 	//unsets the chip select line

byte CC1101_GD0_status_huh(); //gets status of GD0 line (1 or 0)
byte CC1101_GD1_status_huh();

/**
 * Class: SPI
 *
 * Description:
 * Basic SPI class
 */
//class SPI
//{
//  public:
//    /**
//     * init
//     *
//     * SPI initialization
//     */
//    void init();
//
//    /**
//     * send
//     *
//     * Send byte via SPI
//     *
//     * 'value'  Value to be sent
//     *
//     * Return:
//     *  Response received from SPI slave
//     */
//    byte send(byte value);
//};
#endif
