/*
 * gpio.h
 *
 *  Created on: Apr 18, 2015
 *      Author: IEUser
 */

#ifndef GPIO_H_
#define GPIO_H_



#endif /* GPIO_H_ */

//gpio registers
#define GPIO_ENABLE_ADDRESS		0x01B00000
#define GPIO_DIRECTION_ADDRESS	0x01B00004
#define GPIO_VALUE_ADDRESS		0x01B00008

// function prototypes
void ToggleDebugGPIO(short IONum);


//debug gpio function
void gpioInit();
void gpioToggle();
void cc1101_Sel(int gpio, int value);
void wait_Miso();
void cc1101_Select();
void cc1101_Deselect();
void wait_GDO0_high();
void wait_GDO0_low();
