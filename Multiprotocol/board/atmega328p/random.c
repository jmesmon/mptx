#include <mptx/random.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>

//Random variable
volatile uint32_t gWDT_entropy = 0;

void random_init(void)
{
	cli();					// Temporarily turn off interrupts, until WDT configured
	MCUSR = 0;				// Use the MCU status register to reset flags for WDR, BOR, EXTR, and POWR
	WDTCSR |= _BV(WDCE);	// WDT control register, This sets the Watchdog Change Enable (WDCE) flag, which is  needed to set the prescaler
	WDTCSR = _BV(WDIE);		// Watchdog interrupt enable (WDIE)
	sei();					// Turn interupts on
}

uint32_t random_value(void)
{
	while (!gWDT_entropy);
	return gWDT_entropy;
}

// Random interrupt service routine called every time the WDT interrupt is triggered.
// It is only enabled at startup to generate a seed.
ISR(WDT_vect)
{
	static uint8_t gWDT_buffer_position=0;
	#define gWDT_buffer_SIZE 32
	static uint8_t gWDT_buffer[gWDT_buffer_SIZE];
	gWDT_buffer[gWDT_buffer_position] = TCNT1L; // Record the Timer 1 low byte (only one needed) 
	gWDT_buffer_position++;                     // every time the WDT interrupt is triggered
	if (gWDT_buffer_position >= gWDT_buffer_SIZE)
	{
		// The following code is an implementation of Jenkin's one at a time hash
		for(uint8_t gWDT_loop_counter = 0; gWDT_loop_counter < gWDT_buffer_SIZE; ++gWDT_loop_counter)
		{
			gWDT_entropy += gWDT_buffer[gWDT_loop_counter];
			gWDT_entropy += (gWDT_entropy << 10);
			gWDT_entropy ^= (gWDT_entropy >> 6);
		}
		gWDT_entropy += (gWDT_entropy << 3);
		gWDT_entropy ^= (gWDT_entropy >> 11);
		gWDT_entropy += (gWDT_entropy << 15);
		WDTCSR = 0;	// Disable Watchdog interrupt
	}
}
