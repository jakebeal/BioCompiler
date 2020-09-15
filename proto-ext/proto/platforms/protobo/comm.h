/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#ifndef __comm_h
#define __comm_h

///////////////// messaging constants //////////////////////////////////

#define MAX_RECEIVE_QUEUE_LENGTH 2
#define MAX_SEND_QUEUE_LENGTH 2
#define MAX_MESSAGE_SIZE 22
#define MAX_RANDOM_BACKOFF_RETRIES 4

//MIN_RETRY_TIME is measured in hundreds of microseconds
//e.g. 15 = 1500us = 1.5ms
#define MIN_RETRY_TIME 15

///////////////// message container //////////////////////////////////

/**The struct returned by receiveMessage and the struct stored in the random backoff queues
* Note: all of the returned arrays will have size MAX_MESSAGE_SIZE, so the messageLength
* variable should be checked to see which bytes actually contain information*/
typedef struct
{
 uint8_t message[MAX_MESSAGE_SIZE];    //the message we received
 uint8_t messageLength;   			   //the number of bytes in the received message
 uint8_t incomingChannel;			   //the channel this message was received on
} ReceiveMessagePacket;

///////////////// PINS ///////////////////////////////////////////////////

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

/////////////////// Channel Assignment Information //////////////////////////////////

#define MAX_CHANNELS    4
#define CHANNEL1        1
#define CHANNEL2        2
#define CHANNEL3        3
#define CHANNEL4        4

/////////////////// Channels to send to. //////////////////////////////////

#ifdef USE_UART

#define ALL_CHANNELS 0xFD

#else

#define ALL_CHANNELS 0xFF

#endif // USE_UART

// Networking variables - VAS (A lot of these are not used in Protobo)
#define MSG_ID (receivedMessage.message[0] & ~(0x80))
#define ROUTING_MODIFIER(msg_type) (((msg_type & 0x80) == 0x80) ? 3 : 0) // 3 byte addition;

#define CURRENT_CHANNEL _BV(receivedMessage.incomingChannel-1)
#define ALL_BUT_CURRENT_CHANNEL (ALL_CHANNELS & ~_BV(receivedMessage.incomingChannel-1))

#define SEND_CHANNEL1 0x01
#define SEND_CHANNEL2 0x02
#define SEND_CHANNEL3 0x04
#define SEND_CHANNEL4 0x08

///////////////// bit banging constants //////////////////////////////////

//10 = 60 us. 20 = 100 us. usually takes a part 20 us to respond.
#define TX_WAIT_FOR_NEIGHBOR_RELOAD 30

//30 = 10 us - too low. 60 works. want it to be about twice the bit_low_time
#define RX_TIMEOUT_RELOAD 60

//used to be 30.
#define TX_TIMEOUT_RELOAD 60

#define BIT_LOW_TIME 18 //20 us
#define BIT_HIGH_TIME 7 //20 us

// Number of retries for transmitting and receiving bytes.
#define RX_RETRIES 3
#define TX_TRIES 4 

///////////////// functions ///////////////////////////////////////////////////

uint8_t rxChar(uint8_t channel);
uint8_t txChar(uint8_t data, uint8_t channel);

ReceiveMessagePacket receiveMessage(uint8_t incoming_channel);
void sendMessage(uint8_t sendList, uint8_t message[], uint8_t messageLength);
void resendMsgAllButCurrent(ReceiveMessagePacket receivedMessage);
void resendMessage(uint8_t sendList, ReceiveMessagePacket receivedMessage);

void addProcessPendingMsg(ReceiveMessagePacket msg);
ReceiveMessagePacket removeProcessPendingMsg(void);
void processAllPending(void);

uint8_t getByte(uint8_t channel);
uint8_t sendByte(uint8_t data, uint8_t channel);

void loadRxMsgBuff(uint8_t b);
void loadTxMsgBuff(uint8_t b);
uint8_t checkAllChannels(void);
void initCommunication(void);

// Returns True if the data pin of the given comm channel is high
// Returns False if not
uint8_t pollDataLine(uint8_t channel);
uint8_t pollClockLine(uint8_t channel);
void releaseDataLine(uint8_t channel);
void releaseClockLine(uint8_t channel);
void setDataLine(uint8_t channel, uint8_t val);
void setClockLine(uint8_t channel, uint8_t val);


void rbmSend(uint8_t sendList, uint8_t message[], uint8_t messageLength);
uint8_t rbSendMessageHelper(uint8_t channel, uint8_t message [], uint8_t messageLength);
void putMessageOnQueue(uint8_t qNumber, uint8_t message[], uint8_t messageLength);
uint16_t getRandomWaitTime(uint8_t channelRetryLevel);
void removeFirstMessageFromQueue(uint8_t qNumber);
void randomBackoffDecrementTimeStamps(void);
void randomBackoffRetry(void);  
void refreshChannel(uint8_t incoming_channel);

uint8_t getGoodChannels(void);


#endif
