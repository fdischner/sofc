#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdint.h>

typedef struct settings_t
{
    int16_t target_temp;
} settings_t;

extern settings_t settings;

void settings_load(void);
void settings_save(void);

#endif /* _SETTINGS_H_ */
