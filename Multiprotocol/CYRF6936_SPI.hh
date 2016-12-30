#pragma once
#include <stdint.h>
uint8_t CYRF_Reset(void);
void CYRF_WriteRegister(uint8_t address, uint8_t data);
void CYRF_WritePreamble(uint32_t preamble);
void CYRF_ConfigRFChannel(uint8_t ch);
void CYRF_ConfigDataCode(const uint8_t *datacodes, uint8_t len);
void CYRF_GetMfgData(uint8_t data[]);
void CYRF_SetPower(uint8_t val);
void CYRF_SetTxRxMode(uint8_t mode);
uint8_t CYRF_ReadRegister(uint8_t address);
void CYRF_ReadDataPacketLen(uint8_t dpbuffer[], uint8_t length);
void CYRF_ConfigCRCSeed(uint16_t crc);
void CYRF_ConfigSOPCode(const uint8_t *sopcodes);
void CYRF_FindBestChannels(uint8_t *channels, uint8_t len, uint8_t minspace, uint8_t min, uint8_t max);
void CYRF_WriteDataPacket(const uint8_t dpbuffer[]);
void CYRF_PROGMEM_ConfigSOPCode(const uint8_t *data);
void CYRF_WriteDataPacketLen(const uint8_t dpbuffer[], uint8_t len);

/* FIXME: this does not belong */
extern const uint8_t PROGMEM DEVO_j6pro_sopcodes[][8];
