#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/cpufunc.h>
#include <avr/pgmspace.h>
#include <avr/io.h>

#define LCD_DATA_PORT PORTA
#define LCD_DATA_DDR  DDRA
#define LCD_DATA_PINS PINA
#define LCD_CTRL_PORT PORTC
#define LCD_CTRL_DDR  DDRC
#define LCD_ENABLE    _BV(PC3)
#define LCD_RW        _BV(PC4)
#define LCD_RS        _BV(PC5)

/* LCD_RS values */
#define INSTR 0
#define DATA  1

#define LCD_BUSY_MASK 0x80

static uint8_t cur_addr;


static uint8_t lcd_read(uint8_t rs)
{
    uint8_t data;

    /* set data port as input */
    LCD_DATA_DDR = 0x00;
    /* pullups off */
    LCD_DATA_PORT = 0x00;

    /* select data/instruction */
    if (rs)
        LCD_CTRL_PORT |= LCD_RS;
    else
        LCD_CTRL_PORT &= ~LCD_RS;

    /* set read command */
    LCD_CTRL_PORT |= LCD_RW;

    /* address set-up time */
    _delay_us(0.04);

    /* do read */
    LCD_CTRL_PORT |= LCD_ENABLE;
    _delay_us(0.50);
    data = LCD_DATA_PINS;
    LCD_CTRL_PORT &= ~LCD_ENABLE;
    _delay_us(0.50);

    return data;
}

static void lcd_wait_busy(void)
{
    while (lcd_read(INSTR) & LCD_BUSY_MASK);
}

static void lcd_write(uint8_t data, uint8_t rs, bool wait_busy)
{
    if (wait_busy)
        lcd_wait_busy();

    /* select data/instruction */
    if (rs)
        LCD_CTRL_PORT |= LCD_RS;
    else
        LCD_CTRL_PORT &= ~LCD_RS;

    /* set write command */
    LCD_CTRL_PORT &= ~LCD_RW;

    /* set data port as output */
    LCD_DATA_DDR = 0xFF;
    /* write data to port */
    LCD_DATA_PORT = data;

    /* address set-up time */
    _delay_us(0.04);

    /* do write */
    LCD_CTRL_PORT |= LCD_ENABLE;
    _delay_us(0.50);
    LCD_CTRL_PORT &= ~LCD_ENABLE;
    _delay_us(0.50);
}

static void lcd_set_address(uint8_t addr)
{
    cur_addr = addr;
    lcd_write(0x80 | addr, INSTR, true);
}

// initialize the LCD
void lcd_init(void)
{
    /* set control pins as outputs */
    LCD_CTRL_DDR |= LCD_ENABLE;
    LCD_CTRL_DDR |= LCD_RW;
    LCD_CTRL_DDR |= LCD_RS;
    /* enable pin low */
    LCD_CTRL_PORT &= ~LCD_ENABLE;

    // initialization
    _delay_ms(17);
    lcd_write(0x38, INSTR, false);
    // no, really
    _delay_ms(5);
    lcd_write(0x38, INSTR, false);
    // seriously, I mean it this time
    _delay_us(120);
    lcd_write(0x38, INSTR, false);

    // the data sheet is not clear on whether the busy flag
    // can be checked yet, so we use a delay to be safe
    _delay_us(120);
    // 8-bit two lines
    lcd_write(0x38, INSTR, false);
    _delay_us(120);

    // display on, cursor off, cursor blink off
    lcd_write(0x0C, INSTR, true);
    // clear display
    lcd_write(0x01, INSTR, true);
    // auto increment on, shift off
    lcd_write(0x06, INSTR, true);

    cur_addr = 0;
}

void lcd_set_position(uint8_t row, uint8_t column)
{
    uint8_t addr;

    // make sure location is valid
    if (row > 3 || column > 15)
        return;

    addr = column;

    if (row == 1)
        addr += 64;
    else if (row == 2)
        addr += 16;
    else if (row == 3)
        addr += 80;

    lcd_set_address(addr);
}

void lcd_putc(char c)
{
    lcd_write((uint8_t) c, DATA, true);

    cur_addr++;

    // if we're at the end of a line
    // move to the next one
    if (cur_addr == 16)
        lcd_set_address(64);
    else if (cur_addr == 80)
        lcd_set_address(16);
    else if (cur_addr == 32)
        lcd_set_address(80);
    else if (cur_addr == 96)
        lcd_set_address(0);
}

void lcd_puts(const char *s)
{
    // write all characters
    while (*s)
        lcd_putc(*s++);
}

void lcd_puts_P(PGM_P s)
{
    char c;

    // write all characters
    while ((c = pgm_read_byte(s++)))
        lcd_putc(c);
}

void lcd_clear(void)
{
    lcd_write(0x01, INSTR, true);
}

void lcd_clear_line(uint8_t row)
{
    uint8_t i;

    lcd_set_position(row, 0);
    for (i = 0; i < 16; i++)
        lcd_putc(' ');
}
