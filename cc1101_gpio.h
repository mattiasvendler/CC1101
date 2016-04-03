/*
 * gpio.h
 *
 *  Created on: Apr 18, 2015
 *      Author: Stefan
 */

#ifndef GPIO_H_
#define GPIO_H_

#endif /* GPIO_H_ */
#define GPIO_0 0
#define GPIO_2 1
#define LED_1 2
#define LED_2 3
#define SPI_CHANNEL 0

//gpio registers
#define GPIO_ENABLE_ADDRESS		0x01B00000
#define GPIO_DIRECTION_ADDRESS	0x01B00004
#define GPIO_VALUE_ADDRESS		0x01B00008

// function prototypes
void ToggleDebugGPIO(short IONum);


//debug gpio function
void gpioInit(void);
void gpioToggle(void);
void cc1101_Sel(int gpio, int value);
void wait_Miso(void);
void cc1101_Select(void);
void cc1101_Deselect(void);
void wait_GDO0_high(void);
void wait_GDO0_low(void);
//SPI interface
void spiWriteReg(const unsigned char regAddr, const unsigned char regData);
unsigned char spiReadReg (const unsigned char regAddr);
unsigned char spiReadRegData (const unsigned char regAddr);
void spiWriteAddr(const unsigned char regAddr);
void spiWriteData(const unsigned char regData);
unsigned char spiReadData (void);
void spiBurstWrite(const unsigned char regAddr,
			const unsigned char data[], int len);
void spiBurstRead(const unsigned char regAddr, unsigned char data[], int len);

