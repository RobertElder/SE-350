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
int isMemBlockJustReleased = 0;

ProcessControlBlock pcb_array[NUM_PROCESSES];

extern unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;

init_t proc_init_table[NUM_PROCESSES];

int set_process_priority (int process_ID, int priority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in set process priority.");

   	if(priority >= 0 && priority < 4) {
		process->processPriority = priority;
		return 0;
	} else {
	 	return -1;
	}
}

int get_process_priority (int process_ID) {
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in get process priority.");

	return process->processPriority;
}

ProcessControlBlock * get_process_pointer_from_id(int process_ID) {
	return (process_ID > NUM_PROCESSES - 1) ? NULL : &pcb_array[process_ID];
}

int is_process_blocked(int processId){
	//  Right now this is the only blocking state
	return (pcb_array[processId].currentState == BLOCKED_ON_MEMORY);
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

int has_blocked_processes(){
	int i;
	//  We don't need to check the null process, pid 0
	for(i = 1; i < NUM_PROCESSES; i++){
		if(is_process_blocked(i)){
			return 1;
		}
	}
	//  nothing is blocked
	return 0;
}

	 // We will have 5 priorities --> 0,1,2,3  are for normal processes; 4 is for the NULL process
void* k_init_processes_to_create() {
	unsigned int sp = free_mem;
   	int i;

	for( i = 0; i < 6; i++ ){
		init_t proc;

		proc.pid = i;
		proc.priority = (i == 0) ? 4 : (i - 1) % 4;
		proc.stack_size = USR_SZ_STACK;
		sp += USR_SZ_STACK;
		proc.start_sp = sp;
		

		proc_init_table[i] = proc;
	}
	 return 0;
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

	k_init_processes_to_create();

	//  For all the processes
	for (procIndex = 0; procIndex < NUM_PROCESSES; ++procIndex) {
		ProcessControlBlock process;

		//  Set up the process control block
		process.processId = proc_init_table[procIndex].pid;
		process.currentState = NEW;
		// must mod 4 so that the priorities don't go over 3 (only null process can be level 4)
		process.processPriority =  proc_init_table[procIndex].priority;
		
		sp  = (uint32_t*)proc_init_table[procIndex].start_sp;

		// 8 bytes alignement adjustment to exception stack frame
		// TODO: figure out why we want sp to have 4 right-aligned non-zero bits before 
		// decrementing it.
		if (!(((uint32_t)sp) & 0x04)) {
		    --sp; 
		}

		*(--sp) = INITIAL_xPSR;      // user process initial xPSR  

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
				*(--sp)  = (uint32_t) run_block_memory_test;
				break;
			case 4:
				*(--sp)  = (uint32_t) memory_request_process;
				break;						
			default:
				*(--sp)  = (uint32_t) nullProc;
				break;
		}

		for (i = 0; i < 6; i++) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}

		process.processStackPointer = sp;

		pcb_array[procIndex] = process;


	}

	//  To start off, set the current process to the null process
	pCurrentProcessPCB = &pcb_array[0];
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
	volatile int highest_priority_process = 0;
	int j;

	assert((int)pCurrentProcessPCB, "There was no current process set in the scheduler.");

	current_pid = pCurrentProcessPCB->processId;
		

	if (isMemBlockJustReleased) {

		// get next process
		j = (pCurrentProcessPCB->processId < (NUM_PROCESSES - 1)) ? pCurrentProcessPCB->processId + 1 : 0;

		// Find highest priority blocked process
		while (pCurrentProcessPCB->processId != j) {
			if(pcb_array[j].currentState == BLOCKED_ON_MEMORY && pcb_array[j].processPriority < pcb_array[highest_priority_process].processPriority ) {
				highest_priority_process = j;
			}
			j = (j == NUM_PROCESSES - 1) ? 0 : j + 1;
		}

		pcb_array[highest_priority_process].currentState = RDY;

		isMemBlockJustReleased = 0;

		return highest_priority_process;
	} else {
		j =  pCurrentProcessPCB->processId;
	 	do {
			j = (j < (NUM_PROCESSES - 1)) ? j + 1 : 0;
		} while(is_process_blocked(j));
	}
	//  This will cycle through the list of processes then repeat
	return j;	
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
	
	//  Screw this process, we are getting a newer BETTER process
	pOldProcessPCB = pCurrentProcessPCB;

	

	//  Attempt to get a process that is not blocked
	idOfNextProcessToRun = scheduler();
	pCurrentProcessPCB = get_process_pointer_from_id(idOfNextProcessToRun);
	
	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()),"Deadlock:  All processes are in blocked state.");
	
	if(pCurrentProcessPCB == NULL) {
		assert((int)pCurrentProcessPCB, "pCurrentProcessPCB was null after calling get process pointer from id.");
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
