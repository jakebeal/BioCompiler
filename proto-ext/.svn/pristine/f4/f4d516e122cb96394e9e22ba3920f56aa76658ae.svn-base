/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include "servo.h"
#include "proto.h"
#include "comm.h"
#include "proto_msg_structs.h"
#include "protobo.h"
#include "util.h"
#include "button.h"
#include "main.h"
#include "spi_eeprom.h"
#include "messages.h"
#include "script.h"

/*** GLOBALS ***/
TICKS ticks = 0;
MSECS msecs = 0;
DATA probes[MAX_PROBES];
uint8_t is_machine_open;
uint8_t membuf_start;
uint8_t membuf[MAX_MEM_SIZE];
uint8_t membuf_stop;
MACHINE machine_data;
DATA *coord;           // 
VEC_VAL *ranger;       /// Dummy Pointers
VEC_VAL *mouse_sensor; //
extern uint8_t normalizedADCData;
extern volatile uint8_t gossipFlag;

/*** UTILITY FUNCTIONS ***/
inline MSECS get_msecs (void) {
  return msecs;
}

inline TIME get_secs(void) {
  return 0.001 * get_msecs();
}

void uerror(char* dstring, ...) {
  return;
}

void MEM_GROW (MACHINE *m) {
  return;
}

uint8_t* MEM_CONS (MACHINE *m) {
  m->memlen = MAX_MEM_SIZE;
  m->membuf = membuf;
  m->saved_memptr = m->memptr = m->membuf;
  return m->membuf;
}

float atan2f(float x, float y){
  if(x > 0 && y > 0){
    return atan(x / y);
  }else if(x < 0 && y > 0){
    return atan(x / y) + M_PI / 4;
  }else{
    return atan(x / y) - M_PI / 4;
  }
}

/*** MAJOR VM OPERATIONS: ***/

// Initialize th' Proto VM:
void initProtobo(void) {
  memset(probes, 0, MAX_PROBES*sizeof(NUM_VAL)); // Initialize probe variables.
  is_machine_open = FALSE;

  // Create th' VM:
  MEM_CONS(&machine_data);
  machine = new_machine(&machine_data, getUniqueID(), 0, 0, 0, 0.1, script, script_len);
 
  // Default settings:
  machine->is_digest = 0;
  coord = new_tup(3, 0);

  // Bring th' VM online:
  open_machine();
  is_machine_open = TRUE;
}

void reinitHardware(void) {
  GREEN_LED_ON;
  RED_LED_ON;
  setServoPosition(denormalize(127));
  delay_ms(250);
  delay_ms(250);
  disableServo();
  RED_LED_OFF;
  GREEN_LED_OFF;
}

// Execute one Proto VM cycle:
void execProtobo(void) {
  // We're not going to worry about execution frequency manipulation for now...
  exec_machine(ticks++, get_secs());
}

// Send machine state to neighbors in HOOD_MSG format:
void exportProtoboState(void) {
  export_machine();
}

// Send current machine software state/script to neighbords:
void exportProtoboGossip(void) {
  export_script();
}

/*** VM COMMUNICATION ***/
void protoboReceiveExport (uint8_t *receivedExportMessage, uint8_t receivedMessageLength) {
  HOOD_MSG msg;
  // Zero-out th' new message struct, since we received a variable-length message.
  memset(&msg, 0, sizeof(HOOD_MSG));
  // Copy th' received message into th' fresh struct.
  memcpy(&msg, receivedExportMessage, receivedMessageLength);
  // Check for concurrency with neighbor's software.
  uint8_t currentVersion = machine->cur_script;
  if (msg.version != currentVersion) {
    //return; // The packet will not make any sense if from different software. (Handled in proto?)
  }
  // Send the packet along to th' kernel.
  radio_receive_export (msg.id, msg.version, msg.timeout,
			0, 0, 0, msg.n, msg.buf);
  return;
}

void protoboReceiveDigest (uint8_t *receivedDigestMessage, uint8_t receivedMessageLength) {
  DIGEST_MSG msg;
  // Zero-out th' new message struct, since we received a variable-length message.
  memset(&msg, 0, sizeof(DIGEST_MSG));
  // Copy th' received message into th' fresh struct.
  memcpy(&msg, receivedDigestMessage, receivedMessageLength);
  // Send the packet along to th' kernel.
  radio_receive_digest (msg.version, msg.n, msg.digest);
  return;
}

void protoboReceiveScript (uint8_t *receivedScriptMessage, uint8_t receivedMessageLength) {
  /*
  static uint8_t recvscript = 1;
  if (recvscript == 1) {
    RED_LED_ON;
    recvscript = 0;
  }
  else {
    RED_LED_OFF;
    recvscript = 1;
  }
  */
  SCRIPT_MSG msg;
  // Zero-out th' new message struct, since we received a variable-length message.
  memset(&msg, 0, sizeof(SCRIPT_MSG));
  // Copy th' received message into th' fresh struct.
  memcpy(&msg, receivedScriptMessage, receivedMessageLength);
  // Send the packet along to th' kernel.
  radio_receive_script_pkt (msg.version, msg.n, msg.pkt_num, msg.script);
  return;
}

int radio_send_export (uint8_t version, uint8_t timeout, uint8_t n, uint8_t len, COM_DATA *buf) {
  uint8_t res = 1; // Dummy return value.
  uint8_t messageLength = getMessageSize(HOOD_MESSAGE, (uint16_t)len, 0);
  uint8_t outgoingMessage[messageLength];
  HOOD_MSG msg;
  msg.messageLength = messageLength;
  msg.messageType = HOOD_MESSAGE;
  msg.id = machine->id;
  msg.n = n; // Number of 3-byte COM_DATA (state data) packets.
  msg.timeout = timeout;
  msg.version = version;
  memcpy(msg.buf, buf, len*sizeof(COM_DATA));
  memcpy(outgoingMessage, (uint8_t*)&msg, messageLength);
  sendMessage(ALL_CHANNELS, outgoingMessage, messageLength);
  return res;
}

int radio_send_digest (uint8_t version, uint16_t script_len, uint8_t *digest) {
  uint8_t res = 1; // Dummy return value.
  uint8_t digestLength = ceil(ceil(script_len/MAX_SCRIPT_PKT)/8.0);
  uint8_t messageLength = getMessageSize(DIGEST_MESSAGE, script_len, 0);
  uint8_t outgoingMessage[messageLength];
  DIGEST_MSG msg;
  msg.messageLength = messageLength;
  msg.messageType = DIGEST_MESSAGE;
  msg.id = machine->id;
  msg.n = script_len; // Number of digest bytes.
  msg.version = version;
  memcpy(msg.digest, digest, digestLength);
  memcpy(outgoingMessage, (uint8_t*)&msg, messageLength);
  sendMessage(ALL_CHANNELS, outgoingMessage, messageLength);
  return res;
}

int radio_send_script_pkt (uint8_t version, uint16_t n, uint8_t pkt_num, uint8_t *script_pkt) {
  uint8_t res = 1; // Dummy return value.
  uint8_t script_bytes; // Bytes of script in packet to be sent.
  uint8_t messageLength = getMessageSize(SCRIPT_MESSAGE, n, pkt_num);
  uint8_t outgoingMessage[messageLength];
  SCRIPT_MSG msg;

  // Figure out how many script bytes are going to be in this packet:
  uint8_t totalPackets = ceil(n/(float)MAX_SCRIPT_PKT);
  if (pkt_num < (totalPackets - 1))
    script_bytes = MAX_SCRIPT_PKT;
  else
    script_bytes = n - ((totalPackets - 1) * MAX_SCRIPT_PKT);

  // Now, on with th' usual fare:
  msg.messageLength = messageLength;
  msg.messageType = SCRIPT_MESSAGE;
  msg.id = machine->id;
  msg.pkt_num = pkt_num; // Number of outgoing packet.
  msg.n = n; // Number of bytes in th' script.
  msg.version = version;
  memcpy(msg.script, script_pkt, script_bytes);
  memcpy(outgoingMessage, (uint8_t*)&msg, messageLength);
  sendMessage(ALL_CHANNELS, outgoingMessage, messageLength);
  script_pkt_callback(pkt_num);
  return res;
}

void radioSendDebugMessage (uint8_t debugLength, uint8_t *message) {
  uint8_t messageLength = getMessageSize(DEBUG_MESSAGE, (uint16_t)debugLength, 0);
  uint8_t outgoingMessage[messageLength];
  DEBUG_MSG msg;
  msg.messageLength = messageLength;
  msg.messageType = DEBUG_MESSAGE;
  msg.id = machine->id;
  memcpy(msg.debug, message, debugLength);
  memcpy(outgoingMessage, (uint8_t*)&msg, messageLength);
  sendMessage(ALL_CHANNELS, outgoingMessage, messageLength);
}

void processReceivedMessage(ReceiveMessagePacket receivedMessage) {
  if (receivedMessage.messageLength == 0) return; // Invalid Packet - Ignore.
  switch (receivedMessage.message[MESSAGE_TYPE]) { // Switch on message type:
  case HOOD_MESSAGE:
    protoboReceiveExport(receivedMessage.message, receivedMessage.messageLength);
    break;
  case DIGEST_MESSAGE:
    protoboReceiveDigest(receivedMessage.message, receivedMessage.messageLength);
    break;
  case SCRIPT_MESSAGE:
    protoboReceiveScript(receivedMessage.message, receivedMessage.messageLength);
    break;
  default:
    break; // Drop packet.
  }
}

/*** PROTOBO-HARDWARE PRIMITIVE IMPLEMENTATIONS ***/

void set_r_led (NUM_VAL val) {
  if (val >= 0.5) RED_LED_ON; else RED_LED_OFF;
}

void set_g_led (NUM_VAL val) {
  if (val >= 0.5) GREEN_LED_ON; else GREEN_LED_OFF;
}

void set_b_led (NUM_VAL val) {
  if (val >= 0.5) {
    RED_LED_ON;
    GREEN_LED_ON;
  }
  else {
    RED_LED_OFF;
    GREEN_LED_OFF;
  }
}

void set_probe (DATA* val, uint8_t k) {
  DATA_SET(&probes[k], val); 
}

VEC_VAL *read_coord_sensor () {
  NUM_SET(&VEC_GET(coord)->elts[0], machine->x);
  NUM_SET(&VEC_GET(coord)->elts[1], machine->y);
  NUM_SET(&VEC_GET(coord)->elts[2], machine->z);
  return VEC_GET(coord);
}

NUM_VAL read_button (uint8_t n) {
  return getButton();
}

NUM_VAL read_bearing (void) {
  return normalizedADCData / 255.0;
}

void flex (NUM_VAL val) {
  // Translate from -pi/2 to pi/2 angle to 0 to 255 uint8_t:
  if(val > (M_PI/2.0)) val = M_PI/2.0;
  if(val < -(M_PI/2.0)) val = -M_PI/2.0;
  uint8_t targetPosition = (val + (M_PI / 2.0)) * (255.0 / M_PI);
  enableServo();
  setServoPosition(denormalize(targetPosition));
}

// NO-OPS: //
NUM_VAL read_speed () { return 0; }
void mov (VEC_VAL *val) { return; }
void set_dt (NUM_VAL dt) { return; }

NUM_VAL read_radio_range (void) {
  return machine->radio_range;
}
