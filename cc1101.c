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

#include "cc1101.h"
#include "gpio.h"
#include "spi.h"
#include "bitbang_spi.h"
//#include <csl_mcbsp.h>
//#include "nvolat.h"

//MCBSP_Handle extern hMcbsp;

/**
 * Macros
 */
//// Select (SPI) CC1101
//#define cc1101_Select()  bitClear(PORT_SPI_SS, BIT_SPI_SS)
//// Deselect (SPI) CC1101
//#define cc1101_Deselect()  bitSet(PORT_SPI_SS, BIT_SPI_SS)
//// Wait until SPI MISO line goes low
//#define wait_Miso()  while(bitRead(PORT_SPI_MISO, BIT_SPI_MISO))
// Get GDO0 pin state
#define getGDO0state()  bitRead(PORT_GDO0, BIT_GDO0)
//// Wait until GDO0 line goes high
//#define wait_GDO0_high()  while(!getGDO0state())
//// Wait until GDO0 line goes low
//#define wait_GDO0_low()  while(getGDO0state())

#define false 0
#define true 1
typedef char bool;

 /**
  * PATABLE
  */
const byte paTable[8] = {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60};

struct CC1101
{
	    /*
	     * RF state
	     */
	    byte rfState;

	    /**
	     * Tx Power byte (single PATABLE config)
	     */
	    byte paTableByte;

	    /**
	     * Carrier frequency
	     */
	    byte carrierFreq;

	    /**
	     * Frequency channel
	     */
	    byte channel;

	    /**
	     * Synchronization word
	     */
	    byte syncWord[2];

	    /**
	     * Device address
	     */
	    byte devAddress;
}CC1101;

//------------------------------------------------------------------------------------
/*		FUNCTION PROTOPYTES	(that are neccessary not to case any compiler errors)	*/
//------------------------------------------------------------------------------------
void CC1101_setSyncWord(byte *sync, bool save);
void CC1101_setDefaultRegs();
void CC1101_setDevAddress(byte addr, bool save);
void CC1101_setChannel(byte chnl, bool save);
void CC1101_setCarrierFreq(byte freq);

///**
// * CC1101
// *
// * Class constructor
// */
//CC1101_CC1101(void)
//{
//  paTableByte = PA_LowPower;            // Priority = Low power
//}

///**
// * wakeUp
// *
// * Wake up CC1101 from Power Down state
// */
//void CC1101::wakeUp(void)
//{
//  cc1101_Select();                      // Select CC1101
//  wait_Miso();                          // Wait until MISO goes low
//  cc1101_Deselect();                    // Deselect CC1101
//}

/**
 * CC1101_writeReg
 *
 * Write single register into the CC1101 IC via SPI
 *
 * 'regAddr'    Register address
 * 'value'      Value to be writen
 */
void CC1101_writeReg(byte regAddr, byte value)
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low

//  // waits are needed since CSn is triggered by GPIO which is not in accordance with SPI peripheral which takes few cycles to shift out after you order it
//  spi_send(regAddr);                    // Send register address
//  DSK6713_waitusec(5);
//  spi_send(value);                      // Send value
//  DSK6713_waitusec(20);

  spiWriteReg(regAddr, value);

  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * writeBurstReg
 *
 * Write multiple registers into the CC1101 IC via SPI
 *
 * 'regAddr'    Register address
 * 'buffer'     Data to be writen
 * 'len'        Data length
 */
void CC1101_writeBurstReg(byte regAddr, byte* buffer, byte len)
{
  byte addr, i;

  addr = regAddr | WRITE_BURST;         // Enable burst transfer
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  //spi_send(addr);                       // Send register address
  spiWriteAddr(addr);
  //DSK6713_waitusec(5);

  for(i=0 ; i<len ; i++)
  {
    //spi_send(buffer[i]);                // Send value
	  spiWriteData(buffer[i]);
    //DSK6713_waitusec(10);
  }

  //DSK6713_waitusec(20);

  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * cmdStrobe
 *
 * Send command strobe to the CC1101 IC via SPI
 *
 * 'cmd'        Command strobe
 */
void CC1101_cmdStrobe(byte cmd)
{
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low

//  spi_send(cmd);                        // Send strobe command
//
//  // send again, for some reason first send does not work
//  DSK6713_waitusec(10);
//  spi_send(cmd);                        // Send strobe command
//
//  DSK6713_waitusec(20);

  spiWriteReg(cmd,0x00);	// basically we only need to send 1 byte

  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * readReg
 *
 * Read CC1101 register via SPI
 *
 * 'regAddr'    Register address
 * 'regType'    Type of register: CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
 *
 * Return:
 *      Data byte returned by the CC1101 IC
 */
byte CC1101_readReg(byte regAddr, byte regType)
{
  byte addr, val;

  addr = regAddr | regType;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low

//  // have to do it twice for some reason, first read fails
//  spi_send(addr);                       // Send register address
//  //val = spi_send(addr);
//  DSK6713_waitusec(5);
//  //DSK6713_waitusec(25);
//  val = spi_send(0x00);                 // Read result
//  DSK6713_waitusec(20);
//
//  spi_send(addr);                       // Send register address
//  DSK6713_waitusec(5);
//  val = spi_send(0x00);                 // Read result
//  DSK6713_waitusec(20);

  val = spiReadReg(addr);

  cc1101_Deselect();                    // Deselect CC1101

  return val;
}

/**
 * readBurstReg
 *
 * Read burst data from CC1101 via SPI
 *
 * 'buffer'     Buffer where to copy the result to
 * 'regAddr'    Register address
 * 'len'        Data length
 */
void CC1101_readBurstReg(byte * buffer, byte regAddr, byte len)
{
  byte addr, i;

  addr = regAddr | READ_BURST;
  cc1101_Select();                      // Select CC1101
  wait_Miso();                          // Wait until MISO goes low
  //spi_send(addr);                       // Send register address
  spiWriteAddr(addr);
  //DSK6713_waitusec(5);

  for(i=0 ; i<len ; i++)
  {
    //buffer[i] = spi_send(0x00);         // Read result byte by byte
	  buffer[i] = spiReadData();         // Read result byte by byte
  	//DSK6713_waitusec(15);
  }

  //DSK6713_waitusec(20);

  cc1101_Deselect();                    // Deselect CC1101
}

/**
 * reset
 *
 * Reset CC1101
 */
void CC1101_reset(void)
{
  cc1101_Deselect();                    // Deselect CC1101
  DSK6713_waitusec(5);
  cc1101_Select();                      // Select CC1101
  DSK6713_waitusec(10);
  cc1101_Deselect();                    // Deselect CC1101
  DSK6713_waitusec(41);
  cc1101_Select();                      // Select CC1101

  wait_Miso();                          // Wait until MISO goes low
  //spi_send(CC1101_SRES);                // Send reset command strobe
  spiWriteReg(CC1101_SRES,0x00);
  wait_Miso();                          // Wait until MISO goes low

  cc1101_Deselect();                    // Deselect CC1101

  CC1101_setDefaultRegs();                     // Reconfigure CC1101
  //setRegsFromEeprom();                  // Take user settings from EEPROM
}

/**
 * setDefaultRegs
 *
 * Configure CC1101 registers
 */
void CC1101_setDefaultRegs()
{
  byte defSyncWrd[] = {CC1101_DEFVAL_SYNC1, CC1101_DEFVAL_SYNC0};

  CC1101_writeReg(CC1101_IOCFG2,  CC1101_DEFVAL_IOCFG2);
  CC1101_writeReg(CC1101_IOCFG1,  CC1101_DEFVAL_IOCFG1);
  CC1101_writeReg(CC1101_IOCFG0,  CC1101_DEFVAL_IOCFG0);
  CC1101_writeReg(CC1101_FIFOTHR,  CC1101_DEFVAL_FIFOTHR);
  CC1101_writeReg(CC1101_PKTLEN,  CC1101_DEFVAL_PKTLEN);
  CC1101_writeReg(CC1101_PKTCTRL1,  CC1101_DEFVAL_PKTCTRL1);
  CC1101_writeReg(CC1101_PKTCTRL0,  CC1101_DEFVAL_PKTCTRL0);

  // Set default synchronization word
  CC1101_setSyncWord(defSyncWrd, false);

  // Set default device address
  CC1101_setDevAddress(CC1101_DEFVAL_ADDR, false);
  // Set default frequency channel
  CC1101_setChannel(CC1101_DEFVAL_CHANNR, false);

  CC1101_writeReg(CC1101_FSCTRL1,  CC1101_DEFVAL_FSCTRL1);
  CC1101_writeReg(CC1101_FSCTRL0,  CC1101_DEFVAL_FSCTRL0);

  // Set default carrier frequency = 868 MHz
  CC1101_setCarrierFreq(CFREQ_315);

  CC1101_writeReg(CC1101_MDMCFG4,  CC1101_DEFVAL_MDMCFG4);
  CC1101_writeReg(CC1101_MDMCFG3,  CC1101_DEFVAL_MDMCFG3);
  CC1101_writeReg(CC1101_MDMCFG2,  CC1101_DEFVAL_MDMCFG2);
  CC1101_writeReg(CC1101_MDMCFG1,  CC1101_DEFVAL_MDMCFG1);
  CC1101_writeReg(CC1101_MDMCFG0,  CC1101_DEFVAL_MDMCFG0);
  CC1101_writeReg(CC1101_DEVIATN,  CC1101_DEFVAL_DEVIATN);
  CC1101_writeReg(CC1101_MCSM2,  CC1101_DEFVAL_MCSM2);
  CC1101_writeReg(CC1101_MCSM1,  CC1101_DEFVAL_MCSM1);
  CC1101_writeReg(CC1101_MCSM0,  CC1101_DEFVAL_MCSM0);
  CC1101_writeReg(CC1101_FOCCFG,  CC1101_DEFVAL_FOCCFG);
  CC1101_writeReg(CC1101_BSCFG,  CC1101_DEFVAL_BSCFG);
  CC1101_writeReg(CC1101_AGCCTRL2,  CC1101_DEFVAL_AGCCTRL2);
  CC1101_writeReg(CC1101_AGCCTRL1,  CC1101_DEFVAL_AGCCTRL1);
  CC1101_writeReg(CC1101_AGCCTRL0,  CC1101_DEFVAL_AGCCTRL0);
  CC1101_writeReg(CC1101_WOREVT1,  CC1101_DEFVAL_WOREVT1);
  CC1101_writeReg(CC1101_WOREVT0,  CC1101_DEFVAL_WOREVT0);
  CC1101_writeReg(CC1101_WORCTRL,  CC1101_DEFVAL_WORCTRL);
  CC1101_writeReg(CC1101_FREND1,  CC1101_DEFVAL_FREND1);
  CC1101_writeReg(CC1101_FREND0,  CC1101_DEFVAL_FREND0);
  CC1101_writeReg(CC1101_FSCAL3,  CC1101_DEFVAL_FSCAL3);
  CC1101_writeReg(CC1101_FSCAL2,  CC1101_DEFVAL_FSCAL2);
  CC1101_writeReg(CC1101_FSCAL1,  CC1101_DEFVAL_FSCAL1);
  CC1101_writeReg(CC1101_FSCAL0,  CC1101_DEFVAL_FSCAL0);
  CC1101_writeReg(CC1101_RCCTRL1,  CC1101_DEFVAL_RCCTRL1);
  CC1101_writeReg(CC1101_RCCTRL0,  CC1101_DEFVAL_RCCTRL0);
  CC1101_writeReg(CC1101_FSTEST,  CC1101_DEFVAL_FSTEST);
  CC1101_writeReg(CC1101_PTEST,  CC1101_DEFVAL_PTEST);
  CC1101_writeReg(CC1101_AGCTEST,  CC1101_DEFVAL_AGCTEST);
  CC1101_writeReg(CC1101_TEST2,  CC1101_DEFVAL_TEST2);
  CC1101_writeReg(CC1101_TEST1,  CC1101_DEFVAL_TEST1);
  CC1101_writeReg(CC1101_TEST0,  CC1101_DEFVAL_TEST0);
}

/**
 * init
 *
 * Initialize CC1101
 */
void CC1101_init(void)
{
  spi_init();                           // Initialize SPI interface
  //pinMode(GDO0, INPUT);                 // Config GDO0 as input

  CC1101_reset();                              // Reset CC1101

  // Configure PATABLE
  CC1101_writeBurstReg(CC1101_PATABLE, (byte*)paTable, 8);
  //CC1101_writeReg(CC1101_PATABLE, CC1101.paTableByte);

  char read = CC1101_readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER);
  read = CC1101_readReg(CC1101_VERSION, CC1101_STATUS_REGISTER);
  read = CC1101_readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER);

  //byte syncWord = 199;
  //CC1101_setSyncWord(&syncWord, false);
  CC1101_setCarrierFreq(CFREQ_315);
  CC1101_disableAddressCheck();

//  //  Serial.print("CC1101_PARTNUM "); //cc1101=0
//    char temp[5];
//  	sprintf(temp, "%s",CC1101_readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER));
//    printf("CC1101 part number: ",temp);
//  //  Serial.print("CC1101_VERSION "); //cc1101=4
//    printf(CC1101_readReg(CC1101_VERSION, CC1101_STATUS_REGISTER));
//  //  Serial.print("CC1101_MARCSTATE ");
//    printf(CC1101_readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & 0x1f);
}

/**
 * setSyncWord
 *
 * Set synchronization word
 *
 * 'sync'       Synchronization word
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101_setSyncWord(byte *sync, bool save)
{
  //if ((CC1101.syncWord[0] != sync[0]) || (CC1101.syncWord[1] != sync[1]))
  {
    CC1101_writeReg(CC1101_SYNC1, sync[0]);
    CC1101_writeReg(CC1101_SYNC0, sync[1]);
    // copy sync word into the struct
    //memcpy(CC1101.syncWord, sync, sizeof(CC1101.syncWord));

//    // Save in EEPROM
//    if (save)
//    {
//      EEPROM.write(EEPROM_SYNC_WORD, sync[0]);
//      EEPROM.write(EEPROM_SYNC_WORD + 1, sync[1]);
//    }
  }
}

/**
 * setDevAddress
 *
 * Set device address
 *
 * 'addr'       Device address
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101_setDevAddress(byte addr, bool save)
{
  //if (CC1101.devAddress != addr)
  {
    CC1101_writeReg(CC1101_ADDR, addr);
    CC1101.devAddress = addr;
    // Save in EEPROM
    //if (save)
    //  EEPROM.write(EEPROM_DEVICE_ADDR, addr);
  }
}

/**
 * setChannel
 *
 * Set frequency channel
 *
 * 'chnl'       Frequency channel
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101_setChannel(byte chnl, bool save)
{
  //if (CC1101.channel != chnl)
  {
    CC1101_writeReg(CC1101_CHANNR,  chnl);
    CC1101.channel = chnl;
    // Save in EEPROM
    //if (save)
    //  EEPROM.write(EEPROM_FREQ_CHANNEL, chnl);
  }
}

/**
 * setCarrierFreq
 *
 * Set carrier frequency
 *
 * 'freq'       New carrier frequency
 */
void CC1101_setCarrierFreq(byte freq)
{
  switch(freq)
  {
    case CFREQ_915:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_915);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_915);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_915);
      break;
    case CFREQ_433:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_433);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_433);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_433);
      break;
    case CFREQ_315:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_315);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_315);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_315);
      break;
    default:
      CC1101_writeReg(CC1101_FREQ2,  CC1101_DEFVAL_FREQ2_868);
      CC1101_writeReg(CC1101_FREQ1,  CC1101_DEFVAL_FREQ1_868);
      CC1101_writeReg(CC1101_FREQ0,  CC1101_DEFVAL_FREQ0_868);
      break;
  }

  CC1101_readReg(CC1101_FREQ2,  CC1101_CONFIG_REGISTER);
  CC1101_readReg(CC1101_FREQ1,  CC1101_CONFIG_REGISTER);
  CC1101_readReg(CC1101_FREQ0,  CC1101_CONFIG_REGISTER);

  CC1101.carrierFreq = freq;
}

///**
// * setRegsFromEeprom
// *
// * Set registers from EEPROM
// */
//void CC1101::setRegsFromEeprom(void)
//{
//  byte bVal;
//  byte arrV[2];
//
//  // Read RF channel from EEPROM
//  bVal = EEPROM.read(EEPROM_FREQ_CHANNEL);
//  // Set RF channel
//  if (bVal < NUMBER_OF_FCHANNELS )
//    setChannel(bVal, false);
//  // Read Sync word from EEPROM
//  arrV[0] = EEPROM.read(EEPROM_SYNC_WORD);
//  arrV[1] = EEPROM.read(EEPROM_SYNC_WORD + 1);
//  // Set Sync word. 0x00 and 0xFF values are not allowed
//  if (((arrV[0] != 0x00) && (arrV[0] != 0xFF)) || ((arrV[1] != 0x00) && (arrV[1] != 0xFF)))
//    setSyncWord(arrV, false);
//  // Read device address from EEPROM
//  bVal = EEPROM.read(EEPROM_DEVICE_ADDR);
//  // Set device address
//  if (bVal > 0)
//    setDevAddress(bVal, false);
//}

///**
// * setPowerDownState
// *
// * Put CC1101 into power-down state
// */
//void CC1101_setPowerDownState()
//{
//  // Comming from RX state, we need to enter the IDLE state first
//  cmdStrobe(CC1101_SIDLE);
//  // Enter Power-down state
//  cmdStrobe(CC1101_SPWD);
//}

/**
 * sendData
 *
 * Send data packet via RF
 *
 * 'packet'     Packet to be transmitted. First byte is the destination address
 *
 *  Return:
 *    True if the transmission succeeds
 *    False otherwise
 */
boolean CC1101_sendData(CCPACKET packet)
{
  byte marcState;

  // Declare to be in Tx state. This will avoid receiving packets whilst
  // transmitting
  CC1101.rfState = RFSTATE_TX;

  	// Enter RX state
	setRxState();

  // Check that the RX state has been entered
  while (((marcState = readStatusReg(CC1101_MARCSTATE)) & 0x1F) != 0x0D)
  {
	  DSK6713_waitusec(1000);

    if (marcState == 0x11)        // RX_OVERFLOW
      flushRxFifo();              // flush receive queue

    if (marcState == 0x16)        // TX_UNDERFLOW
      flushTxFifo();              // flush receive queue
  }
  DSK6713_waitusec(500);

  // Set data length at the first position of the TX FIFO
  CC1101_writeReg(CC1101_TXFIFO,  packet.length);
  // Write data into the TX FIFO
  CC1101_writeBurstReg(CC1101_TXFIFO, packet.data, packet.length);


  // CCA enabled: will enter TX state only if the channel is clear
  CC1101_cmdStrobe(CC1101_STX);
  setTxState();


//  while(!((marcState == 0x13) || (marcState == 0x14) || (marcState == 0x15)))
//  {
//
//// 	    if (marcState == 0x11)        // RX_OVERFLOW
////	      flushRxFifo();              // flush receive queue
////
//	    if (marcState == 0x16)        // TX_UNDERFLOW
//	      flushTxFifo();              // flush receive queue
//
//	  // CCA enabled: will enter TX state only if the channel is clear
//	  setTxState();
//
	  // Check that TX state is being entered (state = RXTX_SETTLING)
	  marcState = readStatusReg(CC1101_MARCSTATE) & 0x1F;
//  }
	  //DSK6713_waitusec(1000);

  if((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15))
  {
    setIdleState();       // Enter IDLE state
    flushTxFifo();        // Flush Tx FIFO
    setRxState();         // Back to RX state

    // Declare to be in Rx state
    CC1101.rfState = RFSTATE_RX;
    return false;
  }

  // Wait for the sync word to be transmitted
  wait_GDO0_high();

  // Wait until the end of the packet transmission
  wait_GDO0_low();

  // Check that the TX FIFO is empty
  if((readStatusReg(CC1101_TXBYTES) & 0x7F) == 0)
    return true;

//  setIdleState();       // Enter IDLE state
//  flushTxFifo();        // Flush Tx FIFO
//
//  // Enter back into RX state
//  setRxState();
//
//  // Declare to be in Rx state
//  CC1101.rfState = RFSTATE_RX;

  return false;
}

/**
 * receiveData
 *
 * Read data packet from RX FIFO
 *
 * 'packet'     Container for the packet received
 *
 * Return:
 *      Amount of bytes received
 */
byte CC1101_receiveData(CCPACKET * packet)
{
//  byte val;
//  byte rxBytes = readStatusReg(CC1101_RXBYTES);
//
//  // Any byte waiting to be read and no overflow?
//  if (rxBytes & 0x7F && !(rxBytes & 0x80))
//  {
//    // Read data length
//    packet->length = readConfigReg(CC1101_RXFIFO);
//    // If packet is too long
//    if (packet->length > CC1101_DATA_LEN)
//      packet->length = 0;   // Discard packet
//    else
//    {
//      // Read data packet
//      CC1101_readBurstReg(packet->data, CC1101_RXFIFO, packet->length);
//      // Read RSSI
//      packet->rssi = readConfigReg(CC1101_RXFIFO);
//      // Read LQI and CRC_OK
//      val = readConfigReg(CC1101_RXFIFO);
//      packet->lqi = val & 0x7F;
//      packet->crc_ok = (val & 0x80)>>7;// 8th or 7th bit //bitRead(val, 7);
//    }
//  }
//  else
//    packet->length = 0;
//
//  setIdleState();       // Enter IDLE state
//  flushRxFifo();        // Flush Rx FIFO
//  //CC1101_cmdStrobe(CC1101_SCAL);
//  CC1101_cmdStrobe(CC1101_SRX);
//
//  // Back to RX state
//  setRxState();
//
//  return packet->length;

	  byte marcState;
	  byte val;

	  // Rx FIFO overflow?
	  if (((marcState=readStatusReg(CC1101_MARCSTATE)) & 0x1F) == 0x11)
	  {
	    setIdleState();       // Enter IDLE state
	    flushRxFifo();        // Flush Rx FIFO
	    //CC1101_cmdStrobe(CC1101_SFSTXON);
	    packet->length = 0;
	  }
	  // Any byte waiting to be read?
	  else if ((val=readStatusReg(CC1101_RXBYTES)) & 0x7F)
	  {
	    // Read data length
	    packet->length = readConfigReg(CC1101_RXFIFO);
	    // If packet is too long
	    if (packet->length > CC1101_DATA_LEN)
	      packet->length = 0;   // Discard packet
	    else
	    {
	      // Read data packet
	      CC1101_readBurstReg(packet->data, CC1101_RXFIFO, packet->length);
	      // Read RSSI
	      packet->rssi = readConfigReg(CC1101_RXFIFO);
	      // Read LQI and CRC_OK
	      val = readConfigReg(CC1101_RXFIFO);
	      packet->lqi = val & 0x7F;
	      packet->crc_ok = (val & 0x80)>>7;
	    }
	  }
	  else
	    packet->length = 0;

	  CC1101_cmdStrobe(CC1101_SRX);
	  //while((marcState=readStatusReg(CC1101_MARCSTATE))==0x80)	; // loop while calibrating
	  // Back to RX state
	  setRxState();

	  return packet->length;
}
