#ifndef _TICK_H_
#define _TICK_H_

#include <stdint.h>

#define TICK_MS 10

void tick_init(void);
uint32_t tick_get(void);

#endif /* _TICK_H_ */
