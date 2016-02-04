#include "tick.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

#define CLOCKS_PER_TICK (((F_CPU * TICK_MS) + 500) / 1000)

#if CLOCKS_PER_TICK > 65536UL
#undef CLOCKS_PER_TICK
#define PRESCALE_8
#define CLOCKS_PER_TICK (((F_CPU * TICK_MS) + (1000 * 8 / 2)) / (1000 * 8))
#endif

static uint32_t ticks = 0;

ISR(TIMER3_COMPA_vect)
{
    ticks++;
}

void tick_init(void)
{
    // CTC mode, no output, TOP = OCR3A
    TCCR3A = 0;
    TCCR3B = _BV(WGM32);
    OCR3A = CLOCKS_PER_TICK - 1;
    TCNT3 = 0;
    // enable interrupt
    TIMSK3 = _BV(OCIE3A);
#ifdef PRESCALE_8
    // start timer with div 8 prescaler
    TCCR3B |= _BV(CS31);
#else
    // start timer with no prescaler
    TCCR3B |= _BV(CS30);
#endif
}

uint32_t tick_get(void)
{
    uint32_t val;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        val = ticks;
    }

    return val;
}
