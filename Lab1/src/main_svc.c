
#include <LPC17xx.h>
#include <system_LPC17xx.h>
#ifdef DEBUG_0
#include <stdio.h>
#else
#define NULL 0
#endif // DEBUG_0
#include "process.h"
#include "uart.h"
#include "uart_polling.h"
#include "timer.h"
#include "memory.h"
#include "utils.h" 
#include "usr_proc.h"
#include "iprocess.h"
#include "system_proc.h"

//extern void process_init(void);

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

	// Initialize UART output
	timer_init(0);
	uart0_init(); 
	uart0_polling_init();  
	// Initialize stack and PCB for processes
	process_init();
	init_i_processes();

	//  Set up memory
	init_memory_allocation_table();

	// Enable interrupt requests
	__enable_irq();
	
	// transit to unprivileged level, default MSP is used
	__set_CONTROL(__get_CONTROL() | BIT(0));  

	ret_val = release_processor();
	uart0_put_string("\nShould never reach here!!!\n\r");
	
	return -1;	
}
