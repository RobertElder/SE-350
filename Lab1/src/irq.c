#include "irq.h"
#include "process.h"
#include "iprocess.h"
#include "utils.h"
#include <LPC17xx.h>


void irq_handler(irq_type type) {

	assert(pCurrentProcessPCB->currentState != NEW,"A new process has been interrupted.");

	pCurrentProcessPCB->currentState = INTERRUPTED;

	switch (type) {
		case TIMER_IRQ:
			context_switch(pCurrentProcessPCB, get_timer_pcb());
			break;
		case UART0_IRQ:
			context_switch(pCurrentProcessPCB, get_uart_pcb()); 
			break;
		default:
			break;
		
	}

	k_release_processor();
}


