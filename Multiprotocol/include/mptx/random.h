#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void random_init(void);
uint32_t random_value(void);

#ifdef __cplusplus
}
#endif

