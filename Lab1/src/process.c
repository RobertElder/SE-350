/**
 * @brief: process.h  process management source file
 * @author Irene Huang
 * @author Thomas Reidemeister
 * @date 2011/01/18
 * NOTE: The example code shows one way of context switching implmentation.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes, NO external interrupts. 
 *  	 The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. Some part of the code is not written in the most efficient way.
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "process.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

int set_process_priority (int process_ID, int priority) {	
	pcb_t * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID");

   	if(priority >= 0 && priority < 4) {
		process->m_priority = priority;
		return 0;
	} else {
	 	return -1;
	}
}

int get_process_priority (int process_ID) {
	pcb_t * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID");

	return process->m_priority;
}

pcb_t * get_process_pointer_from_id(int process_ID) {
	switch(process_ID) {
	 	case 1:
			return &pcb1;
		case 2:
			return &pcb2;
		default:
			return NULL;	
	}
}

/**
 * 


 * NOTE: We assume there are only two user processes in the system in this example.
 *       We should have used an array or linked list pcbs so the repetive coding
 *       can be eliminated.
 *       We should have also used an initialization table which contains the entry
 *       points of each process so that this code does not have to hard code
 *       proc1 and proc2 symbols. We leave all these imperfections as excercies to the reader 
 */
void process_init() 
{
    volatile int i;
	uint32_t * sp;

	// initialize the first process	exception stack frame
	pcb1.m_pid = 1;
	pcb1.m_state = NEW;
	pcb1.m_priority = 0;

	sp  = stack1 + USR_SZ_STACK;
    
	// 8 bytes alignement adjustment to exception stack frame
	if (!(((uint32_t)sp) & 0x04)) {
	    --sp; 
	}
											  
	*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
	*(--sp)  = (uint32_t) proc1; // PC contains the entry point of the process

	for (i=0; i<6; i++) { // R0-R3, R12 are cleared with 0
		*(--sp) = 0x0;
	}
    pcb1.mp_sp = sp;


	// initialize the second process exception stack frame
    pcb2.m_pid = 2;
	pcb2.m_state = NEW;
	pcb2.m_priority = 1;

	sp  = stack2 + USR_SZ_STACK;
    if (!(((uint32_t)sp) & 0x04)) {    
	    sp--;  // 8 bytes alignement adjustment to exception stack frame
	}
	
	*(--sp) = INITIAL_xPSR;
	*(--sp) = (uint32_t) proc2;

	for (i=0; i<6; i++) {
		*(--sp) = 0x0;
	}
	pcb2.mp_sp = sp;

	// initialize the null process	exception stack frame
	pcb0.m_pid = 0;
	pcb0.m_state = NEW;
	pcb0.m_priority = 4;

	sp  = stack0 + USR_SZ_STACK;
    
	// 8 bytes alignement adjustment to exception stack frame
	if (!(((uint32_t)sp) & 0x04)) {
	    --sp; 
	}
	
	*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
	*(--sp)  = (uint32_t) nullProc; // PC contains the entry point of the process

	for (i=0; i<6; i++) { // R0-R3, R12 are cleared with 0
		*(--sp) = 0x0;
	}
    pcb0.mp_sp = sp;
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: pid of the next to run process
 *         selects null process by default
 *POST: if gp_current_process was NULL, then it gets set to &pcb1.
 *      No other effect on other global variables.
 */
int scheduler(void)
{
    volatile int pid;

	if (gp_current_process == NULL) {
	   gp_current_process = &pcb1;
	   return 1;
	}

	pid = gp_current_process->m_pid;
	
	if (pid == 1 ) {
	    return 2;
	} else if (pid == 2) {
		return 1;
	} else {
		return 0; // null process
	}
}
/**
 * @brief release_processor(). 
 * @return -1 on error and zero on success
 * POST: gp_current_process gets updated
 */
int k_release_processor(void)
{
	 volatile int pid;
	 volatile proc_state_t state;
	 pcb_t * p_pcb_old = NULL;

	 pid = scheduler();
	 if (gp_current_process == NULL) {
	     return -1;  
	 }

	 p_pcb_old = gp_current_process;


	gp_current_process = get_process_pointer_from_id(pid);

	if(gp_current_process == NULL) {
	 	return -1;
	}

	 state = gp_current_process->m_state;

     if (state == NEW) {
	     if (p_pcb_old->m_state != NEW) {
		     p_pcb_old->m_state = RDY;
			 p_pcb_old->mp_sp = (uint32_t *) __get_MSP();
		 }
		 gp_current_process->m_state = RUN;
		 __set_MSP((uint32_t) gp_current_process->mp_sp);
		 __rte();  // pop exception stack frame from the stack for new processes
	 } else if (state == RDY){     
		 p_pcb_old->m_state = RDY; 
		 p_pcb_old->mp_sp = (uint32_t *) __get_MSP(); // save the old process's sp
		 
		 gp_current_process->m_state = RUN;
		 __set_MSP((uint32_t) gp_current_process->mp_sp); //switch to the new proc's stack		
	 } else {
	     gp_current_process = p_pcb_old; // revert back to the old proc on error
	     return -1;
	 }	 	 
	 return 0;
}
