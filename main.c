#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "display.h"
#include "lcd.h"
#include "temp_control.h"
#include "fix_point.h"
#include "tick.h"
#include "rpc.h"


FUSES = 
{
    .low = FUSE_CKSEL3,
    .high = FUSE_BOOTSZ0 & FUSE_BOOTSZ1 & FUSE_SPIEN,
    .extended = EFUSE_DEFAULT
};

int main(void)
{
    /* enable all pullups to prevent floating inputs */
    PORTA = 0xFF;
    PORTB = 0xFF;
    PORTC = 0xFF;
    PORTD = 0xFF;

    /* enable interrupts */
    sei();

    tick_init();
    display_init();
    temp_control_init();
    rpc_init();

    if (temp_control_get_num_sensors() == 0)
    {
        lcd_clear();
        lcd_set_position(0, 0);
        lcd_puts_P(PSTR("No Sensors Found"));
        while (true);
    }

    temp_control_set_target_sensor(0);
    temp_control_set_target_temp(FLOAT_TO_FIX(21));
    temp_control_set_running(true);

    /* main loop */
    while (1)
    {
        bool new_temps;

        new_temps = temp_control_update();

        if (new_temps)
            display_update();

        rpc_process_message();
    }
}
