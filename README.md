# CC1101
CC1101 driver for TMS320C6713 DSK

CC1101 driver for C6713 DSK

(Co)Author: Stefan Gvozdenovic
gefa12 et gmail dot com
Creation date: 05/19/2015

Description: The Code is based on the panstamp CC1101 library for Arduino. This code
demonstrates transmission and reception (without interrupts) of
arbitrary data length/type from the CC1101 radio chip attached to the
GPIO pins of the 6713 through J1 extension header. The SPI between radio
and 6713 is implemented by bit-banging the GPIO. Two C6713 DSKs and two CC1101
radios are needed for this demo.


CC1101 Sub-1GHz Transceiver
Current radio settings for the radio can be found in the cc1101.h file. To change these 
settings simply copy/paste the exported registers from smartRF studio.
Basic current settings:
* Base frequency:     315MHz
* Modulation format:  GFSK
* Data rate:          50kBaud
* Deviation:          25kHz
* Channel number:     0
* Channel spacing:    200KHz
* TX power:           12dBm
                

GPIOs that bit-bang SPI are defined in bitbang_spi.h header file.
* SCK	GPIO_13
* MOSI	GPIO_3
* MISO	GPIO_8
* CS	GPIO_0
* GDO0	GPIO_9

More about J1 expansion header: [C6713 DSK Technical Reference](http://c6000.spectrumdigital.com/dsk6713/revc/files/6713_dsk_techref.pdf#page=28)

More about the RF Transceiver Module RF1101SE pinout: [Erwan's blog](http://labalec.fr/erwan/?p=497)

