#include "irq.h"
#include "process.h"
#include "iprocess.h"
#include <LPC17xx.h>


void irq_handler(irq_type type) {

	pCurrentProcessPCB->currentState = INTERRUPTED;

	switch (type) {
		case TIMER_IRQ:
			context_switch(pCurrentProcessPCB, get_timer_pcb());
			break;
		case UART0_IRQ:
			break;
		default:
			break;
	}
	k_release_processor();
	//return_from_interrupt();
	//rte();
	//__enable_irq();

}


