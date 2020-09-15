/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __util_h
#define __util_h

#ifdef IS_644
#define UCSRB UCSR0B
#define RXEN  RXEN0
#define TXEN  TXEN0
#define EEWE  EEPE
#define EEMWE EEMPE
#endif

#include <avr/pgmspace.h>

//////////////////////////////////////////////////////////
///////////// LED ///////////////////////////////////////
//////////////////////////////////////////////////////////

#define LED_PORT	PORTD
#define LED_DDR		DDRD
#define LED_PIN		PIND

#define RED_LED         3  // (output) Red LED (high true)
#define GREEN_LED       4  // (output) Green LED (high true)

// Control Light
#define RED_LED_OFF     LED_PORT &= ~_BV(RED_LED)
#define RED_LED_ON      LED_PORT |= _BV(RED_LED)
#define GREEN_LED_OFF   LED_PORT &= ~_BV(GREEN_LED)
#define GREEN_LED_ON    LED_PORT |= _BV(GREEN_LED)

//////////////////////////////////////////////////////////
///////////// UART ///////////////////////////////////////
//////////////////////////////////////////////////////////
#define __STDIO_FDEVOPEN_COMPAT_12 //make code back compatible

#define BAUDRATE 57600UL
void putnum_uh(uint16_t n);
void putnum_ud(uint16_t n);
int uart_putchar(char c);
int uart_getchar(void);
void initializeUART(void);
void enableUART(void);
void disableUART(void);
void printhex(uint8_t hex);

//////////////////////////////////////////////////////////
///////////// misc ///////////////////////////////////////
//////////////////////////////////////////////////////////

#define FALSE 0
#define TRUE 1

#define NOP asm("nop")


//////////////////////////////////////////////////////////
//////////// internal eeprom addresses  /////////////
//////////////////////////////////////////////////////////

#define UNIQUEID_ADDRESS 0x0 // two bytes

//Servo Calibration data in EEPROM (addr 0x10 thru 0x1C)
#define CALIBRATION_DATA_ADDRESS  0x10
#define SERVO_MIN_ADDRESS             CALIBRATION_DATA_ADDRESS
#define SERVO_MAX_ADDRESS             (CALIBRATION_DATA_ADDRESS + 2)
#define SERVO_READ_MIN_ADDRESS        (CALIBRATION_DATA_ADDRESS + 4)
#define SERVO_READ_MAX_ADDRESS        (CALIBRATION_DATA_ADDRESS + 6)
#define SERVO_RECBUFSIZE_ADDRESS      (CALIBRATION_DATA_ADDRESS + 8)
#define SERVO_INITIALIZED_ADDRESS     (CALIBRATION_DATA_ADDRESS + 10)
//#define DATA_INITIALIZED_ADDRESS      (CALIBRATION_DATA_ADDRESS + 11)

// array of size (see comm.h), 16 bits per item
#define DEBUG_LOG_ADDRESS (CALIBRATION_DATA_ADDRESS + 20) 

// recording bank variables
#define intEEPROM_DEFAULT_BANK   0x1
#define intEEPROM_GREEN_ADDRESS  0x3
#define intEEPROM_TEAL_ADDRESS   0x5
#define intEEPROM_ORANGE_ADDRESS 0x7
#define intEEPROM_BLUE_ADDRESS   0x9 


void generateUniqueID(void);
uint16_t getUniqueID(void);

void halt(void);

void initializeLEDs(void);
void blinkGreen(int times);
void blinkRed(int times);
void blinkOrange(int times);


#define putstring(x) const_putstring(PSTR(x))
void const_putstring(const char *str);

void step(void);
void delay_s(uint8_t s);
void delay_ms(uint8_t ms);
inline volatile void delay_us(uint8_t us);
//void delay_us(uint8_t us);

uint8_t internal_eeprom_read8(uint16_t addr);
void internal_eeprom_write8(uint16_t addr, uint8_t data);
uint16_t internal_eeprom_read16(uint16_t addr);
void internal_eeprom_write16(uint16_t addr, uint16_t data);

#endif
