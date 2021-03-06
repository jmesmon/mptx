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
//**************************
// Telemetry serial code   *
//**************************
#if defined TELEMETRY
#include "Telemetry.hh"
#include "Multiprotocol.h"
#include "serial.hh"

uint8_t v_lipo1;
uint8_t v_lipo2;
int16_t RSSI_dBm;
uint8_t TX_RSSI;
uint8_t telemetry_link=0;
uint8_t telemetry_counter=0;
uint8_t telemetry_lost;

#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
uint8_t seq_last_sent;
uint8_t seq_last_rcvd;
#endif

static uint8_t pass = 0;

void telemetry_reset(void)
{
    tx_pause();
    pass=0;
    telemetry_link=0;
    telemetry_lost=1;
    serial_reset_buf();
    TX_RX_PAUSE_off;
    TX_MAIN_PAUSE_off;
}

/* valid lenght of pktt */
static uint8_t pktt_len;
static uint8_t pktt[MAX_PKT];

#if defined MULTI_TELEMETRY
	#define MULTI_TIME 250 //in ms
	static uint32_t lastMulti = 0;
#endif

#if defined SPORT_TELEMETRY	
    #define SPORT_TIME 12000  //12ms
	#define FRSKY_SPORT_PACKET_SIZE   8
	static uint32_t last = 0;
	static uint8_t sport_counter=0;
	static uint8_t RxBt = 0;
	static uint8_t rssi;
	static uint8_t sport = 0;
#endif
#if defined HUB_TELEMETRY
	#define USER_MAX_BYTES 6
	static uint8_t prev_index;
#endif

#define START_STOP              0x7e
#define BYTESTUFF               0x7d
#define STUFF_MASK              0x20
#define MAX_PKTX 10
static uint8_t pktx[MAX_PKTX];
static uint8_t pktx1[MAX_PKTX];
static uint8_t indx;
static uint8_t frame[18];

#ifdef MULTI_TELEMETRY
static void multi_send_header(uint8_t type, uint8_t len)
{
    Serial_write('M');
    Serial_write('P');
    Serial_write(type);
    Serial_write(len);
}

static void multi_send_frskyhub()
{
    multi_send_header(MULTI_TELEMETRY_HUB, 9);
    for (uint8_t i = 0; i < 9; i++)
        Serial_write(frame[i]);
}

static void multi_send_status()
{
    multi_send_header(MULTI_TELEMETRY_STATUS, 5);

    // Build flags
    uint8_t flags=0;
    if (IS_INPUT_SIGNAL_on)
        flags |= 0x01;
    if (mode_select==MODE_SERIAL)
        flags |= 0x02;
    if (remote_callback != 0)
        flags |= 0x04;
    if (!IS_BIND_DONE_on)
        flags |= 0x08;
    Serial_write(flags);

    // Version number example: 1.1.6.1
    Serial_write(VERSION_MAJOR);
    Serial_write(VERSION_MINOR);
    Serial_write(VERSION_REVISION);
    Serial_write(VERSION_PATCH_LEVEL);
}
#endif

#ifdef DSM_TELEMETRY
	#ifdef MULTI_TELEMETRY
		static void DSM_frame(void)
		{
			if (pkt[0] == 0x80)
			{
				multi_send_header(MULTI_TELEMETRY_DSMBIND, 10);
				for (uint8_t i = 1; i < 11; i++) 	// 10 bytes of DSM bind response
					Serial_write(pkt[i]);

			}
			else
			{
				multi_send_header(MULTI_TELEMETRY_DSM, 17);
				for (uint8_t i = 0; i < 17; i++)	// RSSI value followed by 16 bytes of telemetry data
					Serial_write(pkt[i]);
			}
		}
	#else
		static void DSM_frame(void)
		{
			Serial_write(0xAA);						// Telemetry packet
			for (uint8_t i = 0; i < 17; i++)		// RSSI value followed by 16 bytes of telemetry data
				Serial_write(pkt[i]);
		}
	#endif
#endif

#ifdef AFHDS2A_FW_TELEMETRY
	static void AFHDSA_short_frame(void)
	{
		#if defined MULTI_TELEMETRY
			multi_send_header(MULTI_TELEMETRY_AFHDS2A, 29);
		#else
			Serial_write(0xAA);						// Telemetry packet
		#endif
		for (uint8_t i = 0; i < 29; i++)			// RSSI value followed by 4*7 bytes of telemetry data
			Serial_write(pkt[i]);
	}
#endif

static void frskySendStuffed(void)
{
	Serial_write(START_STOP);
	for (uint8_t i = 0; i < 9; i++)
	{
		if ((frame[i] == START_STOP) || (frame[i] == BYTESTUFF))
		{
			Serial_write(BYTESTUFF);	    	  
			frame[i] ^= STUFF_MASK;	
		}
		Serial_write(frame[i]);
	}
	Serial_write(START_STOP);
}

static void compute_RSSIdbm(void)
{
	
	RSSI_dBm = (((uint16_t)(pktt[pktt_len-2])*18)>>4);
	if(pktt[pktt_len-2] >=128)
		RSSI_dBm -= 164;
	else
		RSSI_dBm += 130;
}

void frsky_check_telemetry(uint8_t *pkt,uint8_t len)
{
	if(pkt[1] == rx_tx_addr[3] && pkt[2] == rx_tx_addr[2] && len ==(pkt[0] + 3))
	{
		pktt_len = len;
		for (uint8_t i=3;i<len;i++)
			pktt[i]=pkt[i];				 
		telemetry_link=1;
		if(pktt[6])
			telemetry_counter=(telemetry_counter+1)%32;
		//
#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
		telemetry_lost=0;
		if (protocol==MODE_FRSKYX)
		{
			if ((pktt[5] >> 4 & 0x0f) == 0x08)
			{  
				seq_last_sent = 8;
				seq_last_rcvd = 0;
				pass=0;
			} 
			else
			{
				if ((pktt[5] >> 4 & 0x03) == (seq_last_rcvd + 1) % 4)
					seq_last_rcvd = (seq_last_rcvd + 1) % 4;
				else
					pass=0;//reset if sequence wrong
			}
		}
#endif
	}
}

void init_hub_telemetry(void)
{
	telemetry_link=0;
	telemetry_counter=0;
	v_lipo1=0;
	v_lipo2=0;
	RSSI_dBm=0;
	TX_RSSI=0;
}

static void frsky_link_frame(void)
{
	frame[0] = 0xFE;
	if (protocol==MODE_FRSKYD)
	{		
		compute_RSSIdbm();				
		frame[1] = pktt[3];
		frame[2] = pktt[4];
		frame[3] = pktt[5];
		frame[4] = (uint8_t)RSSI_dBm;
	}
	else
		if (protocol==MODE_HUBSAN||protocol==MODE_AFHDS2A||protocol==MODE_BAYANG)
		{	
			frame[1] = v_lipo1;
			frame[2] = v_lipo2;			
			frame[3] = (uint8_t)RSSI_dBm;
			frame[4] = TX_RSSI;
		}
	frame[5] = frame[6] = frame[7] = frame[8] = 0;
	#if defined MULTI_TELEMETRY
		multi_send_frskyhub();
	#else
		frskySendStuffed();
	#endif
}

#if defined HUB_TELEMETRY
static void frsky_user_frame(void)
{
	uint8_t indexx = 0,  j=8, i;
	//uint8_t c=0, n=0;
	
	if(pktt[6]>0 && pktt[6]<=10)
	{//only valid hub frames	  
		frame[0] = 0xFD;
		frame[2] = pktt[7];		
		switch(pass)
		{
			case 0:
				indexx=pktt[6];
				for(i=0;i<indexx;i++)
				{
//					if(pktt[j]==0x5E)
//					{
//						if(c++)
//						{
//							c=0;
//							n++;
//							j++;
//						}
//					}
					pktx[i]=pktt[j++];
				}	
//				indexx = indexx-n;
				pass=1;
				
			case 1:
				indx=indexx;
				prev_index = indexx; 
				if(indx<USER_MAX_BYTES)
				{   			
					for(i=0;i<indx;i++)
						frame[i+3]=pktx[i];
					pktt[6]=0;
					pass=0;
				}
				else
				{
					indx = USER_MAX_BYTES;
					for(i=0;i<indx;i++)
						frame[i+3]=pktx[i];
					pass=2;
				}			
				break;
			case 2:		
				indx = prev_index - indx;
				prev_index=0;
				if(indx<=(MAX_PKTX-USER_MAX_BYTES))	//10-6=4
					for(i=0;i<indx;i++)
						frame[i+3]=pktx[USER_MAX_BYTES+i];
				pass=0;
				pktt[6]=0; 
				break;
			default:
				break;
		}
		if(!indx)
			return;
		frame[1] = indx;
		#if defined MULTI_TELEMETRY
			multi_send_frskyhub();
		#else
			frskySendStuffed();
		#endif
	}
	else
		pass=0;
}	   
#endif

/*
HuB RX packets.
pkt[6]|(counter++)|00 01 02 03 04 05 06 07 08 09 
        %32        
01     08          5E 28 12 00 5E 5E 3A 06 00 5E
0A     09          28 12 00 5E 5E 3A 06 00 5E 5E  
09     0A          3B 09 00 5E 5E 06 36 7D 5E 5E 
03     0B          5E 28 11 00 5E 5E 06 06 6C 5E
0A     0C          00 5E 5E 3A 06 00 5E 5E 3B 09
07     0D          00 5E 5E 06 06 6C 5E 16 72 5E
05     0E          5E 28 11 00 5E 5E 3A 06 00 5E
0A     0F          5E 3A 06 00 5E 5E 3B 09 00 5E
05     10          5E 06 16 72 5E 5E 3A 06 00 5E
*/

#if defined SPORT_TELEMETRY
/* SPORT details serial
			100K 8E2 normal-multiprotocol
			-every 12ms-or multiple of 12; %36
			1  2  3  4  5  6  7  8  9  CRC DESCR
			7E 98 10 05 F1 20 23 0F 00 A6 SWR_ID 
			7E 98 10 01 F1 33 00 00 00 C9 RSSI_ID 
			7E 98 10 04 F1 58 00 00 00 A1 BATT_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 
			7E BA 10 03 F1 E2 00 00 00 18 ADC2_ID 	
			
			
			Telemetry frames(RF) SPORT info 
			15 bytes payload
			SPORT frame valid 6+3 bytes
			[00] PKLEN  0E 0E 0E 0E 
			[01] TXID1  DD DD DD DD 
			[02] TXID2  6D 6D 6D 6D 
			[03] CONST  02 02 02 02 
			[04] RS/RB  2C D0 2C CE	//D0;CE=2*RSSI;....2C = RX battery voltage(5V from Bec)
			[05] HD-SK  03 10 21 32	//TX/RX telemetry hand-shake bytes
			[06] NO.BT  00 00 06 03	//No.of valid SPORT frame bytes in the frame		
			[07] STRM1  00 00 7E 00 
			[08] STRM2  00 00 1A 00 
			[09] STRM3  00 00 10 00 
			[10] STRM4  03 03 03 03  
			[11] STRM5  F1 F1 F1 F1 
			[12] STRM6  D1 D1 D0 D0
			[13] CHKSUM1 --|2 CRC bytes sent by RX (calculated on RX side crc16/table)
			[14] CHKSUM2 --|
			+2	appended bytes automatically  RSSI and LQI/CRC bytes(len=0x0E+3);
			
0x06	0x06	0x06	0x06	0x06

0x7E	0x00	0x03	0x7E	0x00
0x1A	0x00	0xF1	0x1A	0x00
0x10	0x00	0xD7	0x10	0x00
0x03	0x7E	0x00	0x03	0x7E
0xF1	0x1A	0x00	0xF1	0x1A
0xD7	0x10	0x00	0xD7	0x10

0xE1	0x1C	0xD0	0xEE	0x33
0x34	0x0A	0xC3	0x56	0xF3
				
		*/
#ifdef MULTI_TELEMETRY
	static void sportSend(uint8_t *p)
	{
		multi_send_header(MULTI_TELEMETRY_SPORT, 9);
		uint16_t crc_s = 0;
		Serial_write(p[0]) ;
		for (uint8_t i = 1; i < 9; i++)
		{
			if (i == 8)
				p[i] = 0xff - crc_s;
				Serial_write(p[i]);

			if (i>0)
			{
				crc_s += p[i]; //0-1FF
				crc_s += crc_s >> 8; //0-100
				crc_s &= 0x00ff;
			}
		}
	}
#else
	static void sportSend(uint8_t *p)
	{
		uint16_t crc_s = 0;
		Serial_write(START_STOP);//+9
		Serial_write(p[0]) ;
		for (uint8_t i = 1; i < 9; i++)
		{
			if (i == 8)
				p[i] = 0xff - crc_s;
			
			if ((p[i] == START_STOP) || (p[i] == BYTESTUFF))
			{
				Serial_write(BYTESTUFF);//stuff again
				Serial_write(STUFF_MASK ^ p[i]);
			} 
			else			
				Serial_write(p[i]);					
			
			if (i>0)
			{
				crc_s += p[i]; //0-1FF
				crc_s += crc_s >> 8; //0-100
				crc_s &= 0x00ff;
			}
		}
	}
#endif

static void sportIdle(void)
{
	#if !defined MULTI_TELEMETRY
		Serial_write(START_STOP);
	#endif
}	

static void sportSendFrame(void)
{
	uint8_t i;
	sport_counter = (sport_counter + 1) %36;
	if(telemetry_lost)
	{
		sportIdle();
		return;
	}
	if(sport_counter<6)
	{
		frame[0] = 0x98;
		frame[1] = 0x10;
		for (i=5;i<8;i++)
		frame[i]=0;
	}
	switch (sport_counter)
	{
		case 0:
			frame[2] = 0x05;
			frame[3] = 0xf1;
			frame[4] = 0x02 ;//dummy values if swr 20230f00
			frame[5] = 0x23;
			frame[6] = 0x0F;
			break;
		case 2: // RSSI
			frame[2] = 0x01;
			frame[3] = 0xf1;
			frame[4] = rssi;
			break;
		case 4: //BATT
			frame[2] = 0x04;
			frame[3] = 0xf1;
			frame[4] = RxBt;//a1;
			break;								
		default:
			if(sport)
			{	
				for (i=0;i<FRSKY_SPORT_PACKET_SIZE;i++)
				frame[i]=pktx1[i];
				sport=0;
				break;
			}
			else
			{
				sportIdle();
				return;
			}		
	}
	sportSend(frame);
}	

static void proces_sport_data(uint8_t data)
{
	switch (pass)
	{
		case 0:
			if (data == START_STOP)
			{//waiting for 0x7e
				indx = 0;
				pass = 1;
			}
			break;		
		case 1:
			if (data == START_STOP)	// Happens if missed packet
			{//waiting for 0x7e
				indx = 0;
				pass = 1;
				break;		
			}
			if(data == BYTESTUFF)//if they are stuffed
				pass=2;
			else
				if (indx < MAX_PKTX)		
					pktx[indx++] = data;		
			break;
		case 2:	
			if (indx < MAX_PKTX)	
				pktx[indx++] = data ^ STUFF_MASK;	//unstuff bytes	
			pass=1;
			break;	
	} // end switch
	if (indx >= FRSKY_SPORT_PACKET_SIZE)
	{//8 bytes no crc 
		if ( sport )
		{
			// overrun!
		}
		else
		{
			uint8_t i ;
			for ( i = 0 ; i < FRSKY_SPORT_PACKET_SIZE ; i += 1 )
			{
				pktx1[i] = pktx[i] ;	// Double buffer
			}
			sport = 1;//ok to send
		}
		pass = 0;//reset
	}
}

#endif

#ifdef MULTI_TELEMETRY
/* return value is "should skip remaining telemetry updates" */
static bool telem_update_multi(void)
{
	uint32_t now = millis();
	if ((now - lastMulti) > MULTI_TIME) {
		multi_send_status();
		lastMulti = now;
		return true;
	}

	return false;
}
#else
static inline bool telem_update_multi(void) { return false; }
#endif

#ifdef SPORT_TELEMETRY
static void telem_update_sport(void)
{
	if (protocol==MODE_FRSKYX)
	{	// FrSkyX
		if(telemetry_link)
		{
			if(pktt[4] & 0x80)
				rssi=pktt[4] & 0x7F ;
			else
				RxBt = (pktt[4]<<1) + 1 ;
			if(pktt[6]<=6)
				for (uint8_t i=0; i < pktt[6]; i++)
					proces_sport_data(pktt[7+i]);
			telemetry_link=0;
		}
		uint32_t now = micros();
		if ((now - last) > SPORT_TIME)
		{
			sportSendFrame();
#ifdef STM32_BOARD
			last=now;
#else
			last += SPORT_TIME ;
#endif
		}
	}
}
#else
static inline void telem_update_sport(void) {}
#endif

#ifdef DSM_TELEMETRY
static bool telem_update_dsm(void)
{
	if(telemetry_link && protocol == MODE_DSM )
	{	// DSM
		DSM_frame();
		telemetry_link=0;
		return true;
	}

	return false;
}
#else
static inline bool telem_update_dsm(void) { return false; }
#endif

#ifdef AFHDS2A_FW_TELEMETRY
static void telem_update_afhds2a(void) {
	if(telemetry_link == 2 && protocol == MODE_AFHDS2A)
	{
		AFHDSA_short_frame();
		telemetry_link=0;
	}
}
#else
static inline void telem_update_afhds2a(void) {}
#endif

static bool telem_update_frsky(void)
{
	if(telemetry_link && protocol != MODE_FRSKYX )
	{	// FrSkyD + Hubsan + AFHDS2A + Bayang
		frsky_link_frame();
		telemetry_link=0;
		return true;
	}

	return false;
}

#ifdef HUB_TELEMETRY
static void telem_update_hub(void)
{
	if(!telemetry_link && protocol == MODE_FRSKYD)
	{	// FrSky
		frsky_user_frame();
	}
}
#else
static inline void telem_update_hub(void) {}
#endif

static bool protocol_has_telemetry(void)
{
#ifdef MULTI_TELEMETRY
    return true;
#endif

    switch (protocol) {
    case MODE_FRSKYD:
    case MODE_BAYANG:
    case MODE_HUBSAN:
    case MODE_AFHDS2A:
    case MODE_FRSKYX:
    case MODE_DSM:
	return true;
    default:
	return false;
    }
}

static void TelemetryUpdate(void)
{
	if (!tx_buff_has_space())
		return;

	if (telem_update_multi())
		return;

	telem_update_sport();

	if (telem_update_dsm())
		return;

	telem_update_afhds2a();

	if (telem_update_frsky())
		return;

	telem_update_hub();
}

void telemetry_update(void)
{
    if (protocol_has_telemetry())
        TelemetryUpdate();
}

void PPM_Telemetry_serial_init(void)
{
	#ifdef MULTI_TELEMETRY
		Mprotocol_serial_init();
		#ifndef ORANGE_TX
			#ifndef STM32_BOARD
				UCSR0B &= ~(_BV(RXEN0)|_BV(RXCIE0));//rx disable and interrupt
			#endif
		#endif
	#else
		if( (protocol==MODE_FRSKYD) || (protocol==MODE_HUBSAN) || (protocol==MODE_AFHDS2A) || (protocol==MODE_BAYANG) )
			initTXSerial( SPEED_9600 ) ;
		if(protocol==MODE_FRSKYX)
			initTXSerial( SPEED_57600 ) ;
		if(protocol==MODE_DSM)
			initTXSerial( SPEED_125K ) ;
	#endif
}

#endif // TELEMETRY
