#include "rtx.h"
#include "uart.h"
#include "system_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"

//  These are dummy variables so this code will compile.  This code is nowhere near finished

extern uint8_t g_UART0_TX_empty_placeholder=1;

void keyboard_command_decoder(void){
 	while(1){
		int * sender_id = 0;
		void * message = k_receive_message(sender_id);


		release_memory_block(message);		
	}
}

void crt_display(void){
 	while(1){
		int * sender_id = 0;
		void * message = k_receive_message(sender_id);

		LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
		// Set our pointer to the first character in the message to output.
		uint8_t * current_character = message;

	    while ( *current_character != 0 ) {
		    // THRE status, contain valid data  
		    while ( !(g_UART0_TX_empty_placeholder & 0x01) );
			//  Setting this value makes the interrupt fire at a later time	
		    pUart->THR = *current_character;
			g_UART0_TX_empty_placeholder = 0;  // not empty in the THR until it shifts out
		    current_character++;
	    }
		
		release_memory_block(message);		
	}
}
