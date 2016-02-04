/* File:    lcd.h
   Author:  Frank Dischner
   Purpose: Contains prototypes for all LCD related routines
*/

#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>
#include <avr/pgmspace.h>

void lcd_init(void);
void lcd_set_position(uint8_t row, uint8_t column);
void lcd_putc(char c);
void lcd_puts(const char *s);
void lcd_puts_P(PGM_P s);
void lcd_clear(void);
void lcd_clear_line(uint8_t row);

#endif /* _LCD_H_ */
