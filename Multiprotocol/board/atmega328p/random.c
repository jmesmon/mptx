#include <mptx/random.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

//Random variable
volatile uint32_t gWDT_entropy = 0;

void random_init(void)
{
	/* start watchdog timer with shortest timeout */
	wdt_enable(WDTO_15MS);
}

uint32_t random_value(void)
{
	while (!gWDT_entropy);
	return gWDT_entropy;
}

#define gWDT_buffer_SIZE 32
static uint8_t gWDT_buffer_position=0;

// Random interrupt service routine called every time the WDT interrupt is triggered.
// It is only enabled at startup to generate a seed.
ISR(WDT_vect)
{
	uint8_t data = TCNT1L;
	// The following code is an implementation of Jenkin's one at a time hash
	gWDT_entropy += data;
	gWDT_entropy += (gWDT_entropy << 10);
	gWDT_entropy ^= (gWDT_entropy >> 6);
	gWDT_buffer_position++;
	if (gWDT_buffer_position >= gWDT_buffer_SIZE)
	{
		gWDT_entropy += (gWDT_entropy << 3);
		gWDT_entropy ^= (gWDT_entropy >> 11);
		gWDT_entropy += (gWDT_entropy << 15);
		WDTCSR = 0;
	}
}
