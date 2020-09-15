/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

/*
 * porting serial gateway from PIC on 2/16/06. Hayes Raffle.
 */
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <stdio.h>
#include "util.h"
#include "button.h"
#include "main.h"
#include "comm.h"
#include "serialGateway.h"

extern volatile uint16_t interByteTimer;
extern volatile uint16_t button_timer;
extern volatile uint8_t button_status;
extern uint8_t button_disabled;
volatile uint8_t last_button = 0;
volatile uint16_t force_calibration_timeout=0;
volatile uint16_t ping_timer = 1;
extern volatile uint16_t sendStatusToPC;
extern volatile int16_t backpack_timer;
uint8_t remoteDebounceTimer = 0;
uint8_t remoteDebounce = FALSE;
uint8_t RTCflag = FALSE;
extern uint8_t good_channels;
extern uint8_t receiving_serial_message;


// gets called once per ms, for general purpose timing stuff (like button)
// haze : put the high priority stuff first, and for low priority, set flags and handle
// them in the mainloop
SIGNAL(GENERIC_TIMER) {
  //  TIMEOUTS - in serialLoop
  interByteTimer++;
}


int main(void) {
  //uint16_t temp;
  //uint8_t b;     //the button
  testBootloader();
  initHardware();

  RED_LED_ON;
  GREEN_LED_ON;
  delay_ms(200);
  RED_LED_OFF;
  GREEN_LED_OFF;

  initRTC();
  initCommunication();

  //putstring("Topobo serial gateway 2/17/06 v1\n\r");
  
  sei(); // enable interrupts

  force_calibration_timeout = 3000UL; // have to hold down button for 3s to force cal
  while(BUTTON_PRESSED && (force_calibration_timeout != 0)){  // wait until the button is not pressed
    //putnum_ud(force_calibration_timeout); uart_putchar(' ');
    delay_ms(1);
    force_calibration_timeout--;  
  }
  if (force_calibration_timeout == 0){
    eraseDebugLog(); //erase internal eeprom statistics
  }

  //startup lights
  RED_LED_ON; delay_ms(100); RED_LED_OFF;
  GREEN_LED_ON; delay_ms(100);
  RED_LED_ON; delay_ms(100); RED_LED_OFF;
  delay_ms(100);  GREEN_LED_OFF;

  //loadDebugLog();
  //  enableButton(); // set up the button interrupt
  
  // the main loop
  serialLoop();
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

void initRTC(void) {
  // setup RTC (interrupt 0) to run @ 1ms per interrupt
#ifdef IS_644
  TCCR0A = _BV(WGM01);   // CTC mode
  TCCR0B |= 0x3; // clock prescale by 64
  OCR0A = 250;   // to make 1KHz clock
  TIMSK0 |= _BV(OCIE0A); // interrupt on compare
#else
  TCCR0 = _BV(WGM01);   // CTC mode
  TCCR0 |= 0x3; // clock prescale by 64
  OCR0 = 250;   // to make 1KHz clock
  TIMSK |= _BV(OCIE0); // interrupt on compare
#endif
}

void enableRTC(void) {
#ifdef IS_644
  TIMSK0 |= _BV(OCIE0A); // interrupt on compare
#else
  TIMSK |= _BV(OCIE0); // interrupt on compare
#endif
}

void disableRTC(void) {
#ifdef IS_644
  TIMSK0 &= ~_BV(OCIE0A); // interrupt on compare
#else
  TIMSK &= ~_BV(OCIE0); // interrupt on compare
#endif
}

void initHardware(void) {
  initializeUART();  // for debugging - gets turned on/off in main.h
  disableUART(); //
  enableUART();

  // setup PORT directions & pullups & stuff

  LED_DDR |= _BV(RED_LED) | _BV(GREEN_LED);
  BUTTON_DDR &= ~_BV(BUTTON); // make button input
  BUTTON_PORT |= _BV(BUTTON); // turn on button pullup
}

//jump to user's application
void gotoBoot(void) {
  (*((void(*)(void))BOOT_START))(); //jump
}

void testBootloader(void) {
  if ((pollClockLine(BOOTLOADER_CHANNEL)) && (pollDataLine(BOOTLOADER_CHANNEL))) {     //check for neighbor not handshaking
    setDataLine(BOOTLOADER_CHANNEL, 0);
    delay_ms(1);
    if (!pollClockLine(BOOTLOADER_CHANNEL)) { //could erroneously detect start of handshaking here
      releaseDataLine(BOOTLOADER_CHANNEL);
      if (pollClockLine(BOOTLOADER_CHANNEL)) { //should happen faster than handshaking
	blinkRed(1); //for user feedback
	gotoBoot(); //start the bootloader
         }
      }
   }
}
