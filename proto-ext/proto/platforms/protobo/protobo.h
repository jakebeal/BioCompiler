/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

void initProtobo(void);
void execProtobo(void);
void exportProtoboState(void);
void exportProtoboGossip(void);
void protoboReceiveExport (uint8_t *receivedExportMessage, uint8_t receivedMessageLength);
void protoboReceiveDigest (uint8_t *receivedDigestMessage, uint8_t receivedMessageLength);
void protoboReceiveScript (uint8_t *receivedScriptMessage, uint8_t receivedMessageLength);
void processReceivedMessage(ReceiveMessagePacket receivedMessage);
