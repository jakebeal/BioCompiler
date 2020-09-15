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
#include "servo.h"
#include "main.h"
#include <stdio.h>

// contains stuff like led code, timing functions and other random stuff

void halt(void) {
  while (1) {
    blinkRed(1);
  }
}
/* get and generate a unique id */
uint16_t getUniqueID(void) {
  uint16_t t;
  t = internal_eeprom_read16(UNIQUEID_ADDRESS);
  //putstring("Unique ID is 0x"); putnum_uh(t); putstring("\n\r");
  //return internal_eeprom_read16(UNIQUEID_ADDRESS);
  return t;
}


void generateUniqueID(void) {
  uint8_t seed1, seed2;

  //putstring("generating unique id: ");
  /*
   * Set the ADC to read from the servo.
   */
  enableADC();
  disableADCinterrupt();

  seed1 = readADC_blocking();
  //take a first seed
  enableServo();
  setServoPosition(denormalize(~seed1));
  delay_ms(50);
  disableServo();
  seed1 = readADC_blocking();
  delay_ms(100);

  enableServo();
  setServoPosition(denormalize(~seed1));
  delay_ms(50);
  disableServo();
  seed2 = readADC_blocking();
  delay_ms(10);

  //putnum_uh(((uint16_t)seed1 << 8) | seed2);
  //putstring("\n\r");

  internal_eeprom_write16(UNIQUEID_ADDRESS, ((uint16_t)seed1 << 8) | seed2);
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

void printhex(uint8_t hex) {
#ifdef USE_UART
  hex &= 0xF;
  if (hex < 10)
    uart_putchar(hex + '0');
  else
    uart_putchar(hex + 'A' - 10);
#endif
}

void putnum_uh(uint16_t n) {
#ifdef USE_UART
  if (n >> 12)
    printhex(n>>12);
  if (n >> 8)
    printhex(n >> 8);
  if (n >> 4)
    printhex(n >> 4);
  printhex(n);

  return;
#endif 
}

void putnum_ud(uint16_t n) {
#ifdef USE_UART
  uint16_t pow;

  for (pow = 10000UL; pow >= 10; pow /= 10) {
    if (n / pow) {
      uart_putchar((n/pow)+'0');
      n %= pow;
      pow/= 10;
      break;
    }
    n %= pow;
  }
  for (;pow != 0; pow /= 10) {
    uart_putchar((n/pow)+'0');
    n %= pow;
  }
  return;
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
#ifdef IS_644
  EECR &= ~_BV(EEPM0|EEPM1);
#endif
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
