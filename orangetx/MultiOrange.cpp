#define ARDUINO_AVR_PRO                1

//#define __AVR_ATmega328P__	1

#define ORANGE_TX	1

// For BLUE module use:
//#define DSM_BLUE

#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

static void protocol_init(void) ;
static void update_channels_aux(void) ;
static uint32_t random_id(uint16_t adress, uint8_t create_new) ;
static void update_serial_data(void) ;
static void Mprotocol_serial_init(void) ;
static void update_led_status(void) ;
static void set_rx_tx_addr(uint32_t id) ;
uint16_t limit_channel_100(uint8_t ch) ;
void initTXSerial( uint8_t speed);
void Serial_write(uint8_t data);

extern void NRF24L01_Reset(void ) ;
extern void A7105_Reset(void ) ;
extern void CC2500_Reset(void ) ;
extern uint8_t CYRF_Reset(void ) ;
extern void CYRF_SetTxRxMode(uint8_t mode) ;

extern void frskyUpdate(void) ;
extern uint16_t initDsm2(void) ;
extern uint16_t ReadDsm2(void) ;
extern uint16_t DevoInit(void) ;
extern uint16_t devo_callback(void) ;
extern uint16_t initJ6Pro(void) ;
extern uint16_t ReadJ6Pro(void) ;
extern uint16_t WK_setup(void) ;
extern uint16_t WK_cb(void) ;

extern void randomSeed(unsigned int seed) ;
extern long random(long howbig) ;
extern long map(long x, long in_min, long in_max, long out_min, long out_max) ;

extern uint32_t millis(void) ;
extern uint32_t micros(void) ;
extern void delayMicroseconds(uint16_t x) ;
extern void delayMilliseconds(unsigned long ms) ;
extern void init(void) ;

extern void modules_reset() ;
extern uint8_t Update_All() ;
extern void tx_pause() ;
extern void tx_resume() ;
extern void TelemetryUpdate() ;
extern uint16_t initDsm() ;
extern uint16_t ReadDsm() ;

#define yield()

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


void init()
{
	// this needs to be called before setup() or some functions won't
	// work there

	// Enable external oscillator (16MHz)
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_256CLK_gc ;
	OSC.CTRL |= OSC_XOSCEN_bm ;
	while( ( OSC.STATUS & OSC_XOSCRDY_bm ) == 0 )
		/* wait */ ;
	// Enable PLL (*2 = 32MHz)
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 2 ;
	OSC.CTRL |= OSC_PLLEN_bm ;
	while( ( OSC.STATUS & OSC_PLLRDY_bm ) == 0 )
		/* wait */ ;
	// Switch to PLL clock
	CPU_CCP = 0xD8 ;
	CLK.CTRL = CLK_SCLKSEL_RC2M_gc ;
	CPU_CCP = 0xD8 ;
	CLK.CTRL = CLK_SCLKSEL_PLL_gc ;

	PMIC.CTRL = 7 ;		// Enable all interrupt levels
	sei();
	
#if defined(ADCSRA)
	// set a2d prescale factor to 128
	// 16 MHz / 128 = 125 KHz, inside the desired 50-200 KHz range.
	// XXX: this will not work properly for other clock speeds, and
	// this code should use F_CPU to determine the prescale factor.
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	sbi(ADCSRA, ADPS0);

	// enable a2d conversions
	sbi(ADCSRA, ADEN);
#endif

	// the bootloader connects pins 0 and 1 to the USART; disconnect them
	// here so they can be used as normal digital i/o; they will be
	// reconnected in Serial.begin()
#if defined(UCSRB)
	UCSRB = 0;
#elif defined(UCSR0B)
	UCSR0B = 0;
#endif

// Dip Switch inputs
	PORTA.DIRCLR = 0xFF ;
	PORTA.PIN0CTRL = 0x18 ;
	PORTA.PIN1CTRL = 0x18 ;
	PORTA.PIN2CTRL = 0x18 ;
	PORTA.PIN3CTRL = 0x18 ;
	PORTA.PIN4CTRL = 0x18 ;
	PORTA.PIN5CTRL = 0x18 ;
	PORTA.PIN6CTRL = 0x18 ;
	PORTA.PIN7CTRL = 0x18 ;
}

#include "Multiprotocol.ino"
#include "SPI.ino"
#include "Common.ino"
#include "Arduino.ino"

#include "CYRF6936_SPI.ino"
#include "DSM_cyrf6936.ino"
#include "Devo_cyrf6936.ino"
#include "J6Pro_cyrf6936.ino"
#include "WK2x01_cyrf6936.ino"

#include "Telemetry.ino"


int main(void)
{
	init() ;
	setup() ;
	for(;;)
	{
		loop() ;
	}
}
