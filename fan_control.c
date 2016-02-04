#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

#define PWM_FREQ 25000UL
#define PWM_CLOCKS ((F_CPU + (PWM_FREQ / 2)) / PWM_FREQ)

#if PWM_CLOCKS > 256
#define PRESCALE_8
#undef PWM_CLOCKS
#define PWM_CLOCKS ((F_CPU + ((PWM_FREQ * 8) / 2)) / (PWM_FREQ * 8))
#endif

#define PWM_TOP (PWM_CLOCKS - 1)
//#define PWM_HIGH (PWM_CLOCKS)
#define PWM_HIGH (PWM_CLOCKS / 2)
#define PWM_LOW (PWM_CLOCKS / 50)

#if (PWM_LOW < 1)
#undef PWM_LOW
#define PWM_LOW 1
#endif

void fan_control_init(void)
{
    // fast PWM, TOP = OCR2A, non-inverted output on OC2B
    TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(WGM22);

    // set frequency
    OCR2A = PWM_TOP;
    // initially off
    OCR2B = PWM_LOW;
    // reset counter
    TCNT2 = 0;
#ifdef PRESCALE_8
    // enable timer, div 8 prescaler
    TCCR2B |= _BV(CS21);
#else
    // enable timer, no prescaler
    TCCR2B |= _BV(CS20);
#endif
    // enable pin output
    DDRD |= _BV(DDD6);
}

void fan_control_set_on(bool on)
{
    if (on)
        OCR2B = PWM_HIGH;
    else
        OCR2B = PWM_LOW;
}
