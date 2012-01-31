/**
 * @brief:  main_svc.c, demonstrate svc as a gateway to os functions
 * @author: Yiqing Huang
 * @date: 2012/01/08
 * NOTE: standard C library is not allowed in the final kernel code
 *       it is OK if you only use the C library for debugging purpose 
 *       during the development/prototyping phase
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#ifdef DEBUG_0
#include <stdio.h>
#else
#define NULL 0
#endif // DEBUG_0
#include "uart_polling.h"

#include "rtx.h"

extern void process_init(void);

int main() 
{
	 
	volatile unsigned int ret_val = 1234;
	

	SystemInit();	// initialize the system
	__disable_irq();
	uart0_init();   
	process_init();
	__enable_irq();
	
	// transit to unprivileged level, default MSP is used
	__set_CONTROL(__get_CONTROL() | BIT(0));  

	ret_val = release_processor();

	// should never reach here!!!
	return -1;	
}
