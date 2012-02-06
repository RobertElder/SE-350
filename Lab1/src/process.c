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
#include "utils.h"
#include "process.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

/* Variable definitions */
ProcessControlBlock * pCurrentProcessPCB  = NULL;

uint32_t stack0[USR_SZ_STACK];      // stack for nullProc
uint32_t stack1[USR_SZ_STACK];      // stack for proc1
uint32_t stack2[USR_SZ_STACK];	    // stack for proc2
uint32_t stack3[USR_SZ_STACK];      // stack for run_priority_tests
uint32_t stack4[USR_SZ_STACK];      // stack for run_memory_tests

ProcessControlBlock process_array[NUM_PROCESSES];

int set_process_priority (int process_ID, int priority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in set process priority.");

   	if(priority >= 0 && priority < 4) {
		process->processPriority = priority;
		return 0;
	} else {
	 	assert(process != NULL, "Invalid priority in 'set_process_priority'");
	}
}

int get_process_priority (int process_ID) {
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in get process priority.");

	return process->processPriority;
}

ProcessControlBlock * get_process_pointer_from_id(int process_ID) {
	return (process_ID > NUM_PROCESSES - 1) ? NULL : &process_array[process_ID];
}

int is_process_blocked(int processId){
	//  Right now this is the only blocking state
	return (process_array[processId].currentState == BLOCKED_ON_MEMORY);
}

int is_deadlocked(){
	int i;
	//  We don't need to check the null process, pid 0
	for(i = 1; i < NUM_PROCESSES; i++){
		if(!(is_process_blocked(i))){
			return 0;
		}
	}
	//  Everything must have been blocked
	return 1;
}

/**
 * @brief: initialize all processes in the system

 *       TODO: We should have used an array or linked list pcbs so the repetive coding
 *       can be eliminated.

 *       TODO: We should have also used an initialization table which contains the entry
 *       points of each process so that this code does not have to hard code
 *       proc1 and proc2 symbols. We leave all these imperfections as excercies to the reader 
 */
void process_init() 
{
    volatile int i;
	uint32_t * sp;
	int procIndex;

	//  For all the processes
	for(procIndex = 0; procIndex < NUM_PROCESSES; ++procIndex) {
		ProcessControlBlock process;

		//  Set up the process control block
		process.processId = procIndex;
		process.currentState = NEW;
		process.processPriority = (procIndex == 0) ? 4 : (procIndex - 1);

		//  Set up the process stacks based on the index of the process
		switch(procIndex) {
		 	case 0:
				sp  = stack0 + USR_SZ_STACK;
				break;
			case 1:
				sp  = stack1 + USR_SZ_STACK;
				break;
			case 2:
				sp  = stack2 + USR_SZ_STACK;
				break;	
			case 3:
				sp  = stack3 + USR_SZ_STACK;
				break;				
			case 4:
				sp  = stack4 + USR_SZ_STACK;
				break;
			default:
				sp  = stack0 + USR_SZ_STACK;
				break;
		}

		// 8 bytes alignement adjustment to exception stack frame
		// TODO: figure out why we want sp to have 4 right-aligned non-zero bits before 
		// decrementing it.
		if (!(((uint32_t)sp) & 0x04)) {
		    --sp; 
		}

		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  

		// Set the entry point of the process
		switch(procIndex) {
		 	case 0:
				*(--sp)  = (uint32_t) nullProc;
				break;
			case 1:
				*(--sp)  = (uint32_t) run_memory_tests;
				break;
			case 2:
				*(--sp)  = (uint32_t) run_priority_tests;
				break;	
			case 3:
				*(--sp)  = (uint32_t) run_memory_tests;
				break;
			case 4:
				*(--sp)  = (uint32_t) run_priority_tests;
				break;						
			default:
				assert(0, "ProcIndex case not handled in process_init");
				break;
		}

		for (i = 0; i < 6; i++) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}

		process.processStackPointer = sp;

		process_array[procIndex] = process;


	}

	//  To start off, set the current process to the null process
	pCurrentProcessPCB = &process_array[0];
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: pid of the next to run process
 *         selects null process by default
 *POST: if pCurrentProcessPCB was NULL, then it gets set to &pcb1.
 *      No other effect on other global variables.
 */
int scheduler(void)
{
    volatile int current_pid;
	volatile int pid_to_select;
	volatile int highest_priority;

	assert((int)pCurrentProcessPCB,"There was no current process set in the scheduler.");

	current_pid = pCurrentProcessPCB->processId;
	highest_priority = 4;	

	//  This will cycle through the list of processes then repeat
	return (pCurrentProcessPCB->processId < (NUM_PROCESSES - 1)) ? pCurrentProcessPCB->processId + 1 : 0;	
}
/**
 * @brief release_processor(). 
 * @return -1 on error and zero on success
 * POST: pCurrentProcessPCB gets updated
 */
int k_release_processor(void){
	volatile int idOfNextProcessToRun;
	volatile proc_state_t state;
	ProcessControlBlock * pOldProcessPCB = NULL;
	
	idOfNextProcessToRun = scheduler();

	if (pCurrentProcessPCB == NULL) {
		// error occured (scheduler should null-check)
		assert((int)pCurrentProcessPCB,"pCurrentProcessPCB was null.");
		return -1;  
	}
	
	//  Screw this process, we are getting a newer BETTER process
	pOldProcessPCB = pCurrentProcessPCB;

	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()),"Deadlock:  All processes are in blocked state.");	

	//  Attempt to get a process that is not blocked
	do{
		idOfNextProcessToRun = scheduler();
		pCurrentProcessPCB = get_process_pointer_from_id(idOfNextProcessToRun);
	}while(is_process_blocked(pCurrentProcessPCB->processId));
	
	if(pCurrentProcessPCB == NULL) {
		assert((int)pCurrentProcessPCB,"pCurrentProcessPCB was null after calling get process pointer from id.");
		return -1;
	}
	
	/*
	__set_MSP() and __get_MSP() are special arm functions that are documented here
	http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/CIHCAEJD.html
	*/

	switch(pCurrentProcessPCB->currentState) {
		case NEW:{
			if (pOldProcessPCB->currentState != NEW && pOldProcessPCB->currentState != BLOCKED_ON_MEMORY) {
				//  It was in the new state, but now it is fine to run again
				pOldProcessPCB->currentState = RDY;
				//  Save the stack pointer for the old process
				pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
			}
			//  New process is now runing
			pCurrentProcessPCB->currentState = RUN;
			//  Update the stack pointer for the new current process
			__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
			// pop exception stack frame from the stack for new processes (assembly function in hal.c)
			__rte();
			break;
		}
		case RDY: {
			if(pOldProcessPCB->currentState != BLOCKED_ON_MEMORY)    
				pOldProcessPCB->currentState = RDY;

			//  Save the stack pointer for the old process 
			pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP(); // save the old process's sp
			
			pCurrentProcessPCB->currentState = RUN;
			__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer); //switch to the new proc's stack
			break;
		}		
		default: {
			pCurrentProcessPCB = pOldProcessPCB; // revert back to the old proc on error
			assert(0,"Reverting back to old process, no state matched.");
			return -1;
		}
	}	 	 
	return 0;
}
