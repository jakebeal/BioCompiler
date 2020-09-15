/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
#include <stdio.h>
#include "util.h"
#include "main.h"
#include "comm.h"
#include "../messages.h"
#include "button.h"
#include "serialGateway.h"

uint8_t serial_msg_buff[MESSAGE_BUFFER_SIZE];
uint8_t num_bytes_in_serial_msg_buff = 0;
uint8_t serialMessageLength;
uint8_t interByteTimeout;
uint8_t errorStatus;
uint8_t uart_tx_complete = FALSE;

volatile uint16_t interByteTimer;
volatile uint16_t sendStatusToPC = 1;
extern volatile uint16_t ping_timer;
extern uint8_t num_bytes_in_rx_msg_buff;
extern uint8_t num_bytes_in_tx_msg_buff;
extern uint8_t rx_msg_buff[MESSAGE_BUFFER_SIZE];
extern uint8_t tx_msg_buff[MESSAGE_BUFFER_SIZE];


uint8_t pcToTopoboCircBuffer[PC_TO_TOPOBO_BUFFER_SIZE];
uint8_t topoboToPcCircBuffer[TOPOBO_TO_PC_BUFFER_SIZE];
uint8_t pcToTopoboCircBuffer_write = 0;
uint8_t pcToTopoboCircBuffer_read = 0;
uint8_t topoboToPcCircBuffer_write = 0;
uint8_t topoboToPcCircBuffer_read = 0;
uint8_t receiving_serial_message = FALSE;

extern uint8_t num_bytes_in_rx_while_sending;

/*
 * This interrupt is called any time a byte is ready in the
 * RS232 receive buffer.
 */
SIGNAL(SIG_UART_RECV) 
{
  uint8_t c;
	
  //get received char
  c = UDR;
	
  if( PC_TO_TOPOBO_BUFFER_REMAINING > 1 )
    pcToTopoboCircPutByte( c );
}

/*
 * This interrupt is called when a byte has finished
 * sending over the uart.
 */
SIGNAL(SIG_UART_DATA) //(SIG_UART_TRANS) 
{ 
  if( TOPOBO_TO_PC_BUFFER_USED > 0 )
    UDR =  topoboToPcCircGetByte();
	
  //disable int to avoid looping constantly when the tx buffer is empty
  else {
    disable_interrupt_TXC();
    uart_tx_complete = TRUE;
  }
}

void serialLoop(void) {
  uint8_t incoming_channel;
	
  enable_interrupt_TXC();
  enable_interrupt_RXC();
	
  while(1) 
    {
      // Check for incoming topobo communication.
      if ((incoming_channel = checkAllChannels()) != 0) {
	//uart_putchar('i');
	if (receiveMessage(incoming_channel) == 0) { // no error or timeout in recv
	  sendSerialMessage();
	} 
      }

      // Not sure if we need what is commented out here:

      //check if a message was received from topobo while sending to topobo
      //      if(num_bytes_in_rx_while_sending > 0)
      //	processReceivedWhileSendingMessage();
	    
      // now deal with incoming serial messages
      processSerialMessage();
	    
		
      //******************** timeouts ********************/

      if (interByteTimer > INTER_BYTE_TIMER_TIMEOUT ) {
	interByteTimer = 0;
	interByteTimeout = TRUE;
      }
		
      /*
       * this statement is true roughly 1
       * times/second. It will send a "serial gateway status message" to the
       * PC. Message type 7F must never be used by Topobo
       */
      /*		
      if (sendStatusToPC > SEND_STATUS_TO_PC_RELOAD) {
	sendStatusToPC = 0;
			
	topoboToPcCircPutByte(0xFF);
	topoboToPcCircPutByte(errorStatus);
	topoboToPcCircPutByte(PC_TO_TOPOBO_BUFFER_REMAINING);
			
	//if we're not already sending data to the pc,
	//send the first byte of the status message to get things rolling to the pc	
	disable_interrupt_TXC();
	UDR = ( topoboToPcCircGetByte() );
	enable_interrupt_TXC();
	errorStatus = 0;
      }
      */
    }
}

void loadSerialMsgBuff(uint8_t b) {
  if (num_bytes_in_serial_msg_buff < MESSAGE_BUFFER_SIZE) {
    serial_msg_buff[num_bytes_in_serial_msg_buff++] = b;
  }
}

void enable_interrupt_TXC(void) {
#ifdef IS_644
  UCSRB |= (_BV(UDRIE0)); //(1<<TXCIE);
#else
  UCSRB |= (_BV(UDRIE)); //(1<<TXCIE);
#endif
}

void disable_interrupt_TXC(void) {
#ifdef IS_644
  UCSRB &= ~(_BV(UDRIE0)); //(1<<TXCIE);
#else
  UCSRB &= ~(_BV(UDRIE)); //(1<<TXCIE);
#endif
}

void enable_interrupt_RXC(void) {
#ifdef IS_644
  UCSRB |= (1<<RXCIE0);
#else
  UCSRB |= (1<<RXCIE);
#endif
}

void disable_interrupt_RXC(void) {
#ifdef IS_644
  UCSRB &= ~(1<<RXCIE0);
#else
  UCSRB &= ~(1<<RXCIE);
#endif
}

uint8_t pcToTopoboCircGetByte(void) {
  uint8_t result;
  result = pcToTopoboCircBuffer[pcToTopoboCircBuffer_read];
  pcToTopoboCircBuffer_read = (pcToTopoboCircBuffer_read + 1) % PC_TO_TOPOBO_BUFFER_SIZE;
  return result;
}

void pcToTopoboCircPutByte(uint8_t datum) {
  pcToTopoboCircBuffer[pcToTopoboCircBuffer_write] = datum;
  pcToTopoboCircBuffer_write = (pcToTopoboCircBuffer_write + 1) % PC_TO_TOPOBO_BUFFER_SIZE;
}

uint8_t topoboToPcCircGetByte(void) {
  uint8_t result;
  result = topoboToPcCircBuffer[topoboToPcCircBuffer_read];
  topoboToPcCircBuffer_read = (topoboToPcCircBuffer_read + 1) % TOPOBO_TO_PC_BUFFER_SIZE;
  return result;
}

void topoboToPcCircPutByte(uint8_t datum) {
  topoboToPcCircBuffer[topoboToPcCircBuffer_write] = datum;
  topoboToPcCircBuffer_write = (topoboToPcCircBuffer_write + 1) % TOPOBO_TO_PC_BUFFER_SIZE;
}

/*
 * This function assumes that a message is incoming on the
 * topobo channel.
 */
void receiveTopoboMessage(void) {
}



/*
 * this function assumes that there is new data in the
 * serial message buffer that needs to be processed and
 * eventually sent to topobo
 
 * This code implements an error recovery pattern that is not
 * obvious from the code structure...
 *
 * First, the data coming from the computer will come in high-speed
 * packets (estimated 10x faster than the topobo data rate), but
 * the inter-packet lag should be such that the average data rate
 * from the computer equals the data rate into the topobo network.
 *
 * There are three types of errors that we anticipate:
 * 1) corrupted packet type;
 * 2) framing error; and
 * 3) computer failure in sending data to gateway (i.e. incomplete packet).
 *
 * POINT-A in the code below is the point at which error type-1 is
 * recognized.  At this point, we have lost framing information,
 * as it is virtually impossible to tell where the next packet will start.
 *
 * We presume error type-2 would eventually manifest itself as error type-1 or
 * error type-3, but if there are multiple packets in the pc-to-topobo circular
 * buffer, it will be difficult for a type-2 error to be recognized as a type-3 error.
 *
 * At POINT-A, we instaneously drain the pc-to-topobo circular buffer to position
 * the gateway toward rapid recovery.  If the computer is sending data to the gateway
 * while this happens, there will be a partial packet in the pc-to-topobo circular buffer
 * shortly after this point.  This IS a type-2 error, but it may appear as a type-1 error,
 * whereupon POINT-A is revisited, and the buffer is flushed again.  This second time, however,
 * the computer should be obeying the rule of inter-pack lag.  The gateway may even iterate through
 * a few attempts to read packets while the computer is still sending data, but the gateway should
 * eventually run into inter-packet lag.
 *
 * As such, all type-1 and type-2 errors should eventually manifest themselves as type-3 errors.
 *
 * At POINT-B, the gateway has read a partial packet and is waiting for more data from the computer.
 * Sometimes, the gateway may attempt to read a valid packet before the computer has
 * finished transmitting it, and so we allow for some delay before timing-out.  The
 * timeout should be greater than the inter-byte delay and significantly less than
 * the inter-packet lag.  Once we have reached a timeout at POINT-B, we have detected
 * a type-3 error, and so we flush the partially-read packet and await new data.
 * The next byte received may be reasonably expected to be the start of a new packet.
 */
void processSerialMessage(void) {
	
  // if transmit message buffer is empty, and data is available
  // begin sending a new msg to topobo. 
  if( ( num_bytes_in_tx_msg_buff == 0 ) && 
      ( PC_TO_TOPOBO_BUFFER_REMAINING < PC_TO_TOPOBO_BUFFER_SIZE ) ) {
    // Load first byte of packet:
    interByteTimeout = FALSE;
    receiving_serial_message = TRUE;
    loadTxMsgBuff( pcToTopoboCircGetByte() );
    interByteTimer = 0; //reset the timer
    //start and reload the inter-byte timer and clear any flags
    // Determine packet size
    serialMessageLength = tx_msg_buff[MESSAGE_LENGTH];
  }
	
	
  // but if there's no pending message to finish, nor new serial data to fetch
  if( num_bytes_in_tx_msg_buff == 0 ) 
    return;
	
  // A message is started. Finish it if there's new data from the UART.
  // Feed transmit message buffer with data until circular buffer is empty or packet is complete
  while( ( PC_TO_TOPOBO_BUFFER_REMAINING < PC_TO_TOPOBO_BUFFER_SIZE ) && 
	 ( num_bytes_in_tx_msg_buff < serialMessageLength ) ) {
    loadTxMsgBuff( pcToTopoboCircGetByte() );
		
    //reload the inter-byte timer
    interByteTimer = 0; // reset the timer
  }
	
  //test the new message to see if it's all there
  //or if there was an error.
  if (serialMessageLength == 0) { 
    // i.e. THE MSG. ID WAS NOT UNDERSTOOD. there was a framing error, or a bad message was sent
    // POINT-A (see comments at start of function)
		
    // Flush the pc-to-topobo buffers
    num_bytes_in_tx_msg_buff = 0; //flush the buffer
    pcToTopoboCircBuffer_read = pcToTopoboCircBuffer_write = 0;
    receiving_serial_message = FALSE;
		
    // tell PC there was a framing error
    errorStatus |= 0x01;
  }
	
  //if the whole message is loaded, send it to topobo
  //and flush the buffer
  if ((serialMessageLength != 0) && 
      (num_bytes_in_tx_msg_buff == serialMessageLength)) {
    sendMessage(ALL_GOOD_CHANNELS); //this could take a while to complete
    serialMessageLength = 0;
    receiving_serial_message = FALSE;
  } 
  else {
    // POINT-B : the program will get here if a message is partially received from the PC
		
    //see if the inter-byte timer is expired
    //if it is, flush buffers and report error
    if (interByteTimeout == TRUE) {
      //putnum_uh(0xAA); //debugging...
      // Flush the pc-to-topobo buffers
      num_bytes_in_tx_msg_buff = 0; //flush the buffer
      pcToTopoboCircBuffer_read = pcToTopoboCircBuffer_write = 0;
      receiving_serial_message = FALSE;
      errorStatus |= 0x02; //incomplete packet
    }
  }
}

void sendSerialMessage(void){
  int i;
  //copy bytes to circular buffer
  for (i=0; i < num_bytes_in_rx_msg_buff; i++) {
    topoboToPcCircPutByte(rx_msg_buff[i]);
  }
  //clear the linear buffer
  num_bytes_in_rx_msg_buff = 0;
	
  /* if we're not already sending something, prime the pump
   * by sending one byte out. The SIGNAL(SIG_UART_TRANS) will
   * complete sending the entire buffer.
   */
  disable_interrupt_TXC();
  UDR = topoboToPcCircGetByte();
  enable_interrupt_TXC();
	
}
