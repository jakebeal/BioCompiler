/* 
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

/**********************************************************

16 Feb 2006, ported by hayes raffle. 

! low level uart interrupts are not fixed yet.


this gateway translates rs232 data to topobo prototcol and vice versa.
it works based on timing, and the pc has to send data at acceptable rates, calculated by these formulas:

The gateway:

   - is smart enough to throw away bad message types and tries to resync with the message stream.
   - can recover from some kinds of framing errors.
   - will send a steady stream of status data to the PC, informing the PC about its internal
     buffers and any errors it might have received. the status messages are 3 bytes long:

      [message_ID 0xFF][PC_TO_TOPOBO_BUFFER_REMAINING][error byte]

      error byte is a bitwise mask where
        0x01 = framing error and
        0x02 = incomplete packet

       if either of these errors is received, the entire last packet should be resent.

packet timing from PC:

   11.7 ms silence in between packets of up to 16 bytes
   6.5 ms inter-byte timeout. Make sure multi-byte messages don't have bytes spaced out more than this.
   69.6 us absolute minimum time required to read in a byte over the PIC's USART - useful for establishing inter-byte timeout.

Math for determining the packet timing:

    T_rate and C_rate are the topobo and computer data rates, respectively.
    P is a reasonable maximum number of bits per packet.
    T_time is the time spent transmitting a packet over topobo.
    C_time is the time spent transmitting a packet over serial.
    S is the minimum average time the computer must be silent after transmitting a packet.

    T_rate = 10000 [bits/sec]
    C_rate = 115000 [bits/sec]

    P = 16 [bytes] * 8 [bits/byte]

    T_time = P / T_rate
    C_time = P / C_rate

    P/(C_time + S) = T_rate

    C_time + S = P/T_rate
    S = P/T_rate - C_time
    S = P/T_rate - P/C_rate
    S = P * ( (1/T_rate) - (1/C_rate) )

    S = 16*8*((1/10000)-(1/115000))= 0.0117 seconds = 11.7 ms

    Minimum inter-byte time: 0.0696 ms = 69.6 us

**********************************************************/

#ifdef IS_644
#define SIG_UART_RECV USART0_RX_vect
#define SIG_UART_DATA USART0_UDRE_vect
#endif

#define TOPOBO_CHANNEL 0X01
#define SERIAL_MESSAGE_BUFFER_SIZE 100
#define SEND_STATUS_TO_PC_RELOAD 3000UL // in ms. 
#define INTER_BYTE_TIMER_TIMEOUT 100UL //in ms. needs to be between 1 and 11 ms

// Declare I/O buffers
#define PC_TO_TOPOBO_BUFFER_SIZE	128
#define TOPOBO_TO_PC_BUFFER_SIZE	128

// Declare circular buffer manipulation routines
#define PC_TO_TOPOBO_BUFFER_USED	((PC_TO_TOPOBO_BUFFER_SIZE + pcToTopoboCircBuffer_write - pcToTopoboCircBuffer_read) % PC_TO_TOPOBO_BUFFER_SIZE)
#define TOPOBO_TO_PC_BUFFER_USED	((TOPOBO_TO_PC_BUFFER_SIZE + topoboToPcCircBuffer_write - topoboToPcCircBuffer_read) % TOPOBO_TO_PC_BUFFER_SIZE)
#define PC_TO_TOPOBO_BUFFER_REMAINING	(PC_TO_TOPOBO_BUFFER_SIZE - PC_TO_TOPOBO_BUFFER_USED)
#define TOPOBO_TO_PC_BUFFER_REMAINING	(TOPOBO_TO_PC_BUFFER_SIZE - TOPOBO_TO_PC_BUFFER_USED)


void loadSerialMsgBuff(uint8_t b);

void enable_interrupt_TXC(void);
void disable_interrupt_TXC(void);
void enable_interrupt_RXC(void);
void disable_interrupt_RXC(void);

uint8_t pcToTopoboCircGetByte(void);
uint8_t topoboToPcCircGetByte(void);
void topoboToPcCircPutByte(uint8_t datum);
void pcToTopoboCircPutByte(uint8_t datum);

void processSerialMessage(void);
void sendSerialMessage(void);
void receiveTopoboMessage(void);
void serialLoop(void);

