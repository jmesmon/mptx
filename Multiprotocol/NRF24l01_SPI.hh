#pragma once
#include <stdint.h>

void NRF24L01_Reset(void);
void NRF24L01_Initialize(void);
void NRF24L01_WriteReg(uint8_t reg, uint8_t data);
void NRF24L01_FlushTx(void);
void NRF24L01_WriteRegisterMulti(uint8_t reg, uint8_t * data, uint8_t length);
void NRF24L01_FlushRx(void);
void NRF24L01_SetPower(void);
void NRF24L01_WritePayload(uint8_t * data, uint8_t length);
void NRF24L01_SetBitrate(uint8_t bitrate);
void NRF24L01_SetTxRxMode(enum TXRX_State mode);
void NRF24L01_ReadPayload(uint8_t * data, uint8_t length);
uint8_t NRF24L01_ReadReg(uint8_t reg);
void NRF24L01_Activate(uint8_t code);
uint8_t NRF24L01_packet_ack(void);

void XN297_Configure(uint8_t flags);
void XN297_WritePayload(uint8_t* msg, uint8_t len);
void XN297_SetTXAddr(const uint8_t* addr, uint8_t len);
void XN297_ReadPayload(uint8_t* msg, uint8_t len);
void XN297_SetRXAddr(const uint8_t* addr, uint8_t len);
void XN297_SetScrambledMode(const uint8_t mode);

uint8_t LT8900_ReadPayload(uint8_t* msg, uint8_t len);
void LT8900_SetChannel(uint8_t channel);
void LT8900_SetAddress(uint8_t *address,uint8_t addr_size);
void LT8900_SetTxRxMode(enum TXRX_State mode);
void LT8900_WritePayload(uint8_t* msg, uint8_t len);
#define LT8900_CRC_ON 6
#define LT8900_SCRAMBLE_ON 5
#define LT8900_PACKET_LENGTH_EN 4
#define LT8900_DATA_PACKET_TYPE_1 3
#define LT8900_DATA_PACKET_TYPE_0 2
#define LT8900_FEC_TYPE_1 1
#define LT8900_FEC_TYPE_0 0
void LT8900_Config(uint8_t preamble_len, uint8_t trailer_len, uint8_t flags, uint8_t crc_init);

/* FIXME: avoid exposing or relocate */
uint16_t crc16_update(uint16_t crc, uint8_t a);
