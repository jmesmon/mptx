#pragma once
#include <stdint.h>
uint16_t limit_channel_100(uint8_t ch);
uint16_t convert_channel_10b(uint8_t num);
uint8_t convert_channel_8b(uint8_t num);
uint8_t convert_channel_8b_scale(uint8_t num,uint8_t min,uint8_t max);
uint16_t convert_channel_frsky(uint8_t num);
void Frsky_init_hop(void);
void convert_channel_HK310(uint8_t num, uint8_t *low, uint8_t *high);
uint8_t convert_channel_s8b(uint8_t num);
int16_t convert_channel(uint8_t num, int16_t min, int16_t max);
