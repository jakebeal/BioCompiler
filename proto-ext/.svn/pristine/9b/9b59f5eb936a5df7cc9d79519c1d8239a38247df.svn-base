/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __comm_h
#define __comm_h


#define COMM_PORT       PORTC
#define COMM_DDR       	DDRC
#define COMM_PIN       	PINC

#define CLOCK1          0  // I/O clock pin 1
#define DATA1           1  // I/O data pin 1

#define CLOCK2          2  // I/O clock pin 2
#define DATA2           3  // I/O data pin 2

#define CLOCK3          4  // I/O clock pin 3
#define DATA3           5  // I/O data pin 3

#define CLOCK4          6  // I/O clock pin 4
#define DATA4           7  // I/O data pin 4

// Channel Assignment Information
#define MAX_CHANNELS    4
#define CHANNEL1        1
#define CHANNEL2        2
#define CHANNEL3        3
#define CHANNEL4        4

#define RETRY_INCREMENT 1 //this incrementally increases the length of handshaking after successive failures
#define TX_WAIT_FOR_NEIGHBOR_RELOAD 200 //8=28us. 10=30us. 12=55us. 15=65us. 20=100us. 40=180us. usually takes a part 20 us to respond. used to be 20.
#define RX_TIMEOUT_RELOAD 60 //30 is 10 us - too low. 60 works. want it to be about twice the bit_low_time
#define TX_TIMEOUT_RELOAD 60 //used to be 30.
#define BIT_LOW_TIME 18 // 18 = 20 us. 20 = 22 us. used to be 20.
#define BIT_HIGH_TIME 7 // 7 = 20 us. 20 = 30 us. used to be 20

// Number of retries for transmitting and receiving bytes.
#define RX_RETRIES 5 //(dockHost ? 6 : 3)// 3 //used to be 6
#define TX_TRIES 1 //(dockHost ? 8 : 4) // 4   //used to be 8

//how many bytes are added to the end of a message to keep info
//currently keeps a hop count to kill looping msgs
//this limits the max number of topobo nodes that can be in a network!!!
#define NUM_MSG_FOOTER_BYTES 1
#define MAX_MSG_HOPS 50

// Channels to send to.
#define ALL_GOOD_CHANNELS 0x01// channel one only! (0x0F & good_channels) // four channels, one bit each
#define ALL_CHANNELS 0x01

#define ALL_GOOD_BUT_CURRENT_CHANNEL (ALL_GOOD_CHANNELS & ~_BV(incoming_channel-1))
#define CURRENT_CHANNEL _BV(incoming_channel-1)
#define SEND_CHANNEL1 0x01
#define SEND_CHANNEL2 0x02
#define SEND_CHANNEL3 0x04
#define SEND_CHANNEL4 0x08




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

#define MESSAGE_BUFFER_SIZE 22

// Message data structure for preventing resending messages in network loops 
#define MESSAGE_LOG_SIZE 3
struct Message
{
  int messageType;
  int messageID;
} MessageLog[MESSAGE_LOG_SIZE];


#define MSG_ID (rx_msg_buff[0] & ~(0x80))		// mask out the routing bit (high bit)
#define ROUTING_MODIFIER(id) (((id & 0x80) == 0x80) ? 3 : 0) // 3 byte addition;

uint8_t rxChar(uint8_t channel);
uint8_t txChar(int data, int channel);


void debugLog(uint8_t error);
void loadDebugLog(void);
void saveDebugLog(void);
void eraseDebugLog(void);


uint8_t receiveMessage(uint8_t incoming_channel);
uint8_t receiveWhileSending(uint8_t incoming_channel);
void processReceivedWhileSendingMessage(void);
void sendMessage(uint8_t c);
void resendMessage(uint8_t channels);
uint8_t getByte(uint8_t channel);
uint8_t sendByte(uint8_t data, uint8_t channel);

void loadRxMsgBuff(uint8_t b);
void loadTxMsgBuff(uint8_t b);
uint8_t checkAllChannels(void);
void initCommunication(void);

uint8_t pollClockLine(uint8_t channel);

#endif
