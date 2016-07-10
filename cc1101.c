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
 *
 * Converted to C code for C6713: Stefan Gvozdenovic
 * Creation date: 05/19/2015
 */

#include "cc1101.h"

#include <stdio.h>

#include "../generic_noarch/debug/debug.h"

//extern GPIO_Handle    hGpio; /* GPIO handle */
static struct cc1101_hw *hw;
static volatile u8_t interrupt_state = 0;

/**
 * Macros
 */
// Select (SPI) CC1101
#define cc1101_Select()
// Deselect (SPI) CC1101
#define cc1101_Deselect()
// Wait until SPI MISO line goes low
#define wait_Miso()
// Get GDO0 pin state
//#define getGDO0state()  bitRead(PORT_GDO0, BIT_GDO0)
// Wait until GDO0 line goes high
//#define wait_GDO0_high() while(!GPIO_pinRead( hGpio, GPIO_PIN9 ))
// Wait until GDO0 line goes low
//#define wait_GDO0_low()  while(GPIO_pinRead( hGpio, GPIO_PIN9 ))

#define false 0
#define true 1
typedef char bool;

/**
 * PATABLE
 */
//const byte paTable[8] = { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const byte paTable[8] = { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 };

struct CC1101 {
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
} CC1101;

//------------------------------------------------------------------------------------
/*		FUNCTION PROTOPYTES	(that are neccessary not to case any compiler errors)	*/
//------------------------------------------------------------------------------------
void CC1101_setSyncWord(byte *sync, bool save);
void CC1101_setDefaultRegs(void);
void CC1101_setDevAddress(byte addr, bool save);
void CC1101_setChannel(byte chnl, bool save);
void CC1101_setCarrierFreq(byte freq);

/**
 * wakeUp
 *
 * Wake up CC1101 from Power Down state
 */
void CC1101_wakeUp(void) {
	cc1101_Select();                      // Select CC1101
	wait_Miso();                          // Wait until MISO goes low
	cc1101_Deselect();                    // Deselect CC1101
}

/**
 * CC1101_writeReg
 *
 * Write single register into the CC1101 IC via SPI
 *
 * 'regAddr'    Register address
 * 'value'      Value to be writen
 */
void CC1101_writeReg(byte regAddr, byte value) {
	hw->spiWriteReg(regAddr, value);
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
void CC1101_writeBurstReg(byte regAddr, byte* buffer, byte len) {
	hw->spiBurstWrite(regAddr, (unsigned char *) buffer, len);
}
void CC1101_writeBurstTXFIFO(byte regAddr, byte* buffer, byte len) {
	hw->spiBurstWrite(regAddr, (unsigned char *) buffer, len);
}

/**
 * cmdStrobe
 *
 * Send command strobe to the CC1101 IC via SPI
 *
 * 'cmd'        Command strobe
 */
void CC1101_cmdStrobe(byte cmd) {
	hw->spiBurstWrite(cmd, NULL, 0);	// basically we only need to send 1 byte
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
byte CC1101_readReg(byte regAddr, byte regType) {
	byte addr, val;

	addr = regAddr | regType;
	val = hw->spiReadReg(addr);

	return val;
}
byte CC1101_readRegData(byte regAddr, byte regType) {
	byte addr, val;

	addr = regAddr | regType;
	val = hw->spiReadRegData(addr);

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
void CC1101_readBurstReg(byte * buffer, byte regAddr, byte len) {
	byte addr;
	addr = regAddr | READ_BURST;
	hw->spiBurstRead(addr, (unsigned char *) buffer, len);
}

/**
 * reset
 *
 * Reset CC1101
 */
void CC1101_reset(void) {
	hw->spiWriteReg(CC1101_SRES, 0x00);
}

/**
 * setDefaultRegs
 *
 * Configure CC1101 registers
 */
void CC1101_setDefaultRegs(void) {
	byte defSyncWrd[] = { CC1101_DEFVAL_SYNC1, CC1101_DEFVAL_SYNC0 };

	CC1101_writeReg(CC1101_IOCFG2, CC1101_DEFVAL_IOCFG2);
	CC1101_writeReg(CC1101_IOCFG1, CC1101_DEFVAL_IOCFG1);
	CC1101_writeReg(CC1101_IOCFG0, 0x06);
	CC1101_writeReg(CC1101_FIFOTHR, CC1101_DEFVAL_FIFOTHR);
	CC1101_writeReg(CC1101_PKTLEN, 0x3D);
	CC1101_writeReg(CC1101_PKTCTRL1, CC1101_DEFVAL_PKTCTRL1);
	CC1101_writeReg(CC1101_PKTCTRL0, CC1101_DEFVAL_PKTCTRL0);

	// Set default synchronization word
	CC1101_setSyncWord(defSyncWrd, false);

	// Set default device address
	CC1101_setDevAddress(CC1101_DEFVAL_ADDR, false);
	// Set default frequency channel
	CC1101_setChannel(CC1101_DEFVAL_CHANNR, false);

	CC1101_writeReg(CC1101_FSCTRL1, CC1101_DEFVAL_FSCTRL1);
	CC1101_writeReg(CC1101_FSCTRL0, CC1101_DEFVAL_FSCTRL0);

	// Set default carrier frequency = 868 MHz
	CC1101_setCarrierFreq(CFREQ_433);

	CC1101_writeReg(CC1101_MDMCFG4, CC1101_DEFVAL_MDMCFG4);
	CC1101_writeReg(CC1101_MDMCFG3, CC1101_DEFVAL_MDMCFG3);
	CC1101_writeReg(CC1101_MDMCFG2, CC1101_DEFVAL_MDMCFG2);
	CC1101_writeReg(CC1101_MDMCFG1, CC1101_DEFVAL_MDMCFG1);
	CC1101_writeReg(CC1101_MDMCFG0, CC1101_DEFVAL_MDMCFG0);
	CC1101_writeReg(CC1101_DEVIATN, CC1101_DEFVAL_DEVIATN);
	CC1101_writeReg(CC1101_MCSM2, CC1101_DEFVAL_MCSM2);
//	CC1101_writeReg(CC1101_MCSM1, 0x0F);
	CC1101_writeReg(CC1101_MCSM1, CC1101_DEFVAL_MCSM1);
	CC1101_writeReg(CC1101_MCSM0, CC1101_DEFVAL_MCSM0);
//	CC1101_writeReg(CC1101_MCSM0, 0x24);
	CC1101_writeReg(CC1101_FOCCFG, CC1101_DEFVAL_FOCCFG);
	CC1101_writeReg(CC1101_BSCFG, CC1101_DEFVAL_BSCFG);
	CC1101_writeReg(CC1101_AGCCTRL2, CC1101_DEFVAL_AGCCTRL2);
	CC1101_writeReg(CC1101_AGCCTRL1, CC1101_DEFVAL_AGCCTRL1);
	CC1101_writeReg(CC1101_AGCCTRL0, CC1101_DEFVAL_AGCCTRL0);
	CC1101_writeReg(CC1101_WOREVT1, CC1101_DEFVAL_WOREVT1);
	CC1101_writeReg(CC1101_WOREVT0, CC1101_DEFVAL_WOREVT0);
	CC1101_writeReg(CC1101_WORCTRL, CC1101_DEFVAL_WORCTRL);
	CC1101_writeReg(CC1101_FREND1, CC1101_DEFVAL_FREND1);
	CC1101_writeReg(CC1101_FREND0, CC1101_DEFVAL_FREND0);
	CC1101_writeReg(CC1101_FSCAL3, CC1101_DEFVAL_FSCAL3);
	CC1101_writeReg(CC1101_FSCAL2, CC1101_DEFVAL_FSCAL2);
	CC1101_writeReg(CC1101_FSCAL1, CC1101_DEFVAL_FSCAL1);
	CC1101_writeReg(CC1101_FSCAL0, CC1101_DEFVAL_FSCAL0);
	CC1101_writeReg(CC1101_RCCTRL1, CC1101_DEFVAL_RCCTRL1);
	CC1101_writeReg(CC1101_RCCTRL0, CC1101_DEFVAL_RCCTRL0);
	CC1101_writeReg(CC1101_FSTEST, CC1101_DEFVAL_FSTEST);
	CC1101_writeReg(CC1101_PTEST, CC1101_DEFVAL_PTEST);
	CC1101_writeReg(CC1101_AGCTEST, CC1101_DEFVAL_AGCTEST);
	CC1101_writeReg(CC1101_TEST2, CC1101_DEFVAL_TEST2);
	CC1101_writeReg(CC1101_TEST1, CC1101_DEFVAL_TEST1);
	CC1101_writeReg(CC1101_TEST0, CC1101_DEFVAL_TEST0);
}

/**
 * init
 *
 * Initialize CC1101
 */
void CC1101_init(struct cc1101_hw *cc1101_hw) {
	hw = cc1101_hw;
	CC1101_reset();
	CC1101_writeReg(CC1101_PKTLEN, 0x3D);
// Reset CC1101
	// Configure PATABLE
	hw->spiBurstWrite(CC1101_PATABLE, (unsigned char *) paTable, 8);
	//CC1101_writeReg(CC1101_PATABLE, CC1101.paTableByte);

	CC1101_readReg(CC1101_PARTNUM, CC1101_STATUS_REGISTER);
	CC1101_readReg(CC1101_VERSION, CC1101_STATUS_REGISTER);
	CC1101_readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER);
	CC1101_setDefaultRegs();                     // Reconfigure CC1101

//	byte syncWord = 199;
//	CC1101_setSyncWord(&syncWord, false);
	CC1101_setCarrierFreq(CFREQ_433);
	CC1101_disableAddressCheck();
	disableCCA();
	setIdleState();
	flushRxFifo();
	flushTxFifo();
	setRxState();

}

/**
 * setSyncWord
 *
 * Set synchronization word
 *
 * 'sync'       Synchronization word
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101_setSyncWord(byte *sync, bool save) {
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
 * else {
 outgoingMessage = null;
 nextState = RadioPacketManagerState.RX;
 }
 * 'addr'       Device address
 * 'save' If TRUE, save parameter in EEPROM
 */
void CC1101_setDevAddress(byte addr, bool save) {
	//if (CC1101.devAddress != addr)
	{
		CC1101_writeReg(CC1101_ADDR, addr);
		CC1101.devAddress = addr;
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
void CC1101_setChannel(byte chnl, bool save) {
	//if (CC1101.channel != chnl)
	{
		CC1101_writeReg(CC1101_CHANNR, chnl);
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
void CC1101_setCarrierFreq(byte freq) {
	switch (freq) {
	case CFREQ_915:
		CC1101_writeReg(CC1101_FREQ2, CC1101_DEFVAL_FREQ2_915);
		CC1101_writeReg(CC1101_FREQ1, CC1101_DEFVAL_FREQ1_915);
		CC1101_writeReg(CC1101_FREQ0, CC1101_DEFVAL_FREQ0_915);
		break;
	case CFREQ_433:
		CC1101_writeReg(CC1101_FREQ2, CC1101_DEFVAL_FREQ2_433);
		CC1101_writeReg(CC1101_FREQ1, CC1101_DEFVAL_FREQ1_433);
		CC1101_writeReg(CC1101_FREQ0, CC1101_DEFVAL_FREQ0_433);
		break;
	case CFREQ_315:
		CC1101_writeReg(CC1101_FREQ2, CC1101_DEFVAL_FREQ2_315);
		CC1101_writeReg(CC1101_FREQ1, CC1101_DEFVAL_FREQ1_315);
		CC1101_writeReg(CC1101_FREQ0, CC1101_DEFVAL_FREQ0_315);
		break;
	default:
		CC1101_writeReg(CC1101_FREQ2, CC1101_DEFVAL_FREQ2_868);
		CC1101_writeReg(CC1101_FREQ1, CC1101_DEFVAL_FREQ1_868);
		CC1101_writeReg(CC1101_FREQ0, CC1101_DEFVAL_FREQ0_868);
		break;
	}

	CC1101_readReg(CC1101_FREQ2, CC1101_CONFIG_REGISTER);
	CC1101_readReg(CC1101_FREQ1, CC1101_CONFIG_REGISTER);
	CC1101_readReg(CC1101_FREQ0, CC1101_CONFIG_REGISTER);

	CC1101.carrierFreq = freq;
}

static u8_t marcstate_get(void) {
	byte marcState = 0xFF;
	u8_t count = 0;
	while (1) {
		byte m = readStatusReg(CC1101_MARCSTATE) & 0x1F;
		if (marcState == m) {
			if(count>3){
				return m;
			}
			count ++;
		}else{
			count = 0;
		}
		marcState = m;
	}

}

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
boolean CC1101_sendData(CCPACKET packet) {
	byte marcState;
	// Declare to be in Tx state. This will avoid receiving packets whilst
	// transmitting
	CC1101.rfState = RFSTATE_TX;

	// Enter RX state
	marcState = marcstate_get();
	if (marcState != 0x0D) {
		setRxState();
		// Check that the RX state has been entered
		if (marcState == 0x11) {        // RX_OVERFLOW
			setIdleState();       // Enter IDLE state
//			flushTxFifo();        // Flush Tx FIFO
			flushRxFifo();
			setRxState();         // Back to RX state
		}
		if (marcState == 0x16) {        // TX_UNDERFLOW
			setIdleState();       // Enter IDLE state
			flushTxFifo();        // Flush Tx FIFO
//			flushRxFifo();
			setRxState();         // Back to RX state
		}
	}
	// Set data length at the first position of the TX FIFO

	CC1101_writeReg(CC1101_TXFIFO, packet.length);
	// Write data into the TX FIFO
	CC1101_writeBurstTXFIFO(CC1101_TXFIFO_BURST, &packet.data[0],
			packet.length);
	// CCA enabled: will enter TX state only if the channel is clear
	setTxState();

	// Check that TX state is being entered (state = RXTX_SETTLING)

//	while (marcstate_get() <= 0x10) {
//		dbg_printf("TX SETTLING FAIL MARCSTATE %x\n", marcstate_get());
//		if (marcstate_get() == 0x01 || marcstate_get() == 0x0D) {
//			return false;
//			setTxState();
//		}
//	}
	marcState = marcstate_get();
	if ((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15)) {
		setIdleState();       // Enter IDLE state
		flushTxFifo();        // Flush Tx FIFO
//		flushRxFifo();
		setRxState();         // Back to RX state

		// Declare to be in Rx state
		CC1101.rfState = RFSTATE_RX;
		dbg_printf("TX FAIL MARCSTATE %x\n",
		marcstate_get());
		return false;
	}

	// Wait for the sync word to be transmitted
	while (!interrupt_state)
		;
	while (interrupt_state)
		;
//	hw->wait_GDO0_high();
//
//	// Wait until the end of the packet transmission
//	hw->wait_GDO0_low();
	// Check that the TX FIFO is empty
//	marcState = marcState;
//	dbg_printf("After send %x\n",marcState);
//	if (marcState == 0x16 && !CC1101_tx_fifo_empty()) {
//		flushTxFifo();
//	}
		setRxState();
	return true;
}
boolean CC1101_tx_fifo_empty(void) {
	if ((readStatusReg(CC1101_TXBYTES) & 0x7F) == 0) {
		return true;
	}
	return false;

}
boolean CC1101_rx_mode(void) {
	byte marcState;
	// Enter RX state
	setRxState();
	// Check that the RX state has been entered
	while (((marcState = marcstate_get()) & 0x1F) != 0x0D) {
		if (marcState == 0x11)        // RX_OVERFLOW
			flushRxFifo();              // flush receive queue

		if (marcState == 0x16)        // TX_UNDERFLOW
			flushTxFifo();              // flush receive queue

//		dbg_printf("set rx mode fail MARCSTATE %x \n", marcState);

		return false;
	}

	return true;
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
byte CC1101_receiveData(CCPACKET * packet) {

	byte val;
	// Rx FIFO overflow?
	if (((val = readStatusReg(CC1101_MARCSTATE)) & 0x1F) == 0x11) {
		setIdleState();       // Enter IDLE state
		flushRxFifo();        // Flush Rx FIFO
		//CC1101_cmdStrobe(CC1101_SFSTXON);
		packet->length = 0;
	}
	// Any byte waiting to be read?
	// Read data length
	else if ((packet->length = readConfigReg(CC1101_RXFIFO))) {

		// If packet is too long
		if (packet->length > CC1101_DATA_LEN) {
			flushRxFifo();
			setRxState();
			packet->crc_ok = 0;
			packet->length = 0;   // Discard packet
		} else {
			// Read data packet
			CC1101_readBurstReg(packet->data, CC1101_RXFIFO, packet->length);
			// Read RSSI
			packet->rssi = readConfigReg(CC1101_RXFIFO);
			// Read LQI and CRC_OK
			val = readConfigReg(CC1101_RXFIFO);
			packet->lqi = val & 0x7F;
			packet->crc_ok = (val & 0x80);
		}
	} else {
		packet->length = 0;
	}
	if (!packet->crc_ok) {
		flushRxFifo();
	}
//	CC1101_cmdStrobe(CC1101_SRX);
	// Back to RX state
	setRxState();

	return packet->length;
}

void CC1101_interrupt(u8_t state) {
	interrupt_state = state;
}
