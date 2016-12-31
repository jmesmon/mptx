
/*  **************************
	* By Midelic on RCGroups *
	**************************
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

#if defined(FRSKYX_CC2500_INO)

#include "FrSkyX_cc2500.hh"
#include "Multiprotocol.h"
#include "CC2500_SPI.hh"
#include "Telemetry.hh"
#include "Common.hh"
#include "iface_cc2500.h"

uint8_t chanskip;
uint8_t counter_rst;
uint8_t ctr;

static void __attribute__((unused)) set_start(uint8_t ch )
{
	CC2500_Strobe(CC2500_SIDLE);
	CC2500_WriteReg(CC2500_25_FSCAL1, calData[ch]);
	CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[ch]);
}		

static void __attribute__((unused)) frskyX_init()
{
	for(uint8_t i=0;i<36;i++)
	{
		uint8_t reg=pgm_read_byte_near(&cc2500_conf[i][0]);
		uint8_t val=pgm_read_byte_near(&cc2500_conf[i][1]);
		
		if(reg==CC2500_06_PKTLEN)
			val=0x1E;
		else
		if(reg==CC2500_08_PKTCTRL0)
			val=0x01;
		else
		if(reg==CC2500_0B_FSCTRL1)
			val=0x0A;
		else
		if(reg==CC2500_10_MDMCFG4)
			val=0x7B;
		else
		if(reg==CC2500_11_MDMCFG3)
			val=0x61;
		else
		if(reg==CC2500_12_MDMCFG2)
			val=0x13;
		else
		if(reg==CC2500_15_DEVIATN)
			val=0x51;
		
		CC2500_WriteReg(reg,val);
	}

	CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);			
	prev_option = option ;
	CC2500_WriteReg(CC2500_0C_FSCTRL0, option);
	CC2500_Strobe(CC2500_SIDLE);    
	//
	for(uint8_t c=0;c < 48;c++)
	{//calibrate hop channels
		CC2500_Strobe(CC2500_SIDLE);    
		CC2500_WriteReg(CC2500_0A_CHANNR,hopping_frequency[c]);
		CC2500_Strobe(CC2500_SCAL);
		delayMicroseconds(900);//
		calData[c] = CC2500_ReadReg(CC2500_25_FSCAL1);
	}
	//#######END INIT########		
}

static void __attribute__((unused)) initialize_data(uint8_t adr)
{
	CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
	CC2500_WriteReg(CC2500_18_MCSM0,    0x8);	
	CC2500_WriteReg(CC2500_09_ADDR, adr ? 0x03 : rx_tx_addr[3]);
	CC2500_WriteReg(CC2500_07_PKTCTRL1,0x05);
}

//**CRC**
const uint16_t PROGMEM CRC_Short[]={
	0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
	0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7 };
static uint16_t CRCTable(uint8_t val)
{
	uint16_t word ;
	word = pgm_read_word(&CRC_Short[val&0x0F]) ;
	val /= 16 ;
	return word ^ (0x1081 * val) ;
}
static uint16_t __attribute__((unused)) crc_x(uint8_t *data, uint8_t len)
{
	uint16_t crc = 0;
	for(uint8_t i=0; i < len; i++)
		crc = (crc<<8) ^ CRCTable((uint8_t)(crc>>8) ^ *data++);
	return crc;
}

 // 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
 //64=860,1024=1500,1984=2140//Taranis 125%

static uint16_t  __attribute__((unused)) scaleForPXX( uint8_t i )
{	//mapped 860,2140(125%) range to 64,1984(PXX values);
	return (uint16_t)(((Servo_data[i]-servo_min_125)*3)>>1)+64;
}

static void __attribute__((unused)) frskyX_build_bind_packet()
{
	packet[0] = 0x1D;       
	packet[1] = 0x03;          
	packet[2] = 0x01;               
	//	
	packet[3] = rx_tx_addr[3];
	packet[4] = rx_tx_addr[2];
	int idx = ((state -FRSKY_BIND) % 10) * 5;
	packet[5] = idx;	
	packet[6] = hopping_frequency[idx++];
	packet[7] = hopping_frequency[idx++];
	packet[8] = hopping_frequency[idx++];
	packet[9] = hopping_frequency[idx++];
	packet[10] = hopping_frequency[idx++];
	packet[11] = 0x02;
	packet[12] = RX_num;
	//
	memset(&packet[13], 0, 15);	
	uint16_t lcrc = crc_x(&packet[3], 25);	
	//
	packet[28] = lcrc >> 8;
	packet[29] = lcrc;
	//
}

static void __attribute__((unused)) frskyX_data_frame()
{
	//0x1D 0xB3 0xFD 0x02 0x56 0x07 0x15 0x00 0x00 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x00 0x04 0x40 0x08 0x00 0x00 0x00 0x00 0x00 0x00 0x96 0x12
	//
	static uint8_t lpass;
	uint16_t chan_0 ;
	uint16_t chan_1 ; 
	uint8_t startChan = 0;
	//
	packet[0] = 0x1D; 
	packet[1] = rx_tx_addr[3];
	packet[2] = rx_tx_addr[2];
	packet[3] = 0x02;
	//  
	packet[4] = (ctr<<6)+hopping_frequency_no; 
	packet[5] = counter_rst;
	packet[6] = RX_num;
	//packet[7] = FLAGS 00 - standard packet
	//10, 12, 14, 16, 18, 1A, 1C, 1E - failsafe packet
	//20 - range check packet
	packet[7] = 0;
	packet[8] = 0;		
	//
	if ( lpass & 1 )
		startChan += 8 ;
	
	for(uint8_t i = 0; i <12 ; i+=3)
	{//12 bytes
		chan_0 = scaleForPXX(startChan);		 
		if(lpass & 1 )
			chan_0+=2048;			
		startChan+=1;
		//
		chan_1 = scaleForPXX(startChan);		
		if(lpass & 1 )
			chan_1+= 2048;		
		startChan+=1;
		//
		packet[9+i] = lowByte(chan_0);//3 bytes*4
		packet[9+i+1]=(((chan_0>>8) & 0x0F)|(chan_1 << 4));
		packet[9+i+2]=chan_1>>4;
	}

	packet[21] = seq_last_sent << 4 | seq_last_rcvd;//8 at start
	if (seq_last_sent < 0x08 && seq_last_rcvd < 8)
		seq_last_sent = (seq_last_sent + 1) % 4;
	else if (seq_last_rcvd == 0x00)
		seq_last_sent = 1;
	
	if(sub_protocol== CH_8 )// in X8 mode send only 8ch every 9ms
		lpass = 0 ;
	else
		lpass += 1 ;
	
	for (uint8_t i=22;i<28;i++)
		packet[i]=0;
	uint16_t lcrc = crc_x(&packet[3], 25);
	
	packet[28]=lcrc>>8;//high byte
	packet[29]=lcrc;//low byte
}

uint16_t ReadFrSkyX()
{
	switch(state)
	{	
		default: 
			set_start(47);		
			CC2500_SetPower();
			CC2500_Strobe(CC2500_SFRX);
			//		
			frskyX_build_bind_packet();
			CC2500_Strobe(CC2500_SIDLE);
			CC2500_WriteData(packet, packet[0]+1);
			state++;
			return 9000;
		case FRSKY_BIND_DONE:
			initialize_data(0);
			hopping_frequency_no=0;
			BIND_DONE;
			state++;			
			break;		
		case FRSKY_DATA1:
			if ( prev_option != option )
			{
				CC2500_WriteReg(CC2500_0C_FSCTRL0,option);	// Frequency offset hack 
				prev_option = option ;
			}
			CC2500_SetTxRxMode(TX_EN);
			set_start(hopping_frequency_no);
			CC2500_SetPower();		
			CC2500_Strobe(CC2500_SFRX);
			hopping_frequency_no = (hopping_frequency_no+chanskip)%47;
			CC2500_Strobe(CC2500_SIDLE);		
			CC2500_WriteData(packet, packet[0]+1);
			//
			frskyX_data_frame();
			state++;
			return 5500;
		case FRSKY_DATA2:
			CC2500_SetTxRxMode(RX_EN);
			CC2500_Strobe(CC2500_SIDLE);
			state++;
			return 200;
		case FRSKY_DATA3:		
			CC2500_Strobe(CC2500_SRX);
			state++;
			return 3000;
		case FRSKY_DATA4:
			len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;	
			if (len && (len<=(0x0E + 3)))				//Telemetry frame is 17
			{
				counter=0;
				CC2500_ReadData(pkt, len);
				#if defined TELEMETRY
					frsky_check_telemetry(pkt,len);	//check if valid telemetry packets
					//parse telemetry packets here
					//The same telemetry function used by FrSky(D8).
				#endif
			} 
			else
			{
				counter++;
				// restart sequence on missed packet - might need count or timeout instead of one missed
				if(counter>100)
				{//~1sec
					seq_last_sent = 0;
					seq_last_rcvd = 8;
					counter=0;
					#if defined TELEMETRY
						telemetry_lost=1;
					#endif
				}
				CC2500_Strobe(CC2500_SFRX);			//flush the RXFIFO
			}
			state = FRSKY_DATA1;
			return 300;
	}		
	return 1;		
}

uint16_t initFrSkyX()
{
	set_rx_tx_addr(MProtocol_id_master);
	Frsky_init_hop();
	while(!chanskip)
		chanskip=random(0xfefefefe)%47;
	while((chanskip-ctr)%4)
		ctr=(ctr+1)%4;
	counter_rst=(chanskip-ctr)>>2;

	//for test***************
	//rx_tx_addr[3]=0xB3;
	//rx_tx_addr[2]=0xFD;
	//************************
	hopping_frequency[47]=0;
	frskyX_init();
	CC2500_SetTxRxMode(TX_EN);
	//
	if(IS_AUTOBIND_FLAG_on)
	{	   
		state = FRSKY_BIND;
		initialize_data(1);
	}
	else
	{
		state = FRSKY_DATA1;
		initialize_data(0);
	}
	seq_last_sent = 0;
	seq_last_rcvd = 8;
	return 10000;
}	
#endif
