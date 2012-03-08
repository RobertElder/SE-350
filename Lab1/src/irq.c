#include "irq.h"
#include "process.h"
#include "iprocess.h"

void irq_handler(irq_type type) {
	__disable_irq();
	pCurrentProcessPCB->currentState = INTERRUPTED;
	switch (type) {
		case TIMER_IRQ:
		    context_switch(pCurrentProcessPCB, get_timer_pcb());
			break;
		default:
			break;
	}
	__enable_irq();

}
