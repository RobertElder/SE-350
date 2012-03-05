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
#include "uart.h"
#include "timer.h"
#include "rtx.h"
#include "utils.h" 
#include "usr_proc.h"


extern void process_init(void);

void print_some_numbers(){
	int i;

	for(i = 0; i < 10; i++){
		print_unsigned_integer(get_random() % 200);
		uart0_put_string("\n\r");
	}

	for(i = 0; i < 10; i++){
		print_signed_integer(((signed int)(get_random() % 200)) * -1);
		uart0_put_string("\n\r");
	}

	uart0_put_string("\n\rPrint stuff test passed. \n\r");
}


extern volatile uint32_t g_timer_count;

int main(){
	volatile unsigned int ret_val = 1234;
	SystemInit();	// initialize the system

	// Disable interrupt requests
	__disable_irq();
	/* Lower the priority of the svn calls so that 
	 we can handle UART interrupts inside SVC calls */
	NVIC_SetPriority(SVCall_IRQn, 1);
	// Initialize UART output
	timer_init(0);
	uart0_init();   
	// Initialize stack and PCB for processes
	process_init();
	// Enable interrupt requests
	__enable_irq();
	
	// transit to unprivileged level, default MSP is used
	__set_CONTROL(__get_CONTROL() | BIT(0));  

	//  Set up memory
	init_memory_allocation_table();
	
//    while (1) {
//        if (g_timer_count % 10 == 0) {
//			print_unsigned_integer(g_timer_count / 10);
//		    uart0_put_string("\r\n");
//		}     
//	}

	ret_val = release_processor();
	uart0_put_string("\nShould never reach here!!!\n\r");
	
	return -1;	
}
