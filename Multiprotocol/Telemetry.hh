#pragma once
#include <stdint.h>

#ifdef TELEMETRY
void telemetry_update(void);
void init_hub_telemetry(void);
void frsky_check_telemetry(uint8_t *pkt,uint8_t len);
void telemetry_reset(void);
void PPM_Telemetry_serial_init(void);
#else
static inline void telemetry_update(void) {}
#endif

#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
extern uint8_t seq_last_sent;
extern uint8_t seq_last_rcvd;
#endif

extern uint8_t telemetry_counter;
extern uint8_t v_lipo1, v_lipo2;
extern uint8_t telemetry_link;
extern uint8_t telemetry_lost;
extern int16_t RSSI_dBm;
extern uint8_t TX_RSSI;
extern uint8_t telemetry_link;

#include "serial.hh"
