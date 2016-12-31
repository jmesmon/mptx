#pragma once
#include <stdint.h>
void initTXSerial(uint8_t speed);
void TelemetryUpdate(void);
void init_hub_telemetry(void);
void frsky_check_telemetry(uint8_t *pkt,uint8_t len);
void telemetry_reset(void);
void PPM_Telemetry_serial_init(void);

#if defined SPORT_TELEMETRY && defined FRSKYX_CC2500_INO
extern uint8_t seq_last_sent;
extern uint8_t seq_last_rcvd;
#endif

#ifdef INVERT_TELEMETRY
# if ! defined(ORANGE_TX) && ! defined(STM32_BOARD)
// enable bit bash for serial
#  define BASH_SERIAL 1
# endif
# define INVERT_SERIAL 1
#endif

static inline void tx_pause(void)
{
    // Pause telemetry by disabling transmitter interrupt
#ifdef ORANGE_TX
    USARTC0.CTRLA &= ~0x03 ;
#else
# ifndef BASH_SERIAL
#  ifdef STM32_BOARD
    USART3_BASE->CR1 &= ~ USART_CR1_TXEIE;
#  else
    UCSR0B &= ~_BV(UDRIE0);
#  endif
# endif
#endif
}

static inline void tx_resume(void)
{
    // Resume telemetry by enabling transmitter interrupt
    if(!IS_TX_PAUSE_on)
    {
#ifdef ORANGE_TX
	cli() ;
	USARTC0.CTRLA = (USARTC0.CTRLA & 0xFC) | 0x01 ;
	sei() ;
#else
# ifndef BASH_SERIAL
#  ifdef STM32_BOARD
	USART3_BASE->CR1 |= USART_CR1_TXEIE;
#  else
	UCSR0B |= _BV(UDRIE0);			
#  endif
# else
	resumeBashSerial();
# endif
#endif
    }
}

extern uint8_t telemetry_counter;
extern uint8_t v_lipo1, v_lipo2;
extern uint8_t telemetry_link;
extern uint8_t telemetry_lost;
extern int16_t RSSI_dBm;
extern uint8_t TX_RSSI;
extern uint8_t telemetry_link;
