/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <avr/pgmspace.h>

#ifdef IS_644
#define UCSRB UCSR0B
#define RXEN  RXEN0
#define TXEN  TXEN0
#define EEWE  EEPE
#define EEMWE EEMPE
#endif

#define LED_PORT	PORTD
#define LED_DDR		DDRD
#define LED_PIN		PIND

#define RED_LED         3  // (output) Red LED (high true)
#define GREEN_LED       4  // (output) Green LED (high true)

// Control Light
#define RED_LED_OFF      LED_PORT &= ~_BV(RED_LED)
#define RED_LED_ON     LED_PORT |= _BV(RED_LED)
#define GREEN_LED_OFF    LED_PORT &= ~_BV(GREEN_LED)
#define GREEN_LED_ON   LED_PORT |= _BV(GREEN_LED)

#define __STDIO_FDEVOPEN_COMPAT_12 //make code back compatible
#define BAUDRATE 57600UL //this gets doubled on parts with the bootloader

#define FALSE 0
#define TRUE 1

#define UNIQUEID_ADDRESS 0x0 // two bytes
#define DEBUG_LOG_ADDRESS 0x20 // array of size (see comm.h), 16 bits per item

void generateUniqueID(void);
uint16_t getUniqueID(void);

#define NOP asm("nop")

void halt(void);

void initializeLEDs(void);
void blinkGreen(int times);
void blinkRed(int times);
void blinkOrange(int times);


int uart_putchar(char c);
int uart_getchar(void);

#define putstring(x) const_putstring(PSTR(x))
void const_putstring(const char *str);

void initializeUART(void);
void enableUART(void);
void disableUART(void);

void step(void);
void delay_s(uint8_t s);
void delay_ms(uint8_t ms);
inline volatile void delay_us(uint8_t us);

uint8_t internal_eeprom_read8(uint16_t addr);
void internal_eeprom_write8(uint16_t addr, uint8_t data);
uint16_t internal_eeprom_read16(uint16_t addr);
void internal_eeprom_write16(uint16_t addr, uint16_t data);
