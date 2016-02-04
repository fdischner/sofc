#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdio.h>
#include "temp_control.h"
#include "fan_control.h"
#include "ds18x20.h"
#include "fix_point.h"
#include "tick.h"

static struct temp_sensor sensors[MAX_TEMP_SENSORS];
static uint8_t num_sensors;
static uint8_t target_sensor = 0;
static int16_t target_temp = INT_TO_FIX(18);
static temp_control_state_t state = STOPPED;

static void update_control_output(void)
{
    if (state == STOPPED || target_sensor >= num_sensors)
    {
        fan_control_set_on(false);
        state = STOPPED;
        return;
    }

    if (sensors[target_sensor].temp > target_temp)
    {
        fan_control_set_on(true);
        state = COOLING;
    }
    else if (sensors[target_sensor].temp < (target_temp - FLOAT_TO_FIX(0.2)))
    {
        fan_control_set_on(false);
        state = IDLE;
    }
}

void temp_control_init(void)
{
    uint8_t i;

    fan_control_init();
    temp_control_set_running(false);
    update_control_output();

    ow_reset_search();

    for (i = 0; i < MAX_TEMP_SENSORS; i++)
    {
        if (!DS18X20_find_sensor(sensors[i].id))
            break;

        sensors[i].temp = 0;
        sensors[i].min = INT16_MAX;
        sensors[i].max = INT16_MIN;

        switch (i)
        {
            case 0:
                strcpy_P(sensors[i].name, PSTR("Beer"));
                break;
            case 1:
                strcpy_P(sensors[i].name, PSTR("Chamber"));
                break;
            case 2:
                strcpy_P(sensors[i].name, PSTR("Ice"));
                break;
            case 3:
                strcpy_P(sensors[i].name, PSTR("Ambient"));
                break;
            default:
                snprintf_P(sensors[i].name, 11, PSTR("Sensor %i"), i + 1);
                break;
        }
    }

    num_sensors = i;

    if (num_sensors == 0)
        return;

    // start first conversion
    DS18X20_start_meas(NULL);
}

bool temp_control_update(void)
{
    static bool converting = true;
    static uint32_t conversion_time = 0;

    uint8_t i;

    //if (num_sensors == 0 || DS18X20_conversion_in_progress())
    if (num_sensors == 0)
        return false;

    if (converting)
    {
        if ((tick_get() - conversion_time) < (1000 / TICK_MS))
            return false;
        
        if (DS18X20_conversion_in_progress())
            return false;

        converting = false;
    }
    else
    {
        if ((tick_get() - conversion_time) >= (5000 / TICK_MS))
        {
            // start next conversion
            DS18X20_start_meas(NULL);
            conversion_time = tick_get();
            converting = true;
        }

        return false;
    }

    for (i = 0; i < num_sensors; i++)
    {
        DS18X20_read_fixed_point(sensors[i].id, &(sensors[i].temp));

        if (sensors[i].temp < sensors[i].min)
            sensors[i].min = sensors[i].temp;

        if (sensors[i].temp > sensors[i].max)
            sensors[i].max = sensors[i].temp;
    }

    update_control_output();

    return true;
}

void temp_control_set_target_temp(int16_t temp)
{
    target_temp = temp;
    update_control_output();
}

int16_t temp_control_get_target_temp(void)
{
    return target_temp;
}

void temp_control_set_target_sensor(uint8_t sensor)
{
    if (sensor < num_sensors)
    {
        target_sensor = sensor;
        update_control_output();
    }
}

void temp_control_set_running(bool running)
{
    if (running)
    {
        if (state == STOPPED)
            state = IDLE;
    }
    else
    {
        state = STOPPED;
    }

    update_control_output();
}

temp_control_state_t temp_control_get_state(void)
{
    return state;
}

uint8_t temp_control_get_num_sensors(void)
{
    return num_sensors;
}

struct temp_sensor * temp_control_get_sensor_data(uint8_t sensor)
{
    if (sensor < num_sensors)
        return &sensors[sensor];

    return NULL;
}

