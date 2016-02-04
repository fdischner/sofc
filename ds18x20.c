/*********************************************************************************
Title:    DS18X20-Functions via One-Wire-Bus
Author:   Martin Thomas <eversmith@heizung-thomas.de>   
          http://www.siwawi.arubi.uni-kl.de/avr-projects
Software: avr-gcc 4.3.3 / avr-libc 1.6.7 (WinAVR 3/2010) 
Hardware: any AVR - tested with ATmega16/ATmega32/ATmega324P and 3 DS18B20

Partly based on code from Peter Dannegger and others.

changelog:
20041124 - Extended measurements for DS18(S)20 contributed by Carsten Foss (CFO)
200502xx - function DS18X20_read_meas_single
20050310 - DS18x20 EEPROM functions (can be disabled to save flash-memory)
           (DS18X20_EEPROMSUPPORT in ds18x20.h)
20100625 - removed inner returns, added static function for read scratchpad
         . replaced full-celcius and fractbit method with decicelsius
           and maxres (degreeCelsius*10e-4) functions, renamed eeprom-functions,
           delay in recall_e2 replaced by timeout-handling
20100714 - ow_command_skip_last_recovery used for parasite-powerd devices so the
           strong pull-up can be enabled in time even with longer OW recovery times
20110209 - fix in DS18X20_format_from_maxres() by Marian Kulesza
**********************************************************************************/

#include <stdlib.h>
#include <stdint.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ds18x20.h"
#include "onewire.h"
#include "fix_point.h"

// for 10ms delay in copy scratchpad
#include <util/delay.h>


/* DS18X20 specific values (see datasheet) */
#define DS18S20_FAMILY_CODE       0x10
#define DS18B20_FAMILY_CODE       0x28
#define DS1822_FAMILY_CODE        0x22

/* DS18X20 specific commands */
#define DS18X20_CONVERT_T         0x44
#define DS18X20_WRITE_SCRATCHPAD  0x4E
#define DS18X20_READ_SCRATCHPAD   0xBE
#define DS18X20_COPY_SCRATCHPAD   0x48
#define DS18X20_RECALL_E2         0xB8
#define DS18X20_READ_POWER_SUPPLY 0xB4

// undefined bits in LSB if 18B20 != 12bit
#define DS18B20_9_BIT_UNDF        ((1<<0)|(1<<1)|(1<<2))
#define DS18B20_10_BIT_UNDF       ((1<<0)|(1<<1))
#define DS18B20_11_BIT_UNDF       ((1<<0))
#define DS18B20_12_BIT_UNDF       0

// constant to convert the fraction bits to cel*(10^-4)
#define DS18X20_FRACCONV          625

// DS18X20 EEPROM-Support
#define DS18X20_COPYSP_DELAY      10 /* ms */



/* find DS18X20 Sensors on 1-Wire-Bus
   input/ouput: diff is the result of the last rom-search
                *diff = OW_SEARCH_FIRST for first call
   output: id is the rom-code of the sensor found */
bool DS18X20_find_sensor(uint8_t *id)
{
    // search devices on bus
    while (ow_search_rom(id))
    {
        // check whether device is a temp sensor
        if (id[0] == DS18B20_FAMILY_CODE ||
            id[0] == DS18S20_FAMILY_CODE ||
            id[0] == DS1822_FAMILY_CODE)
        {
            // check crc
            if (ow_crc8(id, OW_ROMCODE_SIZE - 1) == id[OW_ROMCODE_SIZE - 1])
                return true;
        }
    }

    return false;
}

/* get power status of DS18x20 
   input:   id = rom_code 
   returns: DS18X20_POWER_EXTERN or DS18X20_POWER_PARASITE */
bool DS18X20_parasite_powered(const uint8_t *id)
{
    if (!ow_reset())
        return false;

    ow_command(DS18X20_READ_POWER_SUPPLY, id);
    return (!ow_read_bit());
}

/* start measurement (CONVERT_T) for all sensors if input id==NULL 
   or for single sensor where id is the rom-code */
uint8_t DS18X20_start_meas(const uint8_t *id)
{
    if (!ow_reset())
        return DS18X20_START_FAIL;

    ow_command(DS18X20_CONVERT_T, id);

    return DS18X20_OK;
}

// returns 1 if conversion is in progress, 0 if finished
// not available when parasite powered.
bool DS18X20_conversion_in_progress(void)
{
    return (!ow_read_bit());
}

uint8_t DS18X20_read_fixed_point(const uint8_t *id, int16_t *val)
{
    uint8_t sp[DS18X20_SP_SIZE];
    uint8_t ret;

    ret = DS18X20_read_scratchpad(id, sp);

    if (ret == DS18X20_OK)
    {
        int16_t raw_val;
        uint8_t family_code = DS18B20_FAMILY_CODE;

        if (id != NULL)
            family_code = id[0];

        raw_val = sp[0] | (sp[1] << 8);

        if (family_code == DS18S20_FAMILY_CODE)
        {
            // 9 -> 12 bit if 18S20
            /* Extended measurements for DS18S20 contributed by Carsten Foss */
            // Discard LSB, needed for later extended precicion calc
            raw_val &= (uint16_t)0xfffe;
            // Convert to 12-bit, now degrees are in 1/16 degrees units
            raw_val <<= 3;
            // Add the compensation and remember to subtract 0.25 degree (4/16)
            raw_val += (sp[7] - sp[6]) - 4;
        }
        else if (family_code == DS18B20_FAMILY_CODE ||
                 family_code == DS1822_FAMILY_CODE )
        {
            // clear undefined bits for DS18B20 != 12bit resolution
            switch (sp[DS18B20_CONF_REG] & DS18B20_RES_MASK)
            {
            case DS18B20_9_BIT:
                raw_val &= ~(DS18B20_9_BIT_UNDF);
                break;
            case DS18B20_10_BIT:
                raw_val &= ~(DS18B20_10_BIT_UNDF);
                break;
            case DS18B20_11_BIT:
                raw_val &= ~(DS18B20_11_BIT_UNDF);
                break;
            default:
                // 12 bit - all bits valid
                break;
            }
        }

        // check for negative 
        if (raw_val & 0x8000)
        {
            // convert from twos complement to machine representation
            // should compile to nop on twos complement machines
            raw_val = -((~raw_val) + 1);
        }

        *val = INT_TO_FIX(raw_val) / 16;
    }

    return ret;
}

uint8_t DS18X20_write_scratchpad(const uint8_t *id, uint8_t th,
        uint8_t tl, uint8_t conf)
{
    if (id == NULL || !ow_reset())
        return DS18X20_ERROR;

    ow_command(DS18X20_WRITE_SCRATCHPAD, id);
    ow_write_byte(th);
    ow_write_byte(tl);

    if (id[0] == DS18B20_FAMILY_CODE || id[0] == DS1822_FAMILY_CODE)
        ow_write_byte(conf); // config only available on DS18B20 and DS1822

    return DS18X20_OK;
}

uint8_t DS18X20_read_scratchpad(const uint8_t *id, uint8_t *sp)
{
    uint8_t i;

    if (!ow_reset())
        return DS18X20_ERROR;

    ow_command(DS18X20_READ_SCRATCHPAD, id);

    for (i = 0; i < DS18X20_SP_SIZE; i++)
        sp[i] = ow_read_byte();

    if (ow_crc8(sp, DS18X20_SP_SIZE - 1) != sp[DS18X20_SP_SIZE - 1])
        return DS18X20_ERROR_CRC;

    return DS18X20_OK;
}

uint8_t DS18X20_scratchpad_to_eeprom(const uint8_t *id)
{
    if (!ow_reset())
        return DS18X20_START_FAIL;

    ow_command(DS18X20_COPY_SCRATCHPAD, id);
    _delay_ms(DS18X20_COPYSP_DELAY); // wait for 10 ms 

    return DS18X20_OK;
}

uint8_t DS18X20_eeprom_to_scratchpad(const uint8_t *id)
{
    uint8_t retry_count=255;

    if (!ow_reset())
        return DS18X20_START_FAIL;

    ow_command(DS18X20_RECALL_E2, id);
    while (retry_count-- && !ow_read_bit())
        ;

    if (retry_count == 0)
        return DS18X20_ERROR;

    return DS18X20_OK;
}

