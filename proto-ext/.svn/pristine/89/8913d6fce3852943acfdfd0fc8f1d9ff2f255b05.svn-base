/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __messages_h
#define __messages_h

uint8_t getMessageSize(uint8_t messageType, uint16_t n, uint8_t packetNumber);

/*
 * Types of messages that can be sent between nodes.
 * '0' should not be a message type because it is often
 * detected as another unit is plugged in.  Be sure to
 * change the msg_sizes[] table whenever adding a
 * message type.
 */

enum MessageByteEnumeration {
  MESSAGE_LENGTH = 0x0,
  MESSAGE_TYPE, //01
  SENDER_ID     //02
};

enum MessagesEnumeration {
  HOOD_MESSAGE = 0x0,
  DIGEST_MESSAGE, //01
  SCRIPT_MESSAGE, //02
  DEBUG_MESSAGE   //03
};

#endif
