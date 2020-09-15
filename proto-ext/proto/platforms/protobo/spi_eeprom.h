/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __spi_eeprom_h
#define __spi_eeprom_h

#define SPIEE_CS_PORT PORTB
#define SPIEE_CS_PIN 4

#define SPI_EEPROM_READ 0x3
#define SPI_EEPROM_WRITE 0x2
#define SPI_EEPROM_WREN 0x6
#define SPI_EEPROM_RDSR 0x5


///////////// external Eeprom addresses /////////////
// max 6 1280 byte banks in 8K eeprom (25lc640)   //

#define SERVO_RECORD_BUFFER_SIZE 1280UL
#define START_RECORD_ADDRESS     0x0

#define DEFAULT_BANK             0x0
#define START_GREEN_ADDRESS      SERVO_RECORD_BUFFER_SIZE
#define START_TEAL_ADDRESS       (SERVO_RECORD_BUFFER_SIZE*2)
#define START_ORANGE_ADDRESS     (SERVO_RECORD_BUFFER_SIZE*3)
#define START_BLUE_ADDRESS       (SERVO_RECORD_BUFFER_SIZE*4)
#define EEPROM_BANK6             (SERVO_RECORD_BUFFER_SIZE*5)


//this stores the location of the last recording that was made
#define LAST_BANK_ADDRESS        7680UL

////////////////////////////////////////////////////



uint8_t extEepromRead(uint16_t addr);
void extEepromWrite(uint16_t addr, uint8_t data);
void initExtEeprom(void);





#endif
