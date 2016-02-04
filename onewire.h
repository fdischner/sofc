#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#include <stdint.h>
#include <stdbool.h>

// rom-code size including CRC
#define OW_ROMCODE_SIZE 8

bool ow_reset(void);
uint8_t ow_write_bit(uint8_t bit);
#define ow_read_bit() ow_write_bit(1)
uint8_t ow_write_byte(uint8_t byte);
#define ow_read_byte() ow_write_byte(0xFF)
void ow_reset_search(void);
bool ow_search_rom(uint8_t *id);
void ow_command(uint8_t command, const uint8_t *id);
uint8_t ow_input_pin_state(void);
uint8_t ow_crc8(const uint8_t *data, uint16_t len);

#endif
