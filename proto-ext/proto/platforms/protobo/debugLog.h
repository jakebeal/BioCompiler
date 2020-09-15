/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __debugLog_h
#define __debugLog_h


//for debugging of communications code
enum errorEnumeration {
  //rx errors:
  INIT_RX = 0,
  NO_FIRST_BIT,
  NO_START_OF_BIT,
  NO_END_OF_BIT,
  NO_PARITY_BIT, 
  NO_END_OF_MESSAGE,
  PARITY_ERROR,
  SUCCESSFUL_RX,
  //tx errors:
  NO_HANDSHAKE_INIT,
  NO_HANDSHAKE_ON_GOODCHANNEL,
  RECEIVER_DIDNT_RELEASE_CLOCK_LINE,
  NO_BYTE_TX_RECEIPT,
  TX_PARITY_ERROR_TIMEOUT,
  SUCCESSFUL_TX,
  MESSAGE_COMPLETE_AND_PROCESSED // this must be the last message
	
};


void debugLog(uint8_t error);
void loadDebugLog(void);
void saveDebugLog(void);
void eraseDebugLog(void);

#endif
