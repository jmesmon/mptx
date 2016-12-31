#pragma once
#include "Multiprotocol.h" /* IS_TX_PAUSE_on */
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

void initTXSerial(uint8_t speed);
void Serial_write(uint8_t byte);
bool tx_buff_has_space(void);
void serial_reset_buf(void);

#ifdef BASH_SERIAL
void resumeBashSerial(void);
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

