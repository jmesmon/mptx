#include <mptx/random.h>
#include <stdint.h>
#include <Arduino.h>

void random_init(void) {}

uint32_t random_value(void)
{
	return (uint32_t)analogRead(PB0) << 10 | analogRead(PB1);
}
