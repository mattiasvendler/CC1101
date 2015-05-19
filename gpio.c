/**
 * Contains GPIO initialization functions as well as GPIO read and write functions.
 * Created on: May 12, 2015
 *      Author: Stefan, Alex
 */

#define CHIP_6713 1

#include <csl_gpio.h>
#include "dsk6713.h"
#include "dsk6713_led.h"
#include "gpio.h"

// ------------------------------------------
// start of variables
// ------------------------------------------

GPIO_Handle    hGpio; /* GPIO handle */

int led_prev=0;//not sure how to check led state so just keep local copy

// ------------------------------------------
// end of variables
// ------------------------------------------

void toggle_LED(int led)
{
	if(led_prev){
		DSK6713_LED_off(led);
		led_prev = 0;
	}else{
		DSK6713_LED_on(led);
		led_prev=1;
	}
}

void gpioInit()
{
	//--------------NOTE------------------
	//	FOR GPIOs TO WORK ON C6713 DSK SPECTRUM DIGITAL BOARD
	//	SWITCH 4 OF THE DIPSWITCH SW3 (NOT SW1!!!) HAS TO BE ON-CLOSED
	//--------------NOTE------------------

	GPIO_Config gpio_config = {
	0x00000000, /* gpgc */
	0x0000FFFF, /* gpen --*/
	0x00000000, /* gdir -*/
	0x00000000, /* gpval */
	0x00000000, /* gphm all interrupts disabled for io pins */
	0x00000000, /* gplm all interrupts to cpu or edma disabled  */
	0x00000000  /* gppol -- default state */
	};

	hGpio  = GPIO_open( GPIO_DEV0, GPIO_OPEN_RESET );

	GPIO_config(hGpio  , &gpio_config );

	/* Enables pins */
	GPIO_pinEnable (hGpio,
			GPIO_PIN0 |
			GPIO_PIN1 |
			GPIO_PIN2 |
			GPIO_PIN3 |
			GPIO_PIN4 |
			GPIO_PIN5 |
			GPIO_PIN6 |
			GPIO_PIN7 |
			GPIO_PIN8 |
			GPIO_PIN9 |
			GPIO_PIN13);//enable here or in MyConfig

	GPIO_pinDirection(hGpio,GPIO_PIN0, GPIO_OUTPUT);	// CS
	GPIO_pinDirection(hGpio,GPIO_PIN1, GPIO_OUTPUT);
	GPIO_pinDirection(hGpio,GPIO_PIN2, GPIO_OUTPUT);
	GPIO_pinDirection(hGpio,GPIO_PIN3, GPIO_OUTPUT);	// MOSI
	GPIO_pinDirection(hGpio,GPIO_PIN13, GPIO_OUTPUT);	// SCK

	GPIO_pinDirection(hGpio,GPIO_PIN8, GPIO_INPUT);	// MISO
	GPIO_pinDirection(hGpio,GPIO_PIN9, GPIO_INPUT);	// GDO0


}

void gpioToggle()
{
	while(1)
	{
		GPIO_pinWrite( hGpio, GPIO_PIN0, 0 );
		GPIO_pinWrite( hGpio, GPIO_PIN1, 0 );
		*((int*)GPIO_VALUE_ADDRESS) = 255;
		DSK6713_waitusec(500000);
		GPIO_pinWrite( hGpio, GPIO_PIN0, 1 );
		GPIO_pinWrite( hGpio, GPIO_PIN1, 1 );
		*((int*)GPIO_VALUE_ADDRESS) = 0;
		DSK6713_waitusec(500000);


	}
}

void ToggleDebugGPIO(short IONum){
	if (IONum == 0){
		GPIO_pinWrite(hGpio,GPIO_PIN0,1);
		GPIO_pinWrite(hGpio,GPIO_PIN0,0);
	}
	else if(IONum == 1){
		GPIO_pinWrite(hGpio,GPIO_PIN1, 1);
		GPIO_pinWrite(hGpio,GPIO_PIN1, 0);
	}
	else if(IONum == 2){
		GPIO_pinWrite(hGpio,GPIO_PIN2, 1);
		GPIO_pinWrite(hGpio,GPIO_PIN2, 0);
	}
	else if(IONum == 3){
		GPIO_pinWrite(hGpio,GPIO_PIN3, 1);
		GPIO_pinWrite(hGpio,GPIO_PIN3, 0);
	}
	else{
		//error
	}
}

void cc1101_Sel(int gpio, int value)
{
	// for instance you call
	// GPIO_pinWrite( hGpio, GPIO_PIN0, 1 );
	GPIO_pinWrite( hGpio, gpio, value );
}

