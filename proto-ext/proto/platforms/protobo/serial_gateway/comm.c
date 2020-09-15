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
#include "comm.h"
#include "util.h"
#include "button.h"
#include "main.h"
#include "../messages.h"

uint8_t message_log_index =0 ;
uint8_t message_counter = 0;
uint8_t rx_parity_error = 0;
uint8_t rx_parity_errors = 0;
uint8_t tx_parity_error = 0;

uint8_t failed_to_rx = FALSE;

uint8_t routing_flag = FALSE;

uint8_t rx_timeout;

uint8_t num_bytes_in_rx_msg_buff = 0;
uint8_t num_bytes_in_tx_msg_buff = 0;

uint8_t rx_msg_buff[MESSAGE_BUFFER_SIZE];
uint8_t tx_msg_buff[MESSAGE_BUFFER_SIZE];

uint8_t num_bytes_in_rx_while_sending = 0;
uint8_t rx_while_sending_msg_buff[MESSAGE_BUFFER_SIZE];
uint8_t rcvd_while_sending_channel; //channel received message was on

uint8_t is_rx_msg_in_queue = 0;

uint8_t handshake_retry_addition;

uint8_t good_channels = 0x0F;

void initCommunication(void) {
  COMM_DDR = 0x0;
  COMM_PORT = 0xFF;
}
/*
void resendMessage(uint8_t channels) {
  uint8_t len, i;

  len = getMessageSize(rx_msg_buff[0]);
  for (i=0; i<len; i++) {
    loadTxMsgBuff(rx_msg_buff[i]);
  }
  sendMessage(channels);
}
*/

uint8_t receiveMessage(uint8_t incoming_channel) {
  
  uint8_t msg_len = 0;
  num_bytes_in_rx_msg_buff = 0;
  // Put th' first byte into the message array and increment the message length.
  // -> This first byte contains th' size of th' packet.
  //    We use it to set msg_len:
  loadRxMsgBuff(msg_len = getByte(incoming_channel));
  
  // If there is a receiving problem, abort immediately.
  if (failed_to_rx) {
    return -1;
  }

  // Make sure the incoming message is valid:
  // -> This means getMessageSize() is greater than zero.
  if (msg_len == 0) {
    // If we have a bad message, abort immediately.
    return -1;
  }
  
  // The message is valid, so get the rest of the message:
  while (num_bytes_in_rx_msg_buff < msg_len) {
    loadRxMsgBuff(getByte(incoming_channel));
    if (failed_to_rx) {
      return -1;
    }
  }

  is_rx_msg_in_queue = TRUE;
  good_channels |= _BV(incoming_channel-1);
  return 0;
}


//------------------------------receive while sending-----------------------------------
/*receives the message on incoming_channel and puts it into the
rx_while_sending_msg_buff and NOT the rx_msg_buff */
/*
uint8_t receiveWhileSending(uint8_t incoming_channel)
{
  rcvd_while_sending_channel = incoming_channel;
  
  uint8_t msg_len;

  //uart_putchar('m');
  //uart_putchar('0'+incoming_channel);

  num_bytes_in_rx_while_sending = 0;
  rx_while_sending_msg_buff[num_bytes_in_rx_while_sending++] = getByte(incoming_channel);

  if (failed_to_rx) {
    num_bytes_in_rx_while_sending = 0;
    return -1;
  }
  msg_len = getMessageSize(rx_while_sending_msg_buff[0]);
  //putstring("msg len = %d\n\r", msg_len);

  if (msg_len)
  {
    while (num_bytes_in_rx_while_sending < msg_len)
    {
      rx_while_sending_msg_buff[num_bytes_in_rx_while_sending++] = getByte(incoming_channel);
      if (failed_to_rx)
      {
	num_bytes_in_rx_while_sending = 0;
  	return -1;
      }
    }
  }
  else
  {
    num_bytes_in_rx_while_sending = 0;
  }
  return 0;
}
*/
 /*
void processReceivedWhileSendingMessage()
{
   is_rx_msg_in_queue = TRUE;
  
   uint8_t i;
   for (i = 0; i < num_bytes_in_rx_while_sending; i++)
   {
       //copy over to the rx_msg_buff
       rx_msg_buff[i] = rx_while_sending_msg_buff[i];
   }
   
   num_bytes_in_rx_while_sending = 0;
   sendSerialMessage();
}
 */
//-------------------------------------------------------------------


void sendMessage(uint8_t send_list) {
  uint8_t i, tries, channel, send_mask;

  // send to only good channels
  send_list &= ALL_GOOD_CHANNELS;
    
  
  for (tries=TX_TRIES; tries>0; tries--) {

    handshake_retry_addition = (tries * RETRY_INCREMENT);

    //trying to make it do the sendMessage more times on failed channels
    send_mask = 0x01;
    for (channel=1; channel<=MAX_CHANNELS; channel++) {
      uint8_t good_tx = TRUE;
      if (send_list & send_mask) {
	//putnum_ud(channel);
	/*
	 * Check if the line to transmit on is already being used.
	 * If so, receive the message and then carry on.
	 */
	if (pollClockLine(channel) == 0) {
	  receiveMessage(channel);
	}
	for (i=0; i<num_bytes_in_tx_msg_buff; i++) {
	  if (sendByte(tx_msg_buff[i], channel) == FALSE) {
	    //the message was not received.
	    //mark this channel not to be cleared from sendList so it can be resent later.
	    good_tx = FALSE;
	    break;
	  }
	  else
	    good_tx = TRUE;
	}
      }
      if (good_tx == TRUE) {
	send_list &= ~send_mask;  //tx was good. clear the bit from sendList to avoid resending
      }
      send_mask <<= 1; // try sending to the next channel
    }
  }
  // update goodChannels to remove channels that did not succeed
  // this works because sendList keeps track of which channels have not received the message yet.
  good_channels &= ~send_list;
  num_bytes_in_tx_msg_buff = 0;
}


void loadRxMsgBuff(uint8_t b) {
  if (num_bytes_in_rx_msg_buff < MESSAGE_BUFFER_SIZE) {
    rx_msg_buff[num_bytes_in_rx_msg_buff++] = b;
  }
}

void loadTxMsgBuff(uint8_t b) {
  if (num_bytes_in_tx_msg_buff < MESSAGE_BUFFER_SIZE) {
    tx_msg_buff[num_bytes_in_tx_msg_buff++] = b;
  }
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
  //   signed int tries = TX_TRIES;
  signed int tries = 1 ;

#ifdef USE_UART
  if (channel == 2)
    return FALSE;
#endif

  while (1) {
    if (tx_parity_error || tries) {
      if (txChar(data, channel)) {
	return TRUE;
      }
      else if (tries > 0) {
	tries--;
      }
    }
    else {
      return FALSE;
    }
  }
}

// returns a byte of data being received on the specified channel
uint8_t getByte(uint8_t channel) {
  uint8_t tries = RX_RETRIES;
  uint8_t data;

  failed_to_rx = TRUE;
  while(rx_parity_error || (failed_to_rx && tries--)) {
    data = rxChar(channel);
    //putnum_uh(data);
  }

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
  debugLog(INIT_RX);

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
      debugLog(NO_FIRST_BIT);
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
      if(rx_timeout-- == 0) {
	failed_to_rx = TRUE;
	debugLog(NO_START_OF_BIT);
	return 0;
      }
    }
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
	debugLog(NO_END_OF_BIT);
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
      debugLog(NO_PARITY_BIT);
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
      debugLog(NO_END_OF_MESSAGE);
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
    debugLog(PARITY_ERROR);
  }
  setDataLine(channel, 0);    //using data line to clock the parity error
  delay_us(BIT_LOW_TIME);

  /*
   * Relinquish control of the comm lines by setting them as an input.
   */
  releaseDataLine(channel);
  releaseClockLine(channel);

  // update the debug log to acknowledge a successful tx
  debugLog(SUCCESSFUL_RX);

  /*
   * Correctly received a byte, now return it.
   */
  failed_to_rx = FALSE;
  return data;
}


uint8_t txChar(int data, int channel) {
  uint8_t x;
  uint8_t parity_bit = 0;
  uint16_t tx_timeout;

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

  tx_timeout = (TX_WAIT_FOR_NEIGHBOR_RELOAD + handshake_retry_addition);

  while(pollDataLine(channel)) {
    tx_timeout--;
    if(tx_timeout == 0) {
      // Communication timed out... no one listening on this channel.
      // This counts as an unsuccessful transmission.
      releaseClockLine(channel); // Set clock line back to an input.
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
      //      debugLog(RECEIVER_DIDNT_RELEASE_CLOCK_LINE);
      return FALSE;
    }
  }
  setClockLine(channel, 1);
  setDataLine(channel, 1);

  sei();

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
      debugLog(NO_BYTE_TX_RECEIPT); //RECEIVER DIDN'T ACKNOWLEDGE PARTIY BIT
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
      debugLog(TX_PARITY_ERROR_TIMEOUT); //receiver didn't release data line after parity bit
      return FALSE;
    }
  }
  debugLog(SUCCESSFUL_TX);

  good_channels |= _BV(channel - 1); // Mark this channel as good.
  return TRUE;
}


uint16_t error_log[MESSAGE_COMPLETE_AND_PROCESSED];

void loadDebugLog(void){
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
  uint16_t addr = DEBUG_LOG_ADDRESS;
  uint8_t i;
  for (i=0; i<= MESSAGE_COMPLETE_AND_PROCESSED; i++) {
    internal_eeprom_write16(addr, error_log[i]);
    addr += 2;
  }
}

void debugLog(uint8_t error) {
  if (error > MESSAGE_COMPLETE_AND_PROCESSED)
    blinkOrange(5);
	
  if (error_log[error] != 0xFFFE) //don't overflow
    error_log[error]++;
}
