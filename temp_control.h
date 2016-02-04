#ifndef _TEMP_CONTROL_H_
#define _TEMP_CONTROL_H_

#include <stdint.h>
#include <stdbool.h>
#include "onewire.h"

#define MAX_TEMP_SENSORS 10

struct temp_sensor
{   
    uint8_t id[OW_ROMCODE_SIZE];
    char name[11];
    int16_t temp;
    int16_t min;
    int16_t max;
};

typedef enum temp_control_state_t
{
    STOPPED,
    COOLING,
    HEATING,
    IDLE,
} temp_control_state_t;

void temp_control_init(void);
bool temp_control_update(void);
void temp_control_set_target_temp(int16_t temp);
int16_t temp_control_get_target_temp(void);
void temp_control_set_target_sensor(uint8_t sensor);
void temp_control_set_running(bool running);
temp_control_state_t temp_control_get_state(void);
uint8_t temp_control_get_num_sensors(void);
struct temp_sensor * temp_control_get_sensor_data(uint8_t sensor);

#endif /* _TEMP_CONTROL_H_ */
