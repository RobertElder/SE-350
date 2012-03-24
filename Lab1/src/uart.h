

#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>	// typedefs

// The following macros are from NXP uart.h

// This stuff is found in lpc17xx_um_rev2.pdf page 300ish pages/840
/*
	IER - Interrupt Enable Register. Contains individual interrupt enable bits for the 7 potential UART interrupts.
	IIR - Interrupt ID Register. Identifies which interrupt(s) are pending.
	LSR - Line Status Register. Contains flags for transmit and receive status, including line errors.
*/

#define IER_Receive_Data_Available		0x01	 /* Enables the Receive Data Available interrupt for UARTn. 
It also controls the Character Receive Time-out interrupt. 
0 Disable the RDA interrupts. 1 Enable the RDA interrupts.
*/

#define IER_THR_Empty	0x02	 /* Enables the Transmitter	Holding Register
Empty interrupt for UARTn. The status of this can be read from UnLSR[5].*/
#define IER_Receive_Line_Status		0x04	 // Enables the UARTn RX line status interrupts. The status of this interrupt can be read from UnLSR[4:1].

/*  THIS ONE IS NOT EVEN USED???
#define IIR_Pending_Interrupt	0x01	  IntStatus Interrupt status. Note that UnIIR[0] is active low. The pending interrupt 
can be determined by evaluating UnIIR[3:1]. Reset Value: 1 0 At least one interrupt is pending.
1 No interrupt is pending.	   */

#define IIR_Receive_Line_Status		0x03 /*Interrupt identification. UnIER[3:1] identifies an interrupt corresponding to the
UARTn Rx or TX FIFO. All other combinations of UnIER[3:1] not listed below
are reserved (000,100,101,111).
Reset value: 0
011 1 - Receive Line Status (RLS).
010 2a - Receive Data Available (RDA).
110 2b - Character Time-out Indicator (CTI).
001 3 - THRE Interrupt*/


#define IIR_Receive_Data_Available		0x02 /*Interrupt identification. UnIER[3:1] identifies an interrupt corresponding to the
UARTn Rx or TX FIFO. All other combinations of UnIER[3:1] not listed below
are reserved (000,100,101,111).
010 2a - Receive Data Available (RDA).*/
#define IIR_Character_Timeout_Indicator		0x06 /*Interrupt identification. UnIER[3:1] identifies an interrupt corresponding to the
UARTn Rx or TX FIFO. All other combinations of UnIER[3:1] not listed below
are reserved (000,100,101,111).
110 2b - Character Time-out Indicator (CTI).*/
#define IIR_THR_Empty	0x01 /*Interrupt identification. UnIER[3:1] identifies an interrupt corresponding to the
UARTn Rx or TX FIFO. All other combinations of UnIER[3:1] not listed below
are reserved (000,100,101,111).
001 3 - THRE Interrupt*/

#define LSR_Unread_Character		0x01  /*UnLSR0 is set when the UnRBR holds an unread character and is cleared when
the UARTn RBR FIFO is empty.*/
#define LSR_OE		0x02  /*The overrun error condition is set as soon as it occurs. An UnLSR read clears
UnLSR1. UnLSR1 is set when UARTn RSR has a new character assembled and
the UARTn RBR FIFO is full. In this case, the UARTn RBR FIFO will not be
overwritten and the character in the UARTn RSR will be lost.*/
#define LSR_PE		0x04  /*When the parity bit of a received character is in the wrong state, a parity error
occurs. An UnLSR read clears UnLSR[2]. Time of parity error detection is
dependent on UnFCR[0].*/
#define LSR_FE		0x08  /*When the stop bit of a received character is a logic 0, a framing error occurs. An
UnLSR read clears UnLSR[3]. The time of the framing error detection is
dependent on UnFCR0. Upon detection of a framing error, the Rx will attempt to
resynchronize to the data and assume that the bad stop bit is actually an early
start bit. However, it cannot be assumed that the next received byte will be correct
even if there is no Framing Error.
Note: A framing error is associated with the character at the top of the UARTn
RBR FIFO.*/
#define LSR_BI		0x10 /*When RXDn is held in the spacing state (all zeroes) for one full character
transmission (start, data, parity, stop), a break interrupt occurs. Once the break
condition has been detected, the receiver goes idle until RXDn goes to marking
state (all ones). An UnLSR read clears this status bit. The time of break detection
is dependent on UnFCR[0].
Note: The break interrupt is associated with the character at the top of the UARTn
RBR FIFO.*/
#define LSR_THR_Empty	0x20 /*THRE is set immediately upon detection of an empty UARTn THR and is cleared
on a UnTHR write.
Reset value 1
0 UnTHR contains valid data.
1 UnTHR is empty.*/
#define LSR_TEMT	0x40 /*TEMT is set when both UnTHR and UnTSR are empty; TEMT is cleared when
either the UnTSR or the UnTHR contain valid data.
Reset value 1
0 UnTHR and/or the UnTSR contains valid data.
1 UnTHR and the UnTSR are empty.*/
#define LSR_RXFE	0x80/*UnLSR[7] is set when a character with a Rx error such as framing error, parity
error or break interrupt, is loaded into the UnRBR. This bit is cleared when the
UnLSR register is read and there are no subsequent errors in the UARTn FIFO.
Reset value 0
0 UnRBR contains no UARTn RX errors or UnFCR[0]=0.
1 UARTn RBR contains at least one UARTn RX error.*/

#define BUFSIZE		0x40
// end of NXP uart.h file reference



#define BIT(X)    ( 1<<X )
#define UART_8N1  0x83	  // 8 bits, no Parity, 1 Stop bit
						  // 0x83 = 1000 0011 = 1 0 00 0 0 11
						  // LCR[7]  =1  enable Divisor Latch Access Bit DLAB
						  // LCR[6]  =0  disable break transmission
						  // LCR[5:4]=00 odd parity
						  // LCR[3]  =0  no parity
						  // LCR[2]  =0  1 stop bit
						  // LCR[1:0]=11 8-bit char len
						  // See table 279, pg306 LPC17xx_UM


#define uart0_init()        uart_init(0)	
     

int uart_init(int n_uart);		// initialize the n_uart
void uart0_put_string(char *);		 
                                
void execute_uart(void);

volatile extern uint8_t g_UART0_TX_empty;

#endif // ! _UART_H_ 
