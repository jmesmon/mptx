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
#include "Arduino.hh"
#include "Common.hh"

int16_t convert_channel(uint8_t num, int16_t min, int16_t max)
{
	return map(limit_channel_100(num), servo_min_100, servo_max_100, min, max);
}

/************************/
/**  Convert routines  **/
/************************/
// Channel value is converted to 8bit values full scale
uint8_t convert_channel_8b(uint8_t num)
{
	return convert_channel(num, 0, 255);
}

// Channel value is converted to 8bit values to provided values scale
uint8_t convert_channel_8b_scale(uint8_t num,uint8_t min,uint8_t max)
{
	return convert_channel(num, min, max);
}

// Channel value is converted sign + magnitude 8bit values
uint8_t convert_channel_s8b(uint8_t num)
{
	uint8_t ch;
	ch = convert_channel_8b(num);
	return (ch < 128 ? 127-ch : ch);
}

// Channel value is converted to 10bit values
uint16_t convert_channel_10b(uint8_t num)
{
	return convert_channel(num, 1, 1023);
}

// Channel value is multiplied by 1.5
uint16_t convert_channel_frsky(uint8_t num)
{
	return Servo_data[num] + Servo_data[num]/2;
}

// Channel value is converted for HK310
void convert_channel_HK310(uint8_t num, uint8_t *low, uint8_t *high)
{
	uint16_t temp=0xFFFF-(4*Servo_data[num])/3;
	*low=(uint8_t)(temp&0xFF);
	*high=(uint8_t)(temp>>8);
}

// Channel value is limited to PPM_100
uint16_t limit_channel_100(uint8_t ch)
{
	if(Servo_data[ch]>servo_max_100)
		return servo_max_100;
	else
		if (Servo_data[ch]<servo_min_100)
			return servo_min_100;
	return Servo_data[ch];
}

/******************************/
/**  FrSky D and X routines  **/
/******************************/
void Frsky_init_hop(void)
{
	uint8_t channel = rx_tx_addr[0]&0x07;
	uint8_t channel_spacing = (rx_tx_addr[1]&0x7F)+64;
	for(uint8_t i=0;i<50;i++)
	{
		hopping_frequency[i]=i>47?0:channel;
		channel=(channel+channel_spacing) % 0xEB;
		if((channel==0x00) || (channel==0x5A) || (channel==0xDC))
			channel++;
	}
}
