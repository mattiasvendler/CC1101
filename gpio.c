/*************************************************************************
 *  Basic stereo loop code for C6713 DSK and AIC23 codec
 *  D. Richard Brown on 22-Aug-2011
 *  Based on code from "Real-Time Digital Signal Processing Based on TMS320C6000"
 *  by N. Kehtarnavaz and N. Kim.
 *
 *  Timestamping code
 *    - three states:
 *      - state 0: searching
 *      - state 1: recording
 *      - state 2: timestamp calculation
 *		- state 3: sinc response
 *    - looks for modulated sinc pulse
 *    - correlates to find nearest sample
 *    - uses carrier phase to refine timestamp
 *
 *	Current Revision: 	0.1
 *	Revision Name:		Hurr durr I'ma sheep
 *************************************************************************/

//Because
#define CHIP_6713 1


#include <stdio.h>					//For printf
#include <c6x.h>					//generic include
#include <csl.h>					//generic csl include
#include <csl_gpio.h>
#include <csl_mcbsp.h>				//for codec support
#include <csl_irq.h>				//interrupt support
#include <math.h>					//duh

#include "dsk6713.h"
#include "dsk6713_aic23.h"
#include "dsk6713_led.h"
#include "gpio.h"

// ------------------------------------------
// start of variables
// ------------------------------------------


DSK6713_AIC23_CodecHandle hCodec;							// Codec handle
DSK6713_AIC23_Config config = DSK6713_AIC23_DEFAULTCONFIG;  // Codec configuration with default settings

GPIO_Handle    hGpio; /* GPIO handle */

GPIO_Handle gpio_handle;
GPIO_Config gpio_config = {
    0x00000000, // gpgc = global control
    0x0000FFFF, // gpen = enable 0-15
    0x00000000, // gdir = all inputs
    0x00000000, // gpval = n/a
    0x00000000, // gphm all interrupts disabled for io pins
    0x00000000, // gplm all interrupts to cpu or edma disabled
    0x00000000  // gppol -- default state */
};

// ------------------------------------------
// end of variables
// ------------------------------------------





int led_prev=0;//not sure how to check led state so just keep local copy

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

	GPIO_Config MyConfig = {

	0x00000000, /* gpgc */

	0x0000FFFF, /* gpen --*/

	0x00000000, /* gdir -*/

	0x00000000, /* gpval */

	0x00000000, /* gphm all interrupts disabled for io pins */

	0x00000000, /* gplm all interrupts to cpu or edma disabled  */

	0x00000000  /* gppol -- default state */

	};

	hGpio  = GPIO_open( GPIO_DEV0, GPIO_OPEN_RESET );

	GPIO_config(hGpio  , &MyConfig );

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

	/* Sets Pin0, Pin1, and Pin2 as an output pins. */
	//do pin configuration later, in the SPI init function
	int Current_dir;

	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN0, GPIO_OUTPUT);	// CS
	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN1, GPIO_OUTPUT);
	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN2, GPIO_OUTPUT);
	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN3, GPIO_OUTPUT);	// MOSI
	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN13, GPIO_OUTPUT);	// SCK

	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN8, GPIO_INPUT);	// MISO
	Current_dir = GPIO_pinDirection(hGpio,GPIO_PIN9, GPIO_INPUT);	// GDO0


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

void cc1101_Select()
{
	// for instance you call
	// GPIO_pinWrite( hGpio, GPIO_PIN0, 1 );
	GPIO_pinWrite( hGpio, GPIO_PIN0, 0 );
}

void cc1101_Deselect()
{
	// for instance you call
	// GPIO_pinWrite( hGpio, GPIO_PIN0, 1 );
	GPIO_pinWrite( hGpio, GPIO_PIN0, 1 );
}

void wait_Miso()
{
	// wait while cc1101 pulls SO low, indicating to be ready
	while(GPIO_pinRead(hGpio, GPIO_PIN8))	;
}

void wait_GDO0_high()
{
	// Wait until GDO0 line goes high
	while(!GPIO_pinRead(hGpio, GPIO_PIN9))	;
}

void wait_GDO0_low()
{
	// Wait until GDO0 line goes low
	while(GPIO_pinRead(hGpio, GPIO_PIN9))	;
}
