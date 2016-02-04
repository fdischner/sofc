#include <avr/eeprom.h>
#include <util/crc16.h>
#include "settings.h"

#define SETTINGS_MAGIC      0xA5
#define SETTINGS_VERSION    1

settings_t settings;

void settings_load(void)
{
    uint8_t magic, version;
    uint16_t crc;

    magic = eeprom_read_byte(0);
    version = eeprom_read_byte(1);

    if (magic != SETTINGS_MAGIC)
    {
        // set version to zero so all default values are loaded
        version = 0;
    }
    else
    {
        // initialize crc
        crc = 0xFFFF;
        crc = _crc_ccitt_update(crc, magic);
        crc = _crc_ccitt_update(crc, version);
    }

}

void settings_save(void)
{
}
