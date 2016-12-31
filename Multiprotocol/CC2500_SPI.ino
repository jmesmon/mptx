
/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
//-------------------------------
//-------------------------------
//CC2500 SPI routines
//-------------------------------
//-------------------------------
#ifdef CC2500_INSTALLED
#include "Pins.h"
#include "SPI.hh"
#include "Arduino.hh"
#include "CC2500_SPI.hh"
#include "iface_cc2500.h"
#include "Multiprotocol.hh"

//----------------------------
void CC2500_WriteReg(uint8_t address, uint8_t data)
{
	CC25_CSN_off;
	SPI_Write(address); 
	NOP();
	SPI_Write(data);
	CC25_CSN_on;
} 

//----------------------
static void CC2500_ReadRegisterMulti(uint8_t address, uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	SPI_Write(CC2500_READ_BURST | address);
	for(uint8_t i = 0; i < length; i++)
		data[i] = SPI_Read();
	CC25_CSN_on;
}

//--------------------------------------------
uint8_t CC2500_ReadReg(uint8_t address)
{ 
	uint8_t result;
	CC25_CSN_off;
	SPI_Write(CC2500_READ_SINGLE | address);
	result = SPI_Read();  
	CC25_CSN_on;
	return(result); 
} 

//------------------------
void CC2500_ReadData(uint8_t *dpbuffer, uint8_t len)
{
	CC2500_ReadRegisterMulti(CC2500_3F_RXFIFO, dpbuffer, len);
}

//*********************************************
void CC2500_Strobe(uint8_t state)
{
	CC25_CSN_off;
	SPI_Write(state);
	CC25_CSN_on;
}

static void CC2500_WriteRegisterMulti(uint8_t address, const uint8_t data[], uint8_t length)
{
	CC25_CSN_off;
	SPI_Write(CC2500_WRITE_BURST | address);
	for(uint8_t i = 0; i < length; i++)
		SPI_Write(data[i]);
	CC25_CSN_on;
}

void CC2500_WriteData(uint8_t *dpbuffer, uint8_t len)
{
	CC2500_Strobe(CC2500_SFTX);
	CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, dpbuffer, len);
	CC2500_Strobe(CC2500_STX);
}

void CC2500_SetTxRxMode(uint8_t mode)
{
	if(mode == TX_EN)
	{//from deviation firmware
		CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
		CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F | 0x40);
	}
	else
		if (mode == RX_EN)
		{
			CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
			CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F | 0x40);
		}
		else
		{
			CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
			CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
		}
}

//------------------------
/*static void cc2500_resetChip(void)
{
	// Toggle chip select signal
	CC25_CSN_on;
	delayMicroseconds(30);
	CC25_CSN_off;
	delayMicroseconds(30);
	CC25_CSN_on;
	delayMicroseconds(45);
	CC2500_Strobe(CC2500_SRES);
	_delay_ms(100);
}
*/
uint8_t CC2500_Reset()
{
	CC2500_Strobe(CC2500_SRES);
	delayMilliseconds(1);
	CC2500_SetTxRxMode(TXRX_OFF);
	return CC2500_ReadReg(CC2500_0E_FREQ1) == 0xC4;//check if reset
}
/*
static void CC2500_SetPower_Value(uint8_t power)
{
	const unsigned char patable[8]=	{
		0xC5,  // -12dbm
		0x97, // -10dbm
		0x6E, // -8dbm
		0x7F, // -6dbm
		0xA9, // -4dbm
		0xBB, // -2dbm
		0xFE, // 0dbm
		0xFF // 1.5dbm
	};
	if (power > 7)
		power = 7;
	CC2500_WriteReg(CC2500_3E_PATABLE,  patable[power]);
}
*/
void CC2500_SetPower()
{
	uint8_t power=CC2500_BIND_POWER;
	if(IS_BIND_DONE_on)
		power=IS_POWER_FLAG_on?CC2500_HIGH_POWER:CC2500_LOW_POWER;
	if(IS_RANGE_FLAG_on)
		power=CC2500_RANGE_POWER;
	if(prev_power != power)
	{
		CC2500_WriteReg(CC2500_3E_PATABLE, power);
		prev_power=power;
	}
}

const PROGMEM uint8_t cc2500_conf[][2]={
	{ CC2500_02_IOCFG0, 0x06 },
	{ CC2500_00_IOCFG2, 0x06 },
	{ CC2500_17_MCSM1, 0x0c },
	{ CC2500_18_MCSM0, 0x18 },
	{ CC2500_06_PKTLEN, 0x19 },
	{ CC2500_07_PKTCTRL1, 0x04 },
	{ CC2500_08_PKTCTRL0, 0x05 },
	{ CC2500_3E_PATABLE, 0xff },
	{ CC2500_0B_FSCTRL1, 0x08 },
	{ CC2500_0C_FSCTRL0, 0x00 },	// option
	{ CC2500_0D_FREQ2, 0x5c },
	{ CC2500_0E_FREQ1, 0x76 },
	{ CC2500_0F_FREQ0, 0x27 },
	{ CC2500_10_MDMCFG4, 0xAA },
	{ CC2500_11_MDMCFG3, 0x39 },
	{ CC2500_12_MDMCFG2, 0x11 },
	{ CC2500_13_MDMCFG1, 0x23 },
	{ CC2500_14_MDMCFG0, 0x7a },
	{ CC2500_15_DEVIATN, 0x42 },
	{ CC2500_19_FOCCFG, 0x16 },
	{ CC2500_1A_BSCFG, 0x6c },
	{ CC2500_1B_AGCCTRL2, 0x43 },	// bind ? 0x43 : 0x03
	{ CC2500_1C_AGCCTRL1,0x40 },
	{ CC2500_1D_AGCCTRL0,0x91 },
	{ CC2500_21_FREND1, 0x56 },
	{ CC2500_22_FREND0, 0x10 },
	{ CC2500_23_FSCAL3, 0xa9 },
	{ CC2500_24_FSCAL2, 0x0A },
	{ CC2500_25_FSCAL1, 0x00 },
	{ CC2500_26_FSCAL0, 0x11 },
	{ CC2500_29_FSTEST, 0x59 },
	{ CC2500_2C_TEST2, 0x88 },
	{ CC2500_2D_TEST1, 0x31 },
	{ CC2500_2E_TEST0, 0x0B },
	{ CC2500_03_FIFOTHR, 0x07 },
	{ CC2500_09_ADDR, 0x00 }
};
#endif
