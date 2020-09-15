/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "servo.h"
#include "util.h"
#include "button.h"
#include "main.h"
#include "comm.h"
#include "protobo.h"
#include "spi_eeprom.h"
#include "messages.h"
#include "debugLog.h"

/*** GLOBALS ***/
// Servo ADC Data
extern volatile uint8_t is_ADC_done;
volatile uint8_t servo_timer = 0;
uint8_t normalizedADCData;

// Protobo VM Clock
extern volatile int32_t msecs;
extern volatile int32_t ticks;

// Button Handler
extern volatile uint16_t button_timer;
extern volatile uint8_t button_status;
extern uint8_t button_disabled;
volatile uint8_t last_button = 0;

// Timer to check for user's servo recalibration signal
volatile uint16_t force_calibration_timeout=0;

// Timeout Flag for Mainloop (to be set every millisecond from GENERIC_TIMER interrupt)
uint8_t RTCflag = FALSE;

// Gossip Signal (Send Digests) From Protobo
uint8_t gossipFlag;

/*** TIMER INTERRUPTS***/
// 100us = 0.1ms Clock Interrupt: For fine-timing stuff like randomBackoff retries.
ISR(FINE_TIMER) {
  // This interrupt takes 8us to execute
  randomBackoffDecrementTimeStamps();
}

// 1ms Clock Interrupt: Check Servo ADC, Button, Set Timeout Flag for Mainloop
ISR(GENERIC_TIMER) {
  // haze : put the high priority stuff first, and for low priority, set flags and handle
  // them in the mainloop
  /****************************************
 SERVO
  ****************************************/
  // put this first for precision.

  if (servo_timer > SERVO_SAMPLE_PERIOD)
    servo_timer = 0;
  if (servo_timer == 0) {
    // do the servo thing
    startADC();     // start the ADC, will take a while.
  }
  servo_timer++;

  /**************************************
  TIMEOUTS - in mainloop
  **************************************/
  RTCflag = TRUE;  // Timeouts Flag

  /****************************************
  BUTTON HANDLER
  ****************************************/
  uint8_t curr_button;
  if (! button_disabled) {
    if (button_timer != 0xFFFF)  // start counting, but don't let it roll over!
      button_timer++;
    
    curr_button = BUTTON_PRESSED; //poll the button pin 
    // did something just change?
    if ((button_timer >= BUTTON_DEBOUNCE) && (curr_button != last_button)) {
      last_button = curr_button;
      if (curr_button) { // ie just pressed
	if (button_status == FALSE) {
	  button_timer = 0;          // reset timer to prep for release
	}
	else if ((button_status == SINGLE_CLICK) &&
		 (button_timer < DOUBLE_CLICK_TIMEOUT)) {
	  button_timer = 0;     // ok second doubleclick press
	}
      } else {     // most stuff is done on the release....
	if (button_status == FALSE)  { // just released from single click
	  button_status = SINGLE_CLICK;  // clicked once
	  button_timer = 0;             // don't let click again too quickly
	}
	else if ((button_status == SINGLE_CLICK) &&
		 (button_timer < DOUBLE_CLICK_TIMEOUT)) {
	  // just released from double click
	  button_status = DOUBLE_CLICK;
	  button_timer = 0;
	}
      }
    }
  }

  /****************************************
  Update Protobo VM Clock
  ****************************************/
  msecs++;
}

int main(void) {
  /***  BOOT SEQUENCE  ***/
  testBootloader();       // Check to see if we're trying to reprogram.
  initHardware();         // Initialize LEDS, Servo/ADC, Button, UART, Flash Startup LEDs
  initProtobo();          // Initialize Protobo VM, Allocate Memory, Place Initial Settings
  initRTC();              // Start 1ms Clock Interrupt (GENERIC_TIMER)
  initExtEeprom();        // 
  initCommunication();    //
  initRTC2();             // Start 100us Clock Interrupt (FINE_TIMER)
  sei();                  // Enable Interrupts
  enableButton();         // Start noticing button presses
  enableADC();            // Turn on the analog to digital converter
  getServoCalibration();  // Check for recalibration signal from user or read calibration from flash
  enableADCinterrupt();   // Start receiving interrupts when th' ADC is ready (instead of blocking).


  // Set Machine ID
  if (getUniqueID() == 0xffff) { //if it's these characters, it's blank, i.e. never been set
    generateUniqueID();
  }

  /*
   *  TH' MAIN LOOP
   */
  while (1) {
    /*** COMMUNICATIONS ***/
    randomBackoffRetry(); // Retry sending anything that we failed to send before
    processAllPending();  // Process all messages waiting to be processed

    uint8_t incoming_channel;
    if ((incoming_channel = checkAllChannels()) != 0) { // Check for incoming transmissions
      ReceiveMessagePacket incomingPacket = receiveMessage(incoming_channel);
      if (incomingPacket.messageLength > 0) {
	// If we have a new incoming packet, deal with it NOW.
	processReceivedMessage(incomingPacket);
      }
    }

    /*** TIMEOUTS ***/
    if (RTCflag == TRUE) { // Flag set once per millisecond, from GENERIC_TIMER interrupt
      RTCflag = FALSE;
      doTimeouts();
    }

    /*** Retrieve and store incoming data from the servo. ***/
    if (is_ADC_done) {
      normalizedADCData = normalizeADC(ADC);
      is_ADC_done = FALSE;
    }
  }
}

void doTimeouts(void) {
  static uint8_t execProtoFlag = 32;
  static uint8_t exportProtoFlag = 64;
  static uint8_t gossipProtoFlag = 20;
  // Execute one cycle of th' Protobo VM
  if (--execProtoFlag == 0) {
    execProtobo();
    execProtoFlag = 64;
  }
  // Send state message (HOOD_MESSAGE) to all members of neighborhood
  if (--exportProtoFlag == 0) {
      exportProtoboState();
      exportProtoFlag = 64;
      gossipProtoFlag--;
  }
  // Send current software state/script (DIGEST_MESSAGE/SCRIPT_MESSAGE)
  // to all members of neighborhood.
  if (gossipProtoFlag == 0) {
    exportProtoboGossip();
    gossipProtoFlag = 20;
  }
}

// Set up RTC (interrupt 0) to run @ 1ms per interrupt
void initRTC(void) {
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
//  T_OCR0 = 250;   // to make 1KHz clock
//  T_TIMSK |= _BV(T_OCIE0); // interrupt on compare
}

// Set up RTC2 (interrupt 2) to run @ 100us per interrupt
void initRTC2(void) {
#ifdef IS_644
  TCCR2A = _BV(WGM21);   // CTC mode
  TCCR2B |= 0x3; // clock prescale by 64
  OCR2A = 25;   // to make 10KHz clock
  TIMSK2 |= _BV(OCIE2A); // interrupt on compare
#else
  TCCR2 = _BV(WGM21);   // CTC mode
  TCCR2 |= 0x3; // clock prescale by 64
  OCR2 = 25;   // to make 10KHz clock
  TIMSK |= _BV(OCIE2); // interrupt on compare
#endif
//  T_OCR2 = 25;   // to make 10KHz clock
//  T_TIMSK |= _BV(T_OCIE2); // interrupt on compare
}

void enableRTC(void) {
#ifdef IS_644
  TIMSK0 |= _BV(OCIE0A); // interrupt on compare
#else
  TIMSK |= _BV(OCIE0); // interrupt on compare
#endif
//  T_TIMSK |= _BV(T_OCIE0); // interrupt on compare
}

void enableRTC2(void) {
#ifdef IS_644
  TIMSK2 |= _BV(OCIE2A); // interrupt on compare
#else
  TIMSK |= _BV(OCIE2); // interrupt on compare
#endif
//  T_TIMSK |= _BV(T_OCIE2); // interrupt on compare
}

void disableRTC(void) {
#ifdef IS_644
  TIMSK0 &= ~_BV(OCIE0A); // interrupt on compare
#else
  TIMSK &= ~_BV(OCIE0); // interrupt on compare
#endif
//  T_TIMSK &= ~_BV(T_OCIE0); // interrupt on compare
}

void disableRTC2(void) {
#ifdef IS_644
  TIMSK2 &= ~_BV(OCIE2A); // interrupt on compare
#else
  TIMSK &= ~_BV(OCIE2); // interrupt on compare
#endif
//  T_TIMSK &= ~_BV(T_OCIE2); // interrupt on compare
}

void getServoCalibration(void) {
  // User must hold button for three seconds to signal servo recalibrations.
  force_calibration_timeout = 3000UL;
  // Wait until th' button is no longer pressed:
  while(BUTTON_PRESSED && (force_calibration_timeout != 0)) {
    delay_ms(1);
    force_calibration_timeout--;
  }
  if (force_calibration_timeout == 0) {
    eraseDebugLog();             // Erase internal eeprom statistics
    checkCalibration(TRUE);      // Force recalibration
  }
  else
    checkCalibration(FALSE);     // Load previous calibration from internal memory
}

// Reprogramming Hack: Jump to user's application
void goto_boot(void) {
  (*((void(*)(void))BOOT_START))(); //jump
}

// Reprogramming Hack: Check for user's signal to reprogram
void testBootloader(void) {
  if ((pollClockLine(BOOTLOADER_CHANNEL)) && (pollDataLine(BOOTLOADER_CHANNEL))) {     //check for neighbor not handshaking
    setDataLine(BOOTLOADER_CHANNEL, 0);
    delay_ms(1);
    if (!pollClockLine(BOOTLOADER_CHANNEL)) { //could erroneously detect start of handshaking here
      releaseDataLine(BOOTLOADER_CHANNEL);
      if (pollClockLine(BOOTLOADER_CHANNEL)) { //should happen faster than handshaking
	blinkRed(1); //for user feedback
	goto_boot(); //start the bootloader
         }
      }
   }
}

void initHardware(void) {
  initializeUART();  // for debugging - gets turned on/off in main.h
  disableUART(); //
  enableUART();

  // setup PORT directions & pullups & stuff

  initializeServo();
  initializeADC();
  // etc...//

  LED_DDR |= _BV(RED_LED) | _BV(GREEN_LED);
  BUTTON_DDR &= ~_BV(BUTTON); // Make button input
  BUTTON_PORT |= _BV(BUTTON); // Turn on button pullup

  // LED sequence alerts user to startup: RED, GREEN, YELLOW, GREEN, GO!
  // NOTE: This is where we see looping with 24V Power supply on ATMega644's
  RED_LED_ON;
  delay_ms(100);
  RED_LED_OFF;
  GREEN_LED_ON;
  delay_ms(100);
  RED_LED_ON;
  delay_ms(100);
  RED_LED_OFF;
  delay_ms(100);
  GREEN_LED_OFF;
}
