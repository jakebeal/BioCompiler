/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "proto_platform.h"
#include "proto.h"
#include "messages.h"

/*********************************************************/
/*     lookup table to determine msg sizes               */
/*********************************************************/

uint8_t static_msg_sizes[] = {
  6, // HOOD_MESSAGE   //00
  6, // DIGEST_MESSAGE //01
  7, // SCRIPT_MESSAGE //02
  3  // DEBUG_MESSAGE  //03
};

/************************************************************************/
/*                 functions                                            */
/************************************************************************/

uint8_t getMessageSize(uint8_t messageType, uint16_t n, uint8_t packetNumber) {
  uint8_t size = 0; // Our message size.
  uint8_t totalPackets; // Only if needed (DIGEST and SCRIPT messages).
  // Get static part (message header size):
  size += static_msg_sizes[messageType];
  // Add on varying part, based on message-specific parameters n and packetNumber:
  // (Having dynamic message sizes should be well worth the
  // the routing overhead that follows...)
  switch (messageType) {
  case HOOD_MESSAGE:
    // n: Number of "COM_DATA" shared state values.
    size += n * sizeof(COM_DATA);
    break;
  case DIGEST_MESSAGE:
    // n: Number of bytecodes in referenced script.
    totalPackets = ceil(n/(float)MAX_SCRIPT_PKT);
    uint8_t digestLength = ceil(totalPackets/8.0);
    size += digestLength;
    break;
  case SCRIPT_MESSAGE:
    // n: Number of bytecodes.
    // packetNumber: Each script packet has a number, since the script needs
    //               to be broken up.  These start with zero, which is confusing.
    totalPackets = ceil(n/(float)MAX_SCRIPT_PKT);
    if (packetNumber < (totalPackets - 1))
      size += MAX_SCRIPT_PKT;
    else
      size += n - ((totalPackets - 1) * MAX_SCRIPT_PKT);
    break;
  case DEBUG_MESSAGE:
    // n: Number of debug bytes.
    size += n;
    break;
  default:
    size = 0; // Bad messageType.
  }
  return size;
}
