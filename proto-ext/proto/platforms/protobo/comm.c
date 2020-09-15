/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include "comm.h"
#include "messages.h"
#include "util.h"
#include "button.h"
#include "main.h"
#include "servo.h"
#include "spi_eeprom.h"
#include "debugLog.h"

//--struct used to store messages on a queue
typedef struct {
  uint8_t message[MAX_MESSAGE_SIZE];    //the message to send
  uint8_t messageLength;   	        //the number of bytes in the message
} SendMessagePacket;

//--struct that represents the queue for a channel
typedef struct {
  SendMessagePacket packetList[MAX_SEND_QUEUE_LENGTH]; //stack of messages to send: to optimize, make this actually a stack instead of a list
  uint8_t numMessagesOnQueue;                          //number of queued up messages
  uint8_t channelRetryLevel;	          //how many times we've generated a new random wait time on this channel
  volatile uint16_t channelSendWaitTime;  //how long we're going to wait before trying again to send a message on this channel
} channelQueueStruct;

channelQueueStruct channelQueuesArray[MAX_CHANNELS]; //array containing the queue for each channel

ReceiveMessagePacket receivedQueue[MAX_RECEIVE_QUEUE_LENGTH]; //queue of packets waiting to be processed

uint8_t numPacketsPendingProcessing = 0; //the number of packets in receivedQueue

uint8_t rx_parity_error = 0;
uint8_t rx_parity_errors = 0;
uint8_t tx_parity_error = 0;
uint8_t failed_to_rx = FALSE;
uint8_t rx_timeout;

/********** Communications Functions below ************************************************/


void initCommunication(void) {
  COMM_DDR = 0x0;
  COMM_PORT = 0xFF;
}

/**Sends the message to sendList.  If sending fails on one of the channels, the message will be queued up
and an attempt to resend will be made later (randomBackoff style).
 * @param sendList the list of channels to send the message to
 * @param message the message to be sent
 * @param messageLength the length of the message
   @modifies channelQueuesArray
   @effects adds the message to channels in channelQueuesArray if that channel has something queued up
   or if that channel has exceeded the max number of retries
 **/
void rbmSend(uint8_t sendList, uint8_t message[], uint8_t messageLength) {                   
  //send one channel at a time
  uint8_t send_mask = 0x01;
  uint8_t channel;
  
  for (channel=1; channel<=MAX_CHANNELS; channel++) {
    uint8_t qNumber = channel - 1;
    
    if (!(sendList & send_mask)) {
      send_mask <<= 1; // try sending to the next channel
      continue;  //channel is not in the send list
    }
    
    send_mask <<= 1; //for the next time around
        
    //---don't bother sending the message if the channel to send on has something queued up
    if (channelQueuesArray[qNumber].channelRetryLevel > MAX_RANDOM_BACKOFF_RETRIES) {
      //we've already tried too many times to send a message on this channel
      //putstring("\nNOT SENDING MESSAGE.  Exceeded Max Number of tries on channel: ");  putnum_uh(channel); putstring(" "); putnum_uh(channel);
      continue;                       //channel is no good, so don't waste time sending on it
    }
    else if (channelQueuesArray[qNumber].numMessagesOnQueue > 0) {
      //something is queued up and we're currently waiting a random amount of time before sending anything
      //putstring("\nSTUFF ON QUEUE.  ADDING MESSAGE TO CHANNEL, retry level, num messages: ");  putnum_uh(channel); putstring(" ");
      //putnum_uh(channelQueuesArray[qNumber].channelRetryLevel); putstring(" "); putnum_uh(channelQueuesArray[qNumber].numMessagesOnQueue);
      
      putMessageOnQueue(qNumber, message, messageLength);       //add the message to the queue
      continue;
    }
    
    //------------------------------try and send the message. if sending fails, put it on the queue
    if (rbSendMessageHelper(channel, message, messageLength) == TRUE) {
      //putstring("\nSuccesfully sent a message (not from a queue) on channel, retry level, num messages: "); putnum_uh(channel); putstring(" ");
      //putnum_uh(channelQueuesArray[qNumber].channelRetryLevel); putstring(" "); putnum_uh(channelQueuesArray[qNumber].numMessagesOnQueue);
      refreshChannel(channel);
      continue;
    }

    //----sending failed! put the message on the queue
    //didn't succeed, so put the message on the queue and add a time stamp
    //putstring("\nFAILED SENDING: ADDING MESSAGE TO channel, retry level, num messages: ");  putnum_uh(channel);  putstring(" ");
    //putnum_uh(channelQueuesArray[qNumber].channelRetryLevel); putstring(" "); putnum_uh(channelQueuesArray[qNumber].numMessagesOnQueue);
    
    putMessageOnQueue(qNumber, message, messageLength);
    
    disableRTC2();  //the interrupt modifies the channelSendWaitTime variable, so we need to make sure the interrupt doesn't occur during a read or write
    channelQueuesArray[qNumber].channelSendWaitTime = getRandomWaitTime(channelQueuesArray[qNumber].channelRetryLevel++);
    //putstring("\nNEW WAIT TIME = "); putnum_uh(channelQueuesArray[qNumber].channelSendWaitTime); putstring("\n");
    enableRTC2();                  
  }
}

//This does the actual sending of the message
//modifies: nothing
//effects: send message in the Q_tx_msg_buff on the channel
//NOTE: This method does NOT mess with or consider anything having to do with the channelQueuesArray at all
uint8_t rbSendMessageHelper(uint8_t channel, uint8_t message[], uint8_t messageLength) {
  if (channel <= 0 || channel > MAX_CHANNELS) {
    //putstring("\nINVALID CHANNEL PASSED TO rbSendMessageHelper:  ");  putnum_uh(channel);
    while (1) {
      blinkRed(14);
      blinkOrange(14);
    }
  }
  
  disableRTC2();
  
  if (pollClockLine(channel) == 0) {
    //putstring("\n about to ATTEMPT TO RECEIVE BEFORE SENDING on channel: "); putnum_ud(channel);
    ReceiveMessagePacket incomingPacket = receiveMessage(channel);
    if (incomingPacket.messageLength != 0) {
      //valid message, but it in the pending processing buffer
      //putstring("\nReceived Message before sending on channel "); putnum_ud(channel);
      //putstring("   Adding message to process pending queue");
      addProcessPendingMsg(incomingPacket);
    }
  }
  
  uint8_t i;
  uint8_t good_tx = TRUE;
  for (i = 0; i < messageLength; i++) {
    if (sendByte(message[i], channel) == FALSE) {
      //the message was not received.
      //putstring("failed to send a byte");
      good_tx = FALSE;
      break;
       }
    else
      good_tx = TRUE;

    refreshChannel(channel); //able to send at least 1 byte
  }
  
  /*if (channel == 1 && !good_tx)
   {
   putstring("\nch1 send failure\n");
   }*/
  enableRTC2();
  return good_tx;
}

//--------------------------------------RETRYING STUFF-------------------------
//repeatedly called by the main loop (once after each loop)
//this function will send a message on any channel that has an expired timestamp and stuff
//waiting to be sent
//effects: decrements timestamps on channels with messages waiting to be sent
//modifies: channelQueuesArray
void randomBackoffRetry() {
  uint8_t channel, qNumber;
  
  for (channel = 1; channel <= MAX_CHANNELS; channel++) {
    disableRTC2(); //needs to be disabled before reading or writing the channelSendWaitTime variable
    qNumber = channel - 1;
    
    /*putnum_uh(channel); putstring(" "); putnum_uh(channelQueuesArray[qNumber].channelRetryLevel); putstring(" ");
      putnum_uh(channelQueuesArray[qNumber].channelSendWaitTime); putstring(" ");
      putnum_uh(channelQueuesArray[qNumber].numMessagesOnQueue); putstring("\n"); */
    
    //check if a channel has messages queued up; if it does, then see if that timestamp has expired
    //expired or not, subtract from the time stamp
    if  ((channelQueuesArray[qNumber].numMessagesOnQueue > 0) &&
	 (channelQueuesArray[qNumber].channelRetryLevel < MAX_RANDOM_BACKOFF_RETRIES) &&
	 (channelQueuesArray[qNumber].channelSendWaitTime == 0))
      {
	
	//---retry sending
	if (rbSendMessageHelper(channel, channelQueuesArray[qNumber].packetList[0].message, channelQueuesArray[qNumber].packetList[0].messageLength) == TRUE) {
	  //putstring("\nSuccesfully sent a message (FROM a queue) on channel, retry level, num messages: "); putnum_uh(channel); putstring(" ");
	  //putnum_uh(channelQueuesArray[qNumber].channelRetryLevel); putstring(" "); putnum_uh(channelQueuesArray[qNumber].numMessagesOnQueue);
	  //putstring("CHANNEL IS NOW GOOD: queue, channel "); putnum_uh(qNumber); putstring(" "); putnum_uh(channel);
	  
	  removeFirstMessageFromQueue(qNumber); //sent this message, so take it off the queue
	  refreshChannel(channel);  //NOTE: we are setting the time stamp to 0   //channel works, so don't need a high retry level now
	  
	  //since the channel became good, and we want to send all the stuff w/o delay (actually we'll delay 1 main loop)
	}
	else {
	  //channel is still bad, don't remove the message from the queue.
	  //increase the time stamp
	  //putstring("CHANNEL IS STILL BAD: queue, channel");  putnum_uh(qNumber);  putstring(" "); putnum_uh(channel);
	  
	  /*if (channel == 1)
	    {
	    putstring("\n failed retry on ch 1: level = "); putnum_ud(channelQueuesArray[qNumber].channelRetryLevel);
	    }*/
	  disableRTC2();
	  channelQueuesArray[qNumber].channelSendWaitTime  = getRandomWaitTime(channelQueuesArray[qNumber].channelRetryLevel);
	  channelQueuesArray[qNumber].channelRetryLevel++; //up the rety time
	  
	  //putstring("\nNEW WAIT TIME = "); putnum_uh(channelQueuesArray[qNumber].channelSendWaitTime); putstring("\n");
	  
	  //clear the queue if it times out, so we don't keep old messages lying around
	  if (channelQueuesArray[qNumber].channelRetryLevel == MAX_RANDOM_BACKOFF_RETRIES) {
	    channelQueuesArray[qNumber].numMessagesOnQueue = 0;
	  }
	}
      }
  }
  enableRTC2();
}


/**@modifies channelQueuesArray
   @effects decrements the send wait time for channels that have messages queued up and which have a retry level
   less than the maximum retry level*/
void randomBackoffDecrementTimeStamps() {
  uint8_t channel, qNumber;
  for (channel = 1; channel <= MAX_CHANNELS; channel++) {
    qNumber = channel - 1;
    
    //check if a channel has messages queued up and is still valid; if it does, subtract from the time stamp
    if  (channelQueuesArray[qNumber].channelSendWaitTime > 0) {
      channelQueuesArray[qNumber].channelSendWaitTime--;
    }
  }
  
  //(channelQueuesArray[qNumber].numMessagesOnQueue > 0) &&
  //(channelQueuesArray[qNumber].channelRetryLevel < MAX_RANDOM_BACKOFF_RETRIES) &&
}


//--------------------------------------------------------------------
//-----------------------------------------queue stuff--------------
/**Adds the message to the queue for that channel
   @modifies channelQueuesArray*/
void putMessageOnQueue(uint8_t qNumber, uint8_t message[], uint8_t messageLength) {
  if (channelQueuesArray[qNumber].numMessagesOnQueue >= MAX_SEND_QUEUE_LENGTH) {
    return;  //don't add the message to the queue
  }
  
  uint8_t packetIndex = channelQueuesArray[qNumber].numMessagesOnQueue; //where were going to store the next packet
  
  channelQueuesArray[qNumber].packetList[packetIndex].messageLength = messageLength;
  
  //copy the message into the packetList
  uint8_t i;
  for (i = 0; i < messageLength; i++) {
    channelQueuesArray[qNumber].packetList[packetIndex].message[i] = message[i];
  }
  
  channelQueuesArray[qNumber].numMessagesOnQueue++;       //increment the number of packets on the queue
  
  /*if (qNumber == 0)
    {
    putstring("\n Qing on chan 1.  num msgs on Q after = "); putnum_ud(channelQueuesArray[0].numMessagesOnQueue);
    }*/
}

//removes the first message from the queue
void removeFirstMessageFromQueue(uint8_t qNumber) {
  if (channelQueuesArray[qNumber].numMessagesOnQueue == 0) {
    blinkRed(17);
    blinkGreen(15);
    blinkOrange(11);
    return;
  }
  
  uint8_t i = 0;
  for (i = 0; i < channelQueuesArray[qNumber].numMessagesOnQueue - 1; i++) {
    channelQueuesArray[qNumber].packetList[i] = channelQueuesArray[qNumber].packetList[i + 1];
  }
  
  channelQueuesArray[qNumber].numMessagesOnQueue--;  //one less message on the queue
}



//returns a random wait time at least as big as MIN_RETRY_TIME
uint16_t getRandomWaitTime(uint8_t channelRetryLevel) {
  uint16_t randomNumber = (rand() & 0x3) + (0x1 & getUniqueID());
  
  randomNumber = (MIN_RETRY_TIME + MIN_RETRY_TIME*channelRetryLevel + randomNumber);
  
  return randomNumber;
}


/*Sets the wait time and the channel retry level to 0*/
void refreshChannel(uint8_t channel) {
  disableRTC2();
  channelQueuesArray[channel - 1].channelRetryLevel = 0; //channel works, so don't need a high retry level now
  channelQueuesArray[channel - 1].channelSendWaitTime = 0;   //NOTE: we are setting the time stamp to 0
  enableRTC2();
}

//copies the msg into a new array (doesn't modify receivedMessage) and sends it on all channels except
//receivedMessage.incomingChannel
void resendMsgAllButCurrent(ReceiveMessagePacket receivedMessage) {
  uint8_t newMsgArray[receivedMessage.messageLength];
  
  uint8_t i;
  for (i=0; i<receivedMessage.messageLength; i++) {
    newMsgArray[i] = receivedMessage.message[i];
  }
  
  uint8_t allButCurrentList = ALL_CHANNELS & ~_BV(receivedMessage.incomingChannel-1);
  sendMessage(allButCurrentList, newMsgArray, receivedMessage.messageLength);
}

//copies the msg into a new array (doesn't modify receivedMessage) and sends it on sendList
void resendMessage(uint8_t sendList, ReceiveMessagePacket receivedMessage) {
  uint8_t newMsgArray[receivedMessage.messageLength];
  
  uint8_t i;
  for (i=0; i<receivedMessage.messageLength; i++) {
    newMsgArray[i] = receivedMessage.message[i];
  }
  
  sendMessage(sendList, newMsgArray, receivedMessage.messageLength);
}


uint8_t getGoodChannels() {
  uint8_t goodChannels = 0x0; 
  uint8_t channelMask = 0x01;
  uint8_t channel;
  
  for (channel = 1; channel <= MAX_CHANNELS; channel++) {
    
      if (channelQueuesArray[channel - 1].channelRetryLevel < MAX_RANDOM_BACKOFF_RETRIES)
	goodChannels |= channelMask;

      channelMask <<= 1;
  }
  return goodChannels;
}

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------Receive Message------------------------------------------------

/**@return a struct containing the received message and the length of the message.
 * The message length will be 0 if there was failure
 * The incomingChannel value will still be valid if there was failure receiving the message*/
ReceiveMessagePacket receiveMessage(uint8_t incoming_channel) {
  ReceiveMessagePacket incomingPacket;
  incomingPacket.messageLength = 0;
  incomingPacket.incomingChannel = incoming_channel;

  // Put th' first byte into the message array and increment the message length.
  // -> This first byte contains th' size of th' packet.
  incomingPacket.message[incomingPacket.messageLength++] = getByte(incoming_channel);
  
  // If there is a receiving problem, abort immediately.
  if (failed_to_rx) {
    incomingPacket.messageLength = 0;
    return incomingPacket;
  }
  
    // After we receive one byte, we know th' channel is good -> lower th' wait times.
  refreshChannel(incoming_channel);
  
  
  // Set the total size of incoming packet from the first byte:
  uint8_t desiredMsgLength = incomingPacket.message[MESSAGE_LENGTH];

  // Make sure the incoming message is valid:
  // -> This means getMessageSize() is greater than zero.
  if (desiredMsgLength == 0) {
    // If we have a bad message, abort immediately.
    incomingPacket.messageLength = 0;
    return incomingPacket;
  }
  
  // The message is valid, so get the rest of the message:
  while (incomingPacket.messageLength < desiredMsgLength) {
    // Put the next byte into the message array and increment the message length
    incomingPacket.message[incomingPacket.messageLength++] = getByte(incoming_channel);
    
    // If there is a receiving problem, abort immediately.
    if (failed_to_rx) {
      incomingPacket.messageLength = 0;
      return incomingPacket;
    }
  }
  return incomingPacket;
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
//-------------------------------------Send Message-----------------------------------------------------

/**Sends the message to sendList.  If sending fails on one of the channels, the message will be queued up
   and an attempt to resend will be made later (randomBackoff style).
   * @param sendList the list of channels to send the message to
   * @param message the message to be sent
   * @param messageLength the length of the message
   **/
void sendMessage(uint8_t sendList, uint8_t message[], uint8_t messageLength) {
  // Check to see if any pending messages should be sent first:
  randomBackoffRetry(); 

  // Check to see if a neighbor is already trying to send us something:
  uint8_t incoming_channel;
  if ((incoming_channel = checkAllChannels()) != 0) {
    ReceiveMessagePacket incomingPacket = receiveMessage(incoming_channel);
    if (incomingPacket.messageLength > 0) {
      // If we get a new incoming message, deal with it LATER.
      addProcessPendingMsg(incomingPacket);
    }
  }

  // Okay, send it!
  if (messageLength > 0) { // Make sure th' message isn't bogus.
    rbmSend(sendList, message, messageLength);
  }

  return;
}

// Returns True if the data pin of the given comm channel is high
// Returns False if not
uint8_t pollDataLine(uint8_t channel) {
  if (channel > 4)
    return FALSE;
  if (COMM_PIN & (1 << ((2*channel)-1)))
    return TRUE;
  return FALSE;
}

uint8_t pollClockLine(uint8_t channel) {
  if (channel > 4)
    return FALSE;
  if (COMM_PIN & (1 << (2*(channel-1))))
    return TRUE;
  return FALSE;
}

void releaseDataLine(uint8_t channel) {
  if (channel > 4)
    return;

  COMM_PORT |= _BV((2*channel)-1); // turn on pullups
  COMM_DDR &= ~_BV((2*channel)-1);
}

void releaseClockLine(uint8_t channel) {
  if (channel > 4)
    return;

  COMM_PORT |= _BV(2*(channel-1)); // turn on pullups
  COMM_DDR &= ~_BV( 2*(channel-1));
}

void setDataLine(uint8_t channel, uint8_t val) {
  if (channel > 4)
    return;
  
  COMM_DDR |= _BV((2*channel)-1); // set to output
  if (val)
    COMM_PORT |= _BV((2*channel)-1);
  else
    COMM_PORT &= ~_BV((2*channel)-1);
}

void setClockLine(uint8_t channel, uint8_t val) {
  if (channel > 4)
    return;
  
  COMM_DDR |= _BV(2*(channel-1)); // set to output
  if (val)
    COMM_PORT |= _BV( 2*(channel-1)); // turn on pullups
  else
    COMM_PORT &= ~_BV( 2*(channel-1)); // turn on pullups
}


// Checks all channels for incoming characters
// Returns channel with character on input

uint8_t checkAllChannels(void) {
  /* this takes about 2 us if no one is there */
  uint8_t in = COMM_PIN;
	
  //putstring("chan = "); putnum_uh(in); putstring("\n\r");
  //if (in & 0x55)
  //  return 0;
  if (! (in & 0x1))
    return 1;
#ifndef USE_UART
  else if (! (in & 0x4))
    return 2;
#endif
  else if (! (in & 0x10))
    return 3;
  else if (! (in & 0x40))
    return 4;
  else
    return 0;
}

uint8_t sendByte(uint8_t data, uint8_t channel) {
  cli();
  uint8_t tries = 1 ;
  
#ifdef USE_UART
  if (channel == 2) {
    sei();
    return FALSE;
  }
#endif
  
  while (1) {
    if (tx_parity_error || tries) {
      if (txChar(data, channel)) {
	sei();
	return TRUE;
      }
      else if (tries > 0) {
	tries--;
      }
    }
    else {
      sei();
      return FALSE;
    }
  }
}

// returns a byte of data being received on the specified channel
uint8_t getByte(uint8_t channel) {
  uint8_t tries = RX_RETRIES;
  uint8_t data = 0;
  failed_to_rx = TRUE;
  cli();
  while(rx_parity_error || (failed_to_rx && tries--)) 
    {
      data = rxChar(channel);
      //putnum_uh(data);
    }

  sei();
  enableRTC();
  enableRTC2();
  return data;
}


uint8_t rxChar(uint8_t channel) {
  uint8_t x;
  uint8_t parity_bit = 0;
  uint8_t rx_parity_bit = 0;
  uint8_t data = 0x00;
  uint8_t bitplace = 0x1;

  /*
   * Check for a handshake initialization.  Timeout if necessary.
   */
  rx_timeout = RX_TIMEOUT_RELOAD;
  while(pollClockLine(channel)) {
    if(rx_timeout-- == 0) {
      failed_to_rx = TRUE;
      return 0;
    }
  }

  //topobo tried to receive a msg from another active that was connected
  //debugLog(INIT_RX);

  /*
   * Return the handshake and give the transmitter time to respond.
   */
  setDataLine(channel, 0);
  delay_us(BIT_LOW_TIME);

  /*
   * Relinquish control of the data line by setting it as an input.
   */
  releaseDataLine(channel);

  /*
   * Look for the first bit.  Timeout if necessary.
   */
  rx_timeout = RX_TIMEOUT_RELOAD;
  while(! pollClockLine(channel)) {
    if(rx_timeout-- == 0) {
      failed_to_rx = TRUE;
      //debugLog(NO_FIRST_BIT);
      return 0;
    }
  }
  //uart_putchar('h');
  for(x = 0; x < 8; x++) {
    //uart_putchar('0'+x);
    /*
     * Watch the clock line for a sign that a bit is ready
     * to be read.  Timeout if necessary.
     */
    rx_timeout = RX_TIMEOUT_RELOAD;
    while(pollClockLine(channel)) {
      //	RED_LED_ON;
      //GREEN_LED_ON; //testing...
      if(rx_timeout-- == 0) {
	//RED_LED_OFF;
	failed_to_rx = TRUE;
	//debugLog(NO_START_OF_BIT);
	return 0;
      }
    }
    //      GREEN_LED_OFF;
    /*
     * Once the bit is ready, read it.
     */
    if(pollDataLine(channel)) {
      data |= bitplace;
      parity_bit ^= 0x01;
    }
    bitplace <<= 1;

    /*
     * Once a bit has been received, wait for the
     * the transmitter to declare the end of the bit.
     * Timeout if necessary.
     */
    rx_timeout = RX_TIMEOUT_RELOAD;
    // meme : something is happening here.
    while(! pollClockLine(channel)) {
      if(rx_timeout-- == 0) {
	failed_to_rx = TRUE;
	//debugLog(NO_END_OF_BIT);
	return 0;
      }
    }
  }

  /*
   * Read in the parity bit.
   */
  rx_timeout = RX_TIMEOUT_RELOAD;
  while(pollClockLine(channel)) {  // Wait until bit is ready to read.
    if(rx_timeout-- == 0) {
      failed_to_rx = TRUE;
      //debugLog(NO_PARITY_BIT);
      return 0;
    }
  }

  // Read the bit and store it. (initially = 0)
  if(pollDataLine(channel)) { 
    rx_parity_bit = 1;
  }

  rx_timeout = RX_TIMEOUT_RELOAD;
  while(!pollClockLine(channel)) {         // Wait for transmitter to finish.
    if(rx_timeout-- == 0) {
      failed_to_rx = TRUE;
      //debugLog(NO_END_OF_MESSAGE);
      return 0;
    }
  }
  rx_parity_error = (rx_parity_bit != parity_bit);   // Check if the parity bits match.

  /*
   * Acknowledge receipt of the byte just received and
   * indicate whether the byte had a parity error or not.
   */
  setClockLine(channel, 1); //to avoid capacitive noise from data_line

  if (rx_parity_error) {
    rx_parity_errors++;
    setClockLine(channel, 0);
    //debugLog(PARITY_ERROR);
  }
  setDataLine(channel, 0);    //using data line to clock the parity error
  delay_us(BIT_LOW_TIME);

  /*
   * Relinquish control of the comm lines by setting them as an input.
   */
  releaseDataLine(channel);
  releaseClockLine(channel);

  // update the debug log to acknowledge a successful tx
  //debugLog(SUCCESSFUL_RX);

  /*
   * Correctly received a byte, now return it.
   */
  failed_to_rx = FALSE;
  return data;
}


uint8_t txChar(uint8_t data, uint8_t channel) {
  uint8_t x;
  uint8_t parity_bit = 0;
  uint8_t tx_timeout;

  /*
   * Clear errors from previous transmissions.
   */
  tx_parity_error = FALSE;

  /*
   * Let neighbor know you want to talk to it.
   * Then wait for an acknowledgement.  Timeout
   * if necessary.
   */
  //these 3 lines take about 7 us
  setDataLine(channel, 1); //to avoid capacitive noise from clock line
  setClockLine(channel, 0);
  releaseDataLine(channel); // allow neighbor to respond

  tx_timeout = (TX_WAIT_FOR_NEIGHBOR_RELOAD);
  while(pollDataLine(channel)) {
    tx_timeout--;
    if(tx_timeout == 0) {
      // Communication timed out... no one listening on this channel.
      // This counts as an unsuccessful transmission.
      releaseClockLine(channel); // Set clock line back to an input.
      //if (goodChannels & (0x01 << (channel - 1))) {  // if the current channel is good, then...
      //    blinkGreen(2);  // meme
      // }
      //      debugLog(NO_HANDSHAKE_INIT);
      //if (good_channels & _BV(channel-1))
      //debugLog(NO_HANDSHAKE_ON_GOODCHANNEL);
      return FALSE;
    }
  }
  /*
   * Once acknowledged, wait for the receiving neighbor to
   * relinquish control of the data line, then assert control
   * over both clock and data.
   */
  tx_timeout = TX_TIMEOUT_RELOAD;
  while(!pollDataLine(channel)) {
    tx_timeout--;
    if(tx_timeout == 0) {
      // Communication timed out... no one listening on this channel.
      // This counts as an unsuccessful transmission.
      releaseClockLine(channel); // Set clock line back to an input.
      //if (goodChannels & (0x01 << (channel - 1))) {  // if the current channel is good, then...
      //blinkGreen(1);  // meme
      //blinkRed(1);
      //}
      //debugLog(RECEIVER_DIDNT_RELEASE_CLOCK_LINE);
      return FALSE;
    }
  }
  setClockLine(channel, 1);
  setDataLine(channel, 1);

  /*
   * Both parties are agreed on who is sending and who is receiving,
   * so start transmission of the byte, bit by bit.
   */
  for(x = 0; x < 8; x++) {
    /*
     * Send out the LSB.
     */
    setDataLine(channel, (data & 0x01));
    setClockLine(channel, 0);
	
    /*
     * Give the receiver time to process the outgoing bit.
     */
    delay_us(BIT_LOW_TIME);

    /*
     * Modify the parity bit to reflect the bit just sent and
     * prepare the data to send the next bit.
     */
    parity_bit ^= (data & 0x01);
    data >>= 1;
	
    /*
     * Return communication channel to idle state to let
     * the receiver know that a new bit is coming.
     */
    setClockLine(channel, 1);
    setDataLine(channel, 1);
    delay_us(BIT_HIGH_TIME);
  }

  /*
   * Send parity bit for error detection.
   */
  setDataLine(channel, parity_bit);
  setClockLine(channel, 0);
  delay_us(BIT_LOW_TIME);
  releaseDataLine(channel);

  //just testing here...
  //trying to drive the line high so it doesn't get accidentally driven low after release
  //under normal operations, the receiver doesn't try to control this line until 8-10 us after it's released.
  //delay_us(4);

  releaseClockLine(channel);
  /*
   * Wait for the receiver to acknowledge receipt of the byte just sent.
   * Timeout if necessary, indicating a bad transmission.
   * waiting for receiver to indicate that parity bit was received properly.
   */
  tx_timeout = TX_TIMEOUT_RELOAD;
  while(pollDataLine(channel)) { 	//avr samples about 9 times here, at 5 us intervals
    tx_timeout--;
    if(tx_timeout == 0) {
      //if (goodChannels & (0x01 << (channel - 1))) {  // if the current channel is good, then...
      //blinkGreen(1);  // meme
      //blinkRed(1);
      //}
      //debugLog(NO_BYTE_TX_RECEIPT); //RECEIVER DIDN'T ACKNOWLEDGE PARTIY BIT
      return FALSE;
    }
  }
  /*
   * Check if the byte was correctly received.
   * clock line should be high. if it's low, there
   * was a parity error.
   */
  tx_parity_error = !pollClockLine(channel);
  /*
   * Wait for receiver to finish acknowledging receipt of the byte just
   * sent.  Timeout if necessary, indicating a bad transmission.
   * waiting for receiver to release the data line so that normal
   * transmissions can continue.
   */
  tx_timeout = TX_TIMEOUT_RELOAD;
  while(!pollDataLine(channel)) {
    tx_timeout--;
    if(tx_timeout == 0) {
      //if (goodChannels & (0x01 << (channel - 1))) {  // if the current channel is good, then...
      //blinkGreen(1);  // meme
      //blinkRed(1);
      //}
      //debugLog(TX_PARITY_ERROR_TIMEOUT); //receiver didn't release data line after parity bit
      return FALSE;
    }
  }
  //debugLog(SUCCESSFUL_TX);

  return TRUE;
}


//-----------------------------------MESSAGES WAITING TO BE PROCESSED----------------------
//@effects adds  msg to the queue of messages pending processing
void addProcessPendingMsg(ReceiveMessagePacket msg) {
  if (numPacketsPendingProcessing >= MAX_RECEIVE_QUEUE_LENGTH) {
    //too many messages pending processing
    return;
  }
  
  //putstring("Adding Process pending message");
  receivedQueue[numPacketsPendingProcessing++] = msg;
}

//removes the next message from the queue of messages pending processing
//and returns it
//decrements numPacketsPendingProcessing
ReceiveMessagePacket removeProcessPendingMsg() {
  if (numPacketsPendingProcessing == 0) {
    while (1) { blinkRed(25); blinkGreen(50);}
  }
  
  ReceiveMessagePacket returnMsg = receivedQueue[0];
  
  uint8_t i = 0;
  for (i = 0; i < numPacketsPendingProcessing - 1; i++) {
    receivedQueue[i] = receivedQueue[i + 1];
  }
  
  numPacketsPendingProcessing--;
  
  return returnMsg;
}

//processes all currently pending messages
//shouldn't process new pending messages if they are added while processing the current ones
void processAllPending() {
  if (numPacketsPendingProcessing == 0) {
    return;
  }     
  
  while (numPacketsPendingProcessing > 0) { 
    processReceivedMessage(removeProcessPendingMsg());
  }
}


