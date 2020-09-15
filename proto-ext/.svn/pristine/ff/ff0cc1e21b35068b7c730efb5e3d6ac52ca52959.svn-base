/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include "util.h"
#include "debugLog.h"


uint16_t error_log[MESSAGE_COMPLETE_AND_PROCESSED];

void loadDebugLog(void){
#ifdef DEBUG //in main.h

  uint16_t addr = DEBUG_LOG_ADDRESS;
  uint8_t i;
  uint16_t temp;
	
  //putstring("\nDebugLog: ");
  for (i=0; i<= MESSAGE_COMPLETE_AND_PROCESSED; i++) {
    temp = internal_eeprom_read16(addr);
		
    if (temp == 0xFFFF) //if it's blank (0xffff), make it 0
      error_log[i] = 0x0000;
		
    else 
      error_log[i] = temp;
		
    addr += 2;
  }
	
  putstring("\r\nDebugLog:");
  putstring("\r\n INIT_RX = "); putnum_ud(error_log[INIT_RX]);
  putstring ("\r\n SUCCESSFUL_RX = "); putnum_ud( error_log[SUCCESSFUL_RX] );
  putstring("\r\n    //rx errors: ");
  putstring ("\r\n NO_FIRST_BIT = "); putnum_ud( error_log[NO_FIRST_BIT] );
  putstring ("\r\n NO_START_OF_BIT = "); putnum_ud( error_log[NO_START_OF_BIT] );
  putstring ("\r\n NO_END_OF_BIT = "); putnum_ud( error_log[ NO_END_OF_BIT] );
  putstring ("\r\n NO_PARITY_BIT = "); putnum_ud( error_log[NO_PARITY_BIT] );
  putstring ("\r\n NO_END_OF_MESSAGE = "); putnum_ud( error_log[NO_END_OF_MESSAGE] );
  putstring ("\r\n PARITY_ERROR = "); putnum_ud( error_log[PARITY_ERROR] );
  putstring ("\r\n    //tx errors: ");
  putstring ("\r\n NO_HANDSHAKE_INIT = "); putnum_ud( error_log[NO_HANDSHAKE_INIT] );
  putstring ("\r\n NO_HANDSHAKE_ON_GOODCHANNEL = "); putnum_ud( error_log[NO_HANDSHAKE_ON_GOODCHANNEL]  );
  putstring ("\r\n RECEIVER_DIDNT_RELEASE_CLOCK_LINE = "); putnum_ud( error_log[RECEIVER_DIDNT_RELEASE_CLOCK_LINE] );
  putstring ("\r\n NO_BYTE_TX_RECEIPT = "); putnum_ud( error_log[NO_BYTE_TX_RECEIPT] );
  putstring ("\r\n TX_PARITY_ERROR_TIMEOUT = "); putnum_ud( error_log[TX_PARITY_ERROR_TIMEOUT] );
  putstring ("\r\n\r\n SUCCESSFUL_TX = "); putnum_ud( error_log[SUCCESSFUL_TX] );
  putstring ("\r\n MESSAGE_COMPLETE_AND_PROCESSED = "); putnum_ud( error_log[MESSAGE_COMPLETE_AND_PROCESSED] );
  putstring ("\r\n  \r\n");

#endif
}

void eraseDebugLog(void) {
  uint16_t addr = DEBUG_LOG_ADDRESS;
  uint8_t i;
	
  putstring("\n erasing DebugLog! \n ");
  for (i=0; i<= MESSAGE_COMPLETE_AND_PROCESSED; i++) {
    internal_eeprom_write16(addr, 0x0000);
    addr += 2;
  }
} 

void saveDebugLog(void){
#ifdef DEBUG //in main.h

  uint16_t addr = DEBUG_LOG_ADDRESS;
  uint8_t i;
  for (i=0; i<= MESSAGE_COMPLETE_AND_PROCESSED; i++) {
    internal_eeprom_write16(addr, error_log[i]);
    addr += 2;
  }
#endif
}

void debugLog(uint8_t error) {
#ifdef DEBUG //in main.h
  if (error > MESSAGE_COMPLETE_AND_PROCESSED)
    blinkOrange(5);
	
  if (error_log[error] != 0xFFFE) //don't overflow
    error_log[error]++;

#endif
}

