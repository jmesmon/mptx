#pragma once
#include <stdint.h>

uint8_t CC2500_Reset(void);
void CC2500_WriteReg(uint8_t address, uint8_t data);
void CC2500_SetPower();
void CC2500_SetTxRxMode(uint8_t mode);
uint8_t CC2500_ReadReg(uint8_t address);
void CC2500_ReadData(uint8_t *dpbuffer, uint8_t len);
void CC2500_Strobe(uint8_t state);
void CC2500_WriteData(uint8_t *dpbuffer, uint8_t len);
