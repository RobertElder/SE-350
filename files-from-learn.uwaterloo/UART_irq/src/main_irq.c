/**
 * @brief:  main.c
 * @author: NXP Semiconductors
 * @date: 2012/01/20
 * NOTE: modified by Y. Huang
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart.h"

extern volatile uint8_t g_UART0_TX_empty; 
extern volatile uint8_t g_UART0_buffer[BUFSIZE];
extern volatile uint32_t g_UART0_count;

int main()
{
	SystemInit();	// initialize the system
	__disable_irq();
	uart0_init();
	__enable_irq();

	while(1) {
	    if ( g_UART0_count != 0 ) {
	        LPC_UART0->IER = IER_THRE | IER_RLS;			// Disable RBR 
	        uart_send_string( 0, (uint8_t *)g_UART0_buffer, g_UART0_count );
	        g_UART0_count = 0;
	        LPC_UART0->IER = IER_THRE | IER_RLS | IER_RBR;	// Re-enable RBR
	    }
	}	
}

