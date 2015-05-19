/*
 *---------serial_loop.c---------
 *
 * CC1101 driver for C6713 DSK
 *
 * CoAuthor: Stefan Gvozdenovic
 * Creation date: 05/19/2015
 *
 * Description: The Code is based on the panstamp CC1101 library for Arduino. This code
 * 				demonstrates transmission and reception (without interrupts) of
 * 				arbitrary data length/type from the CC1101 radio chip attached to the
 * 				GPIO pins of the 6713 through J1 extension header. The SPI between radio
 * 				and 6713 is implemented by bit-banging the GPIO. Two C6713 DSKs and two CC1101
 * 				radios are needed for this demo.
 *
 */
#include <stdio.h>
#include <csl.h>
#include <csl_gpio.h>
#include <dsk6713.h>
#include "cc1101.h"
#include "gpio.h"

/*---- data declarations ---------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
void main() {

  int i = 0;

  /* Initialize the chip support library, must when using CSL */
  CSL_init();

  //Setup GPIO
  gpioInit();

  // initialize the RF Chip
  CC1101_init();

  char read;
  byte bsend = 0x00;//CC1101_PARTNUM;;

  CCPACKET data;
//  data.length = 5;
//  data.data[0]=5;
//  data.data[1]=0xaa;
//  data.data[2]=0;
//  data.data[3]=1;
//  data.data[4]=0;

  while(1)
  {
//	  while()
//	 	  CC1101_cmdStrobe(0x36);

	  //bsend++;
	  //read = CC1101_readReg(bsend, CC1101_CONFIG_REGISTER);


	  //data.data[0]++;

	  if(CC1101_receiveData(&data) > 0){
		  if(!data.crc_ok)
			  printf("crc not ok\n");


		  if(data.length > 0){
			  printf("packet: len %d data: ",(int)data.length);
			  for(i=0;i<data.length;++i)
				  printf("%d ", data.data[i]);
			  printf("\n");
		  }



	  }else
		  printf("no data\n");

//	  char sent = CC1101_sendData(data);
//
//	  if(!sent)
//		  printf("Failed!\n");
//	  else
//		  printf("Sent :)\n");
//
	  //DSK6713_waitusec(10000);
  }


}

interrupt void serialPortRcvISR()
{
//union {Uint32 combo; short channel[2];} temp;

//temp.combo = MCBSP_read(DSK6713_AIC23_DATAHANDLE);
// Note that right channel is in temp.channel[0]
// Note that left channel is in temp.channel[1]

//MCBSP_write(DSK6713_AIC23_DATAHANDLE, temp.combo);
}
