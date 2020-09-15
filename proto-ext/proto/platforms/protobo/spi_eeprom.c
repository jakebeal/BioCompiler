/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <avr/io.h>
#include "util.h"
#include "spi_eeprom.h"

void initExtEeprom(void) {
  DDRB = 0xB0;
  SPCR = (1<<SPE)|(1<<MSTR) | 0x3 ; // master spi, clk=fosc/8 = 2mhz
}

void extEepromWrite(uint16_t addr, uint8_t data) {
  uint8_t status;

  do {
    SPIEE_CS_PORT &= ~_BV(SPIEE_CS_PIN); // pull CS low
    NOP; NOP;

    SPDR = SPI_EEPROM_RDSR;
    while (!(SPSR & (1<<SPIF)));
    NOP; NOP;
    SPDR = 0;
    while (!(SPSR & (1<<SPIF)));
    status = SPDR;
    SPIEE_CS_PORT |= _BV(SPIEE_CS_PIN); // pull CS high
    //putstring("[s="); putnum_uh(status); putstring("] ");
    NOP; NOP;
  } while ((status & 0x1) != 0);

  SPIEE_CS_PORT &= ~_BV(SPIEE_CS_PIN); // pull CS low
  NOP; NOP;


  SPDR = SPI_EEPROM_WREN;           // send command
  while (!(SPSR & (1<<SPIF)));
  NOP; NOP;
  SPIEE_CS_PORT |= _BV(SPIEE_CS_PIN); // pull CS high

  NOP; NOP; NOP; NOP;  // wait for write enable latch

  SPIEE_CS_PORT &= ~_BV(SPIEE_CS_PIN); // pull CS low
  NOP; NOP;

  SPDR = SPI_EEPROM_WRITE;           // send command
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr >> 8;                 // send high addr 
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr & 0xFF;               // send low addr
  while (!(SPSR & (1<<SPIF)));

  SPDR = data;               // send data
  while (!(SPSR & (1<<SPIF)));

  NOP;
  NOP;

  SPIEE_CS_PORT |= _BV(SPIEE_CS_PIN); // pull CS high
}

uint8_t extEepromRead(uint16_t addr) {
  uint8_t data;

  /*
  // check if there is a write in progress, wait
  do {
  SPIEE_CS_PORT &= ~_BV(SPIEE_CS_PIN); // pull CS low
  NOP; NOP;

  SPDR = SPI_EEPROM_RDSR;
  while (!(SPSR & (1<<SPIF)));
  NOP; NOP;
  SPDR = 0;
  while (!(SPSR & (1<<SPIF)));
  status = SPDR;
  SPIEE_CS_PORT |= _BV(SPIEE_CS_PIN); // pull CS high
  NOP; NOP;
  } while ((status & 0x1) != 0);
  */

  SPIEE_CS_PORT &= ~_BV(SPIEE_CS_PIN); // pull CS low
  NOP; NOP;

  SPDR = SPI_EEPROM_READ;           // send command
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr >> 8;                 // send high addr 
  while (!(SPSR & (1<<SPIF)));

  SPDR = addr & 0xFF;               // send low addr
  while (!(SPSR & (1<<SPIF)));
  NOP;
  NOP;

  SPDR = 0;
  while (!(SPSR & (1<<SPIF)));
  data = SPDR;
  //putstring("Read %x\n\r", data);

  SPIEE_CS_PORT |= _BV(SPIEE_CS_PIN); // pull CS high
  return data;
}
