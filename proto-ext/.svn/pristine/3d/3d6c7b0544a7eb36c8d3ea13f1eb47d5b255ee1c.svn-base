/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __main_h
#define __main_h

//#define USE_UART 1
//#define DEBUG 1  // this enables recording of eeprom debug statistics. saving the dubug log causes topobo not to rx msgs for 250 ms during save (every 2.5 sec). 

#ifdef IS_644
#define FINE_TIMER    TIMER2_COMPA_vect
#define GENERIC_TIMER TIMER0_COMPA_vect

/* NEED TO REMAP THE USART REGISTERS */
#define UBRRL UBRR0L
#define UBRRH UBRR0H
#define UCSRA UCSR0A
#define UDRE UDRE0
#define UDR UDR0
#define RXC RXC0
#else
#define FINE_TIMER    SIG_OUTPUT_COMPARE2
#define GENERIC_TIMER SIG_OUTPUT_COMPARE0
#endif

#define BOOTLOADER_CHANNEL CHANNEL1 //the dongle has the 2 comm lines connected to each other
#ifdef IS_644
#define BOOT_START 0x7C00 //bootloader start address FOR MEGA644
#else
#define BOOT_START 0x3E00 //bootloader start address FOR MEGA32
#endif

void getServoCalibration(void);
void goto_boot(void);
void testBootloader(void);
void initHardware(void);
void initRTC(void);
void enableRTC(void);
void disableRTC(void);
void initRTC2(void);
void enableRTC2(void);
void disableRTC2(void);

void doTimeouts(void);


#define RTC_PERIOD 1 //in ms
#define PING_TIMER_RELOAD (10000UL / RTC_PERIOD) // (1.5 seconds between pings//  used to be 3 when 3000: ~ avg. 1.5 second refresh between pairs of neighbors)
#define FORCE_CAL_TIMEOUT_RELOAD (3000UL / RTC_PERIOD)

#endif
