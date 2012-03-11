#include "irq.h"
#include "process.h"
#include "iprocess.h"
#include <LPC17xx.h>


void irq_handler(irq_type type) {

	pCurrentProcessPCB->currentState = INTERRUPTED;

	switch (type) {
		case TIMER_IRQ:
			context_switch(pCurrentProcessPCB, get_timer_pcb());
			// TODO: Seems like if a timer interrupt fires and a process gets preempted (because a
			// delayed_send fired and a higher priority process preempts, then if ever we try to print,
			// the uart process messes up if we put k_release_processor at the end of irq_handler.
			// We should fix this if it is still a problem. 

			break;
		case UART0_IRQ:
			context_switch(pCurrentProcessPCB, get_uart_pcb()); 
			break;
		default:
			break;
		
	}
	k_release_processor();

}


