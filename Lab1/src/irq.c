#include "irq.h"
#include "process.h"
#include "iprocess.h"
#include <LPC17xx.h>

__asm void __iproc_switch(void) {
	BL __cpp(timeTEMP)	
}
__asm void return_from_interrupt(void) {
	PRESERVE8
	BL 	 __cpp(k_release_processor) 
	MVN  LR, #:NOT:0xFFFFFFF9
	BX   LR
}
void irq_handler(irq_type type) {
	//__disable_irq();
	pCurrentProcessPCB->currentState = INTERRUPTED;
	pCurrentProcessPCB->processStackPointer = (uint32_t *) __get_MSP();

	switch (type) {
		case TIMER_IRQ:
			__set_MSP((uint32_t) get_timer_pcb()->processStackPointer);
			timeTEMP();
			__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
			break;
		default:
			break;
	}
	pCurrentProcessPCB->currentState = RUN;
	//return_from_interrupt();
	//rte();
	//__enable_irq();

}


