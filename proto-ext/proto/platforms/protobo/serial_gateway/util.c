/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include "util.h"
#include "main.h"
#include <stdio.h>

// contains stuff like led code, timing functions and other random stuff

void halt(void) {
  while (1) {
    blinkRed(1);
  }
}

//**************************************************
//        LEDS
//**************************************************


void blinkGreen(int times) {
  while(times--) {
    GREEN_LED_ON;
    delay_ms(200);
    GREEN_LED_OFF;
    delay_ms(200);
  }
}

void blinkRed(int times) {
  while(times--) {
    RED_LED_ON;
    delay_ms(200);
    RED_LED_OFF;
    delay_ms(200);
  }
}

void blinkOrange(int times) {
  while(times--) {
    RED_LED_ON;
    GREEN_LED_ON;
    delay_ms(200);
    RED_LED_OFF;
    GREEN_LED_OFF;
    delay_ms(200);
  }
}

//**************************************************
//         UART
//**************************************************


void initializeUART(void) {
#ifdef USE_UART
  uint16_t baud = (F_CPU / (16 * BAUDRATE)) - 1;
  
  /* setup the UART */
  UBRRL = (uint8_t)baud;               // set baudrate
  UBRRH = (uint8_t)(baud>>8);

  /* attach the UART to stdio */
  fdevopen(NULL, uart_getchar, 0);   // opens stdin to uart
  fdevopen(uart_putchar, NULL, 0);   // opens stdout to uart
#endif
}

void disableUART(void) {
  UCSRB &= ~((1<<RXEN) | (1<<TXEN));
  DDRD &= ~(0x3);
}

void enableUART(void) {
#ifdef USE_UART
  UCSRB |= (1<<RXEN) | (1<<TXEN);    // read and write
#endif
}



void const_putstring(const char *str) {
#ifdef USE_UART
  char c;

  while ((c = pgm_read_byte(str++)))
    uart_putchar(c);
#endif
}

int uart_putchar(char c)
{
#ifdef USE_UART
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;
#endif
  return 0;
}


int uart_getchar(void) {
#ifdef USE_UART
  char c;
  loop_until_bit_is_set(UCSRA, RXC);
  c = UDR;
  return (int)c;
#else
  return 0;
#endif
}


//**************************************************
//         Internal EEPROM
//**************************************************

uint8_t internal_eeprom_read8(uint16_t addr) {
  loop_until_bit_is_clear(EECR, EEWE); // wait for last write to finish
  EEAR = addr;
  EECR |= _BV(EERE);        // start EEPROM read
  return EEDR;            // takes only 1 cycle
}

void internal_eeprom_write8(uint16_t addr, uint8_t data) {
  //printf("writing %d to addr 0x%x...", data, addr);
  loop_until_bit_is_clear(EECR, EEWE); // wait for last write to finish
  EEAR = addr;
  EEDR = data;
  cli();                // turn off interrupts 
  EECR |= _BV(EEMWE);     // these instructions must happen within 4 cycles
  EECR |= _BV(EEWE);      // empirically measured to take 0.75 us. 

  sei();                // turn on interrupts again
  //putstring("done\n\r");
}

void internal_eeprom_write16(uint16_t addr, uint16_t data) {
  internal_eeprom_write8(addr, data & 0xFF);
  internal_eeprom_write8(addr+1, data >> 8);
}

uint16_t internal_eeprom_read16(uint16_t addr) {
  uint16_t v;

  v = internal_eeprom_read8(addr+1);
  v <<= 8;
  v |= internal_eeprom_read8(addr);

  return v;
}

//**************************************************
//         TIMING & DELAY
//**************************************************
void step(void) {
#ifdef USE_UART
  putstring("Step...");
  uart_getchar();
  putstring("\n\r");
#endif
}

void delay_s(uint8_t s) 
{
  while (s > 0) {
    delay_ms(250);
    delay_ms(250);
    delay_ms(250);
    delay_ms(250);
    s--;
  }
}

void delay_ms(uint8_t ms)
{
  unsigned short delay_count = F_CPU / 4000;
#ifdef __GNUC__
  unsigned short cnt;
  asm volatile ("\n"
		"L_dl1%=:\n\t"
		"mov %A0, %A2\n\t"
		"mov %B0, %B2\n"
		"L_dl2%=:\n\t"
		"sbiw %A0, 1\n\t"
		"brne L_dl2%=\n\t"
		"dec %1\n\t" "brne L_dl1%=\n\t":"=&w" (cnt)
		:"r"(ms), "r"((unsigned short) (delay_count))
		);
#else
  unsigned short delay_cnt = F_CPU/6000;
  //unsigned short delay_cnt = 2400;   //*KU* for 14.745600 MHz Clock

  unsigned short delay_cnt_buffer;

  while (ms--) {
    delay_cnt_buffer = delay_cnt;
    while (delay_cnt_buffer--);
  }
#endif
}


// FIXME: make this timing correct
inline volatile void delay_us(uint8_t us) {
  while (us != 0) {
    us--;
    NOP; NOP; NOP;
    NOP; NOP; NOP;
    NOP; NOP; NOP;
  }
}
