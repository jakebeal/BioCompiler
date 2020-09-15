/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __servo_h
#define __servo_h

#define SERVO_PORT PORTD  // servo is on port D5, OC1A (output compare for PWM)
#define SERVO_DDR DDRD
#define SERVO_PORT_NO 5

// 25ms between samples for rec/play
#define SERVO_SAMPLE_PERIOD (25 / RTC_PERIOD) 

// STATE of servo
#define NOT_INITIALIZED       0
#define INITIALIZED           101

// Hardware Servo minimum and Servo maximum values
#define SERVO_MINIMUM 0x400
#define SERVO_MAXIMUM 0x1600

// servo calibration offset from center (to speed calibration)
#define CALIBRATION_CENTER_OFFSET 0 //500

// Min Max threshold
//this determines the accuracy of the motor calibration
#define SERVO_EQUALITY_THRESHOLD 10

// this controls playback synch on startup 
// needs to be higher or better synchronized
#define RECORD_EQUALITY_THRESHOLD 80  

// Local States in servo
enum ServoStatesEnumeration {
   SERVO_RECORD,
   SERVO_PLAYBACK,
   SERVO_IDLE
};



// Servo functions
void initializeServo(void);
void setServoPosition(uint16_t);
void enableServo(void);
void disableServo(void);
uint8_t normalizeADC(uint16_t pot_value);
uint16_t denormalize (uint8_t data_value);

// ADC functions
void initializeADC(void);
void enableADC(void);
void disableADC(void);
void startADC(void);
uint16_t readADC_blocking(void);
void enableADCinterrupt(void);
void disableADCinterrupt(void);


// Servo Calibration
void checkCalibration(uint8_t force_cal);
void calibrateServo(void);
void writeCalibrationData(void);
void recoverCalibrationData(void);

// record + playback functions
void startServoRecord(void);
void startPrepareToRecord(void);
void stopServoRecord(void);
void startRecordFromQueen(uint8_t phase, uint8_t freq, uint8_t amp);
void startServoIdle(void);
void startServoPlayback(void);
void stopServoPlayback(void);
void stopRecordFromPlayback(void);
uint8_t is_data_initialized(void);
 

#endif
