#include <stdlib.h>
#include "temp_control.h"
#include "fix_point.h"
#include "tick.h"
#include "lcd.h"

// switch lcd every 10 seconds
//#define LCD_SWITCH_INTERVAL (10000 / TICK_MS)
#define LCD_SWITCH_INTERVAL (5000 / TICK_MS)

static void fix_to_str(char *buf, int16_t val)
{
    div_t d;
    bool negative = false;

    // convert to decimal
    val = FIX_TO_INT((int32_t) val * 10);

    // check for valid values
    if (val < -99 || val > 999)
    {
        buf[0] = ' ';
        buf[1] = 'E';
        buf[2] = 'R';
        buf[3] = 'R';
        buf[4] = '\0';

        return;
    }

    if (val < 0)
    {
        negative = true;
        val = -val;
    }

    buf[0] = ' ';
    buf[2] = '.';
    buf[4] = '\0';

    d = div(val, 10);

    buf[3] = '0' + d.rem;

    d = div(d.quot, 10);
    buf[1] = '0' + d.rem;

    if (d.quot > 0)
        buf[0] = '0' + d.quot;

    if (negative)
        buf[0] = '-';
}

void display_init(void)
{
    lcd_init();
    lcd_clear();
    lcd_puts_P(PSTR("Initializing..."));
}

void display_update(void)
{
    static uint32_t last_switch_tick = 0;
    static uint8_t sensor_num = 0;
    struct temp_sensor *sensor;
    char buf[5];

    if (tick_get() - last_switch_tick >= LCD_SWITCH_INTERVAL)
    {
        last_switch_tick += LCD_SWITCH_INTERVAL;

        if (++sensor_num >= temp_control_get_num_sensors())
            sensor_num = 0;
    }

    sensor = temp_control_get_sensor_data(sensor_num);

    lcd_clear();

    if (sensor == NULL)
    {
        lcd_set_position(0, 0);
        lcd_puts_P(PSTR("No Sensor Data"));

        return;
    }

    lcd_set_position(0, 0);
    lcd_puts(sensor->name);
    lcd_putc(':');

    lcd_set_position(1, 1);
    lcd_puts_P(PSTR("Cur  Min  Max"));

    lcd_set_position(2, 1);
    fix_to_str(buf, sensor->temp);
    lcd_puts(buf);
    lcd_set_position(2, 6);
    fix_to_str(buf, sensor->min);
    lcd_puts(buf);
    lcd_set_position(2, 11);
    fix_to_str(buf, sensor->max);
    lcd_puts(buf);

    lcd_set_position(3, 0);
    lcd_puts_P(PSTR("SP: "));
    fix_to_str(buf, temp_control_get_target_temp());
    lcd_puts(buf);

    lcd_set_position(3, 11);
    switch (temp_control_get_state())
    {
        case STOPPED:
            lcd_puts_P(PSTR("Stop"));
            break;
        case COOLING:
            lcd_puts_P(PSTR("Cool"));
            break;
        case HEATING:
            lcd_puts_P(PSTR("Heat"));
            break;
        case IDLE:
            lcd_puts_P(PSTR("Idle"));
            break;
    }
}
