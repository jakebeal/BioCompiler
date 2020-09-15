/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
//#include <avr/signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "util.h"
#include "spi_eeprom.h"
#include "servo.h"
#include "button.h"
#include "comm.h"
#include "main.h"
#include "messages.h"


uint8_t is_servo_initialized = NOT_INITIALIZED;
//uint8_t is_data_initialized = NOT_INITIALIZED;

uint16_t play_curr_eeprom_bank_addr;

// The Minimum and Maximum servo values (raw PWM?) to control left and right
//    as determined by calibration
uint16_t servo_min;
uint16_t servo_max;

// The Minimum and Maximum analog to digital conversions of servo position
// based on potentiometer (generally from .9 V to 2.6 V
uint16_t servo_read_min;
uint16_t servo_read_max;

// Useful values that only need to be calculated once
uint16_t servo_range;
uint16_t servo_read_range;

// for a running average of adc values
uint32_t old_ravg = 0;
#define COEFF 700 //wass 500 // higher number is smoother, but slower

// sets up the PWM output for the servo
// freq = 40Hz, clock = 16MHz -> clock divider = 8 & TOP = 0xC350 (see PWM eq. in datasheet) 
#define SERVO_PWM_TOP 0xC350UL  // dep. on clock

uint8_t is_ADC_done = FALSE;

//*************************************************************
//   Servo Initilization, etc
//*************************************************************

void initializeServo(void) {
  // set up the PWM signal
  TCCR1A = (1<<COM1A1) // clear output pin on match, set at TOP
    |  (1 <<  WGM11); // TOP == ICR1, fast PWM mode
  TCCR1B = (1 << WGM13) | (1 << WGM12) // see above line
    | 0x2 ; // prescale by 8
  ICR1 = SERVO_PWM_TOP; // sets the PWM frequency, 40Hz

  // the record/playback timer is done with the RTC
}

// Enables output pin for servo
void enableServo(void) {
  SERVO_DDR |= _BV(SERVO_PORT_NO);
}

// Disables output pin for servo
void disableServo(void) {
  SERVO_DDR &= ~_BV(SERVO_PORT_NO);
}

// Controls the servo position by setting the PWM duty cycle
void setServoPosition(uint16_t pwm_value) {
  if (pwm_value > ICR1) // dont go beyond this for sure!
    return;
  if (pwm_value > SERVO_MAXIMUM)
    pwm_value = SERVO_MAXIMUM;
  if (pwm_value < SERVO_MINIMUM)
    pwm_value = SERVO_MINIMUM;

  OCR1A = pwm_value;    // this is the compare value for the PWM system
}

//***************************************************
//          ADC FUNCTIONS
//***************************************************

SIGNAL(SIG_ADC) {
  is_ADC_done = TRUE;
}

// Only one ADC, for servo reading!
void initializeADC(void) {
  ADMUX = (1 << REFS1) | (1 << REFS0) // internal 2.56V voltage reference,
    | 0x0; // single ended ADC0 input
  
  //  ADMUX = (0 << REFS1) | (0 << REFS0) // external AREF voltage reference,
  // | 0x0; // single ended ADC0 input
  
  DDRA &= ~ _BV(DDA1); //using ADC0, set as input

  ADCSRA = _BV(ADEN) // enable adc
    // ADC clock must be less than 200KHz to run properly.
    // 16MHz / 200Khz = 80, so freq divide clock by 128:
    | _BV(ADPS2)| _BV(ADPS1)| _BV(ADPS0);
  
  // do one first read to get the ADC circuitry running.
  enableADC();
  readADC_blocking();
  disableADC();
}

void enableADC(void) {
  ADCSRA |= _BV(ADEN); // adc enable
}

void disableADC(void) {
  ADCSRA &= ~_BV(ADEN); // adc disable
}

void startADC(void) {
  ADCSRA |= _BV(ADSC); // start conversion!
}

void enableADCinterrupt(void) {
  ADCSRA |= _BV(ADIE);
}

void disableADCinterrupt(void) {
  ADCSRA &= ~_BV(ADIE);
}

// function does not return until ADC is done (no interrupts)
uint16_t readADC_blocking(void) {
  uint16_t v;
  
  startADC();
  loop_until_bit_is_set(ADCSRA, ADIF);
  v = ADC;
  
  return v;
}

//**********************************************************
//          SERVO CALIBRATION FUNCTIONS
//**********************************************************
// This function checks whether or not the servos need to be calibrated.
void checkCalibration(uint8_t force_calibration) {
  recoverCalibrationData();
  if (force_calibration || is_servo_initialized != INITIALIZED) {
    calibrateServo();
    recoverCalibrationData();
  }
  disableServo();
}

void calibrateServo(void) {
  uint16_t current;
  uint16_t c = 0;
  uint16_t trialServoPosition;

  /*
   * Alert user to calibration.
   */
  blinkGreen(1);
  blinkRed(1);
  RED_LED_ON;
  GREEN_LED_ON;

  /* do a testing scheme for the guys in China...
     step 1- Change all the Pins as  PC0 ,PC1 , PC2 ,PC3,PC4 ,PC5 ,PC6, PC7  
     to output  and set it to "0"  and wait 1 sec  
  */
  setClockLine(1, 0);
  setDataLine(1, 0);
  setClockLine(2, 0);
  setDataLine(2, 0);
  setClockLine(3, 0);
  setDataLine(3, 0);
  setClockLine(4, 0);
  setDataLine(4, 0);
  delay_s(1);
  /*
     step  2 -  set a I/O pin (from pC0 to PC7)  to  "high"  and wait 0.5sec  , 
     than set this  i/o to "0" again 
     and  set the next  i/o pin to high  and go to step 2 until PC7 was set .  
  */
  setClockLine(1, 1);
  delay_ms(250); delay_ms(250);
  setClockLine(1, 0);

  setDataLine(1, 1);
  delay_ms(250); delay_ms(250);
  setDataLine(1, 0);

  setClockLine(2, 1);
  delay_ms(250); delay_ms(250);
  setClockLine(2, 0);

  setDataLine(2, 1);
  delay_ms(250); delay_ms(250);
  setDataLine(2, 0);

  setClockLine(3, 1);
  delay_ms(250); delay_ms(250);
  setClockLine(3, 0);

  setDataLine(3, 1);
  delay_ms(250); delay_ms(250);
  setDataLine(3, 0);

  setClockLine(4, 1);
  delay_ms(250); delay_ms(250);
  setClockLine(4, 0);

  setDataLine(4, 1);
  delay_ms(250); delay_ms(250);
  setDataLine(4, 0);

  /*
     step 3-  set back  all the i/o pin to  the normal setting 
     and start the motor initial  procedure . 
   */
  releaseDataLine(1);
  releaseClockLine(1);
  releaseDataLine(2);
  releaseClockLine(2);
  releaseDataLine(3);
  releaseClockLine(3);
  releaseDataLine(4);
  releaseClockLine(4);

  /*
   * Enable the servo. (ADC should be initialized already)
   */
  enableServo();

  /*
   * Position the servo against its lower mechanical stop.
   */
  putstring("positioning against lower stop...");
  trialServoPosition = ((SERVO_MINIMUM + SERVO_MAXIMUM)/2) - CALIBRATION_CENTER_OFFSET; //haze

  while (trialServoPosition > SERVO_MINIMUM) {
    trialServoPosition--;
    setServoPosition(trialServoPosition);

    putstring(" pos: ");
    putnum_uh(trialServoPosition);

    delay_ms(5);
  }
  servo_min = trialServoPosition;
  putstring("done\n\r");
  /*
   * Take a reading of lower position.
   */
  disableServo();
  delay_ms(50);

  servo_read_min =
   (int32_t) (readADC_blocking() + readADC_blocking() + readADC_blocking() + readADC_blocking()) / 4;

  if (servo_read_min < 10)
    servo_read_min = 10;

  putstring("servo read minimum: ");
  putnum_ud(servo_read_min);
  putstring("\n\r");

  /*
   * Find the position of the lower mechanical stop.
   */
  enableServo();
  trialServoPosition = ((SERVO_MINIMUM + SERVO_MAXIMUM)/2) - CALIBRATION_CENTER_OFFSET; // haze
  c = 0;

  /*putstring("trial servo pos: 0x");
    putnum_uh(trialServoPosition); */

  setServoPosition(trialServoPosition);
  delay_ms(200);

  while (trialServoPosition-- > SERVO_MINIMUM) {
    putstring("  ");
    putnum_uh(trialServoPosition);

    setServoPosition(trialServoPosition);
    delay_ms(5);

    if (c != 0 && (c%5) == 0) { //read the adc
      disableServo();
      //delay_ms(50); //wait for motor noise to die off
      current =
	(readADC_blocking() + readADC_blocking() +
	 readADC_blocking() + readADC_blocking()) / 4;
      enableServo();

      putstring(" adc: ");
      putnum_ud(current);

      if (current < 10)
	current = 10;

      if (abs((signed long int) (current - servo_read_min)) <= SERVO_EQUALITY_THRESHOLD) {
	servo_min = trialServoPosition;
	break;
      }
    }
    c++;
  }
  putstring("servo min: ");
  putnum_ud(servo_min);
  putstring("\n\r");
  RED_LED_OFF;

  /*
   * Position the servo against its upper mechanical stop.
   */
  putstring("positioning against upper stop...");
  trialServoPosition = ((SERVO_MINIMUM + SERVO_MAXIMUM)/2) + CALIBRATION_CENTER_OFFSET;  //haze
  while (trialServoPosition < SERVO_MAXIMUM) {
    trialServoPosition++;
    setServoPosition(trialServoPosition);
    delay_ms(5);
  }
  servo_max = trialServoPosition;
  putstring("done\n\r");

  /*
   * Take a reading of upper position.
   */
  disableServo();
  delay_ms(50);

  servo_read_max =
    ((readADC_blocking() + readADC_blocking() +
      readADC_blocking() + readADC_blocking()) / 4);

  putstring("servo read maximum: ");
  putnum_ud(servo_read_max);
  putstring("\n\r");

  /*
   * Find the position of the upper mechanical stop.
   */
  enableServo();
  trialServoPosition =
    ((SERVO_MINIMUM + SERVO_MAXIMUM)/2) + CALIBRATION_CENTER_OFFSET;
  c = 0;

  setServoPosition(trialServoPosition);
  delay_ms(200);

  while (trialServoPosition < SERVO_MAXIMUM) {
    trialServoPosition++;
    setServoPosition(trialServoPosition);

    putstring(" p+ ");
    putnum_uh(trialServoPosition);

    if (c != 0 && (c%5) == 0) {
      disableServo();
      delay_ms(5);

      current =
	((readADC_blocking() + readADC_blocking() +
	  readADC_blocking() + readADC_blocking()) / 4);

      enableServo();
      setServoPosition(trialServoPosition);
      if (abs((signed long int)(servo_read_max-current)) <= SERVO_EQUALITY_THRESHOLD) {
	servo_max = trialServoPosition;
	break;
      }
    }
    delay_ms(5);
    c++;
  }
  putstring("servo max: 0x");
  putnum_uh(servo_max);
  putstring("\n\r");
  GREEN_LED_OFF;

  is_servo_initialized = INITIALIZED;
  writeCalibrationData();
}

void writeCalibrationData(void) {
  // write all this to the internal eeprom!
  internal_eeprom_write16(SERVO_MIN_ADDRESS, servo_min);
  internal_eeprom_write16(SERVO_MAX_ADDRESS, servo_max);
  internal_eeprom_write16(SERVO_READ_MIN_ADDRESS, servo_read_min);
  internal_eeprom_write16(SERVO_READ_MAX_ADDRESS, servo_read_max);
  internal_eeprom_write16(SERVO_RECBUFSIZE_ADDRESS, SERVO_RECORD_BUFFER_SIZE);
  //internal_eeprom_write8(DATA_INITIALIZED_ADDRESS, is_data_initialized);
  internal_eeprom_write8(SERVO_INITIALIZED_ADDRESS, is_servo_initialized);
}

void writeIsDataInitialized(void) {
  // write all this to the internal eeprom!
  //internal_eeprom_write8(DATA_INITIALIZED_ADDRESS, is_data_initialized);
  //internal_eeprom_write8(SERVO_INITIALIZED_ADDRESS, is_servo_initialized);
}

void recoverCalibrationData(void) {
  uint8_t data;
  servo_min = internal_eeprom_read16(SERVO_MIN_ADDRESS);
  servo_max = internal_eeprom_read16(SERVO_MAX_ADDRESS);
  servo_read_min = internal_eeprom_read16(SERVO_READ_MIN_ADDRESS);
  servo_read_max = internal_eeprom_read16(SERVO_READ_MAX_ADDRESS);
  is_servo_initialized = internal_eeprom_read8(SERVO_INITIALIZED_ADDRESS);
  //is_data_initialized = internal_eeprom_read8(DATA_INITIALIZED_ADDRESS);
  data = is_data_initialized();
  servo_range = servo_max - servo_min;
  servo_read_range = servo_read_max - servo_read_min;
}

uint8_t normalizeADC(uint16_t pot_value) {
  // turn the 16 bit adc value into a normalized (for this servo) 8 bit
  uint32_t temp;

  // decrease resolution to eliminate some noise
  pot_value & 0xfffe; //strip off ls-bit

  //putnum_uh(pot_value); uart_putchar(',');
  if (pot_value > servo_read_max)
    pot_value = servo_read_max;
  else if (pot_value < servo_read_min)
    pot_value = servo_read_min;

  temp = pot_value - servo_read_min;

  //putnum_ud(temp); uart_putchar(' ');

  temp <<= 8;
  temp /= servo_read_range;

  
  // do some low pass filtering
  // running average = (old running avg)*(Coeff) + (new sample)*(1-Coeff)
  if (old_ravg != 0) //gets set to 0 in startServoRecord()
    temp = ( (old_ravg * COEFF / 1000) + (temp * (1000-COEFF) / 1000) );   
  old_ravg = temp;
  

  //uart_putchar('>'); putnum_ud(temp); uart_putchar(' ');

  if (temp >= 255)
    return 254;
   
  return (temp & 0xff);
}
     

// Converts 8bit servo range position to raw PWM value, to be passed to set_servo_position
uint16_t denormalize (uint8_t data_value) {
  uint32_t temp;

  temp = servo_range;
  temp *= data_value;
  temp >>= 8;
  temp += servo_min;

  return (uint16_t)temp;
}

//*************************************************************
//         Record/Playback FUNCTIONS
//*************************************************************
uint8_t is_data_initialized(void) {
  uint16_t data;
  data = extEepromRead(play_curr_eeprom_bank_addr);
  data <<= 8;
  data |= extEepromRead(play_curr_eeprom_bank_addr + 1);

  if (data == 0)
    return FALSE;

  else
    return TRUE;
}
