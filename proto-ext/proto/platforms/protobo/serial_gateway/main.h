/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

//
#ifdef IS_644
#define FINE_TIMER    TIMER2_COMPA_vect
#define GENERIC_TIMER TIMER0_COMPA_vect
/*
#define T_OCR0 OCR0A
#define T_OCR2 OCR2A
#define T_TIMSK TIMSK2
#define T_OCIE2 OCIE2A
#define T_OCIE0 OCIE0A
*/
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

void initHardware(void);
void initRTC(void);
void enableRTC(void);
void disableRTC(void);
void gotoBoot(void);
void testBootloader(void);
void putnum_uh(uint16_t n);
void putnum_ud(uint16_t n);

#define USE_UART 1
#define RTC_PERIOD 1 //25 // in ms. this is actually 2*the timer freq, since we only check the timers every other interrupt.
#define PING_TIMER_RELOAD (3000UL / RTC_PERIOD) // (3 seconds between pings ~ avg. 1.5 second refresh between pairs of neighbors)
#define FORCE_CAL_TIMEOUT_RELOAD (3000UL / RTC_PERIOD)
