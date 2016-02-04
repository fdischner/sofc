#ifndef DS18X20_H_
#define DS18X20_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/* return values */
#define DS18X20_OK                0x00
#define DS18X20_ERROR             0x01
#define DS18X20_START_FAIL        0x02
#define DS18X20_ERROR_CRC         0x03

/* DS18X20 specific values (see datasheet) */
#define DS18B20_CONF_REG          4
#define DS18B20_9_BIT             0
#define DS18B20_10_BIT            (1<<5)
#define DS18B20_11_BIT            (1<<6)
#define DS18B20_12_BIT            ((1<<6)|(1<<5))
#define DS18B20_RES_MASK          ((1<<6)|(1<<5))

// conversion times in milliseconds
#define DS18B20_TCONV_12BIT       750
#define DS18B20_TCONV_11BIT       DS18B20_TCONV_12BIT/2
#define DS18B20_TCONV_10BIT       DS18B20_TCONV_12BIT/4
#define DS18B20_TCONV_9BIT        DS18B20_TCONV_12BIT/8
#define DS18S20_TCONV             DS18B20_TCONV_12BIT

// scratchpad size in bytes
#define DS18X20_SP_SIZE           9

// DS18X20 EEPROM-Support
#define DS18X20_TH_REG            2
#define DS18X20_TL_REG            3


bool DS18X20_find_sensor(uint8_t *id);
bool DS18X20_parasite_powered(const uint8_t *id);
uint8_t DS18X20_start_meas(const uint8_t *id);
// not available when parasite powered
bool DS18X20_conversion_in_progress(void);

uint8_t DS18X20_read_fixed_point(const uint8_t *id, int16_t *val);

// write th, tl and config-register to scratchpad (config ignored on DS18S20)
uint8_t DS18X20_write_scratchpad(const uint8_t *id, uint8_t th,
        uint8_t tl, uint8_t conf);
// read scratchpad into array SP
uint8_t DS18X20_read_scratchpad(const uint8_t *id, uint8_t *sp);

// copy values in scratchpad into DS18x20 eeprom
uint8_t DS18X20_scratchpad_to_eeprom(const uint8_t *id);
// copy values from DS18x20 eeprom into scratchpad
uint8_t DS18X20_eeprom_to_scratchpad(const uint8_t *id);


#endif
