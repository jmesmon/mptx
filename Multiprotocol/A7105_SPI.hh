#pragma once
#include <stdint.h>
uint8_t A7105_Reset(void);
uint8_t A7105_ReadReg(uint8_t address);
void A7105_WriteData(uint8_t len, uint8_t channel);
void A7105_ReadData(uint8_t len);
void A7105_SetTxRxMode(uint8_t mode);
void A7105_Strobe(uint8_t address);
void A7105_Init(void);
void A7105_SetPower(void);
void A7105_WriteReg(uint8_t address, uint8_t data);
void A7105_WriteID(uint32_t ida);

/* FIXME: namespace */
#define ID_NORMAL 0x55201041
#define ID_PLUS   0xAA201041
