/* 
Access Dallas 1-Wire Devices with ATMEL AVRs
Author of the initial code: Peter Dannegger (danni(at)specs.de)
modified by Martin Thomas (mthomas(at)rhrk.uni-kl.de)
 9/2004 - use of delay.h, optional bus configuration at runtime
10/2009 - additional delay in ow_bit_io for recovery
 5/2010 - timing modifcations, additonal config-values and comments,
          use of atomic.h macros, internal pull-up support
 7/2010 - added method to skip recovery time after last bit transfered
          via ow_command_skip_last_recovery
*/


#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <string.h>

#include "onewire.h"


/*******************************************/
/* Hardware connection                     */
/*******************************************/

#define OW_PIN  PD5
#define OW_IN   PIND
#define OW_OUT  PORTD
#define OW_DDR  DDRD

// Recovery time (T_Rec) minimum 1usec - increase for long lines 
// 5 usecs is a value give in some Maxim AppNotes
// 30u secs seem to be reliable for longer lines
//#define OW_RECOVERY_TIME        5  /* usec */
//#define OW_RECOVERY_TIME      300 /* usec */
#define OW_RECOVERY_TIME         100 /* usec */

// Use AVR's internal pull-up resistor instead of external 4,7k resistor.
// Based on information from Sascha Schade. Experimental but worked in tests
// with one DS18B20 and one DS18S20 on a rather short bus (60cm), where both 
// sensores have been parasite-powered.
#define OW_USE_INTERNAL_PULLUP     0  /* 0=external, 1=internal */

/*******************************************/


#define OW_MATCH_ROM    0x55
#define OW_SKIP_ROM     0xCC
#define OW_SEARCH_ROM   0xF0

#define OW_SEARCH_FIRST 0xFF        // start new search
#define OW_LAST_DEVICE  0x00        // last device found

#define OW_GET_IN()   (OW_IN & (1<<OW_PIN))
#define OW_OUT_LOW()  (OW_OUT &= (~(1 << OW_PIN)))
#define OW_OUT_HIGH() (OW_OUT |= (1 << OW_PIN))
#define OW_DIR_IN()   (OW_DDR &= (~(1 << OW_PIN)))
#define OW_DIR_OUT()  (OW_DDR |= (1 << OW_PIN))

static uint8_t last_diff;
static uint8_t last_rom[OW_ROMCODE_SIZE];

uint8_t ow_input_pin_state()
{
    return OW_GET_IN();
}

bool ow_reset(void)
{
    uint8_t presence;
    
    OW_OUT_LOW();
    OW_DIR_OUT();            // pull OW-Pin low for 480us
    _delay_us(480);

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        // set Pin as input - wait for clients to pull low
        OW_DIR_IN(); // input
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_HIGH();
#endif
    
        _delay_us(70);       // was 66
        presence = OW_GET_IN();   // no presence detect
                             // if err!=0: nobody pulled to low, still high
    }
    
    // after a delay the clients should release the line
    // and input-pin gets back to high by pull-up-resistor
    _delay_us(480 - 70);       // was 480-66
    if(OW_GET_IN() == 0)
    {
        return false;          // short circuit, expected high but got low
    }
    
    return (presence == 0);
}


uint8_t ow_write_bit(uint8_t bit)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_LOW();
#endif
        OW_DIR_OUT();    // drive bus low
        _delay_us(2);    // T_INT > 1usec accoding to timing-diagramm
        if (bit & 0x1)
        {
            OW_DIR_IN(); // to write "1" release bus, resistor pulls high
#if OW_USE_INTERNAL_PULLUP
            OW_OUT_HIGH();
#endif
        }

        // "Output data from the DS18B20 is valid for 15usec after the falling
        // edge that initiated the read time slot. Therefore, the master must 
        // release the bus and then sample the bus state within 15ussec from 
        // the start of the slot."
        _delay_us(15-2);
        
        if (OW_GET_IN() == 0)
        {
            bit = 0;  // sample at end of read-timeslot
        }
    
        _delay_us(60-15);
#if OW_USE_INTERNAL_PULLUP
        OW_OUT_HIGH();
#endif
        OW_DIR_IN();
    
    } /* ATOMIC_BLOCK */

    _delay_us(OW_RECOVERY_TIME); // may be increased for longer wires

    return bit;
}


uint8_t ow_write_byte(uint8_t byte)
{
    uint8_t i = 8;
    
    for (i = 0; i < 8; i++)
    {
        uint8_t bit;

        bit = byte & 0x1;
        byte >>= 1;
        if (ow_write_bit(bit))
            byte |= 0x80;
    }

    return byte;
}

void ow_reset_search(void)
{
    last_diff = OW_SEARCH_FIRST;
    memset(last_rom, 0, OW_ROMCODE_SIZE);
}

bool ow_search_rom(uint8_t *id)
{
    uint8_t i, next_diff, bit_num = 1;
    
    if (last_diff == OW_LAST_DEVICE || !ow_reset())
        return false;         // error, no device found <--- early exit!
    
    ow_write_byte(OW_SEARCH_ROM);        // ROM search command
    next_diff = OW_LAST_DEVICE;         // unchanged on last device
    
    for (i = 0; i < OW_ROMCODE_SIZE; i++)
    {
        uint8_t byte, j;

        byte = last_rom[i];

        for (j = 0; j < 8; j++)
        {
            uint8_t bit, comp;

            bit = ow_read_bit();
            comp = ow_read_bit();

            if (bit == comp)
            {
                if (bit == 1)
                    return false;

                if (bit_num < last_diff)
                    bit = byte & 0x1;
                else if (bit_num == last_diff)
                    bit = 1;
                else
                    bit = 0;

                if (bit == 0)
                    next_diff = bit_num;
            }

            ow_write_bit(bit);
            byte >>= 1;
            if (bit)
                byte |= 0x80;

            bit_num++;
        }

        id[i] = byte;                           // next byte
    }

    last_diff = next_diff;
    memcpy(last_rom, id, OW_ROMCODE_SIZE);

    return true;
}

void ow_command(uint8_t command, const uint8_t *id)
{
    if (!ow_reset())
        return;

    if (id != NULL)
    {
        uint8_t i;

        ow_write_byte(OW_MATCH_ROM);     // to a single device

        for (i = 0; i < OW_ROMCODE_SIZE; i++)
            ow_write_byte(*id++);
    } 
    else
    {
        ow_write_byte(OW_SKIP_ROM);      // to all devices
    }
    
    ow_write_byte(command);
}

uint8_t ow_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    uint16_t i;

    for (i = 0; i < len; i++)
        crc = _crc_ibutton_update(crc, *data++);

    return crc;
}
