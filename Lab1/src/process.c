#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "utils.h"
#include "process.h"
#include "rtx.h"

/* Variable definitions */
ProcessControlBlock* pCurrentProcessPCB  = NULL;

ProcessEntry proc_init_table[NUM_PROCESSES];
ProcessControlBlock pcb_array[NUM_PROCESSES];

// --------------------------------------------------------------------------
//                  Priority API
// --------------------------------------------------------------------------

int has_more_important_process(int priority) {
	 int i = 0;
	 for (; i < priority; i++) {
	  	if (ready_queue[i].head != NULL) {
		 	return 1;
		}
	 }
	 return 0;
}


int k_set_process_priority (int process_ID, int priority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(!has_more_important_process(pCurrentProcessPCB->processPriority), "Error: running process is not of highest priority");
	assert(process != NULL, "Invalid process ID in set process priority.");
	assert(process_ID != 0, "Error: cannot change the priority of the NULL process.");

   	if(priority >= 0 && priority < NUM_PRIORITIES - 1) {
		if (process->currentState == RDY) {
		 	 remove_proc(&ready_queue[process->processPriority], process);
			 enqueue(&ready_queue[priority], process);
		} else if (process->currentState == BLOCKED_ON_MEMORY) {
			 remove_proc(&blocked_queue[process->processPriority], process);
			 enqueue(&blocked_queue[priority], process);
		}
		process->processPriority = priority;
		if ((has_more_important_process(priority) && pCurrentProcessPCB->processId == process_ID)
		|| (priority < pCurrentProcessPCB->processPriority && process->currentState == RDY)) {
		  	k_release_processor();
		}
		return 0;
	} else {
	 	assert(0, "Error: the set priority is invalid.");
	}
	return -1;
}

int k_get_process_priority (int process_ID) {
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in get process priority.");

	return process->processPriority;
}

// -----------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------
//
//                       Initialize process management
//
// --------------------------------------------------------------------------------------

// We will have 5 priorities --> 0,1,2,3  are for normal processes; 4 is for the NULL process
void init_processe_table() {
	unsigned int sp = free_mem;
   	int i;

	for( i = 0; i < NUM_PROCESSES; i++ ){
		ProcessEntry proc;

		proc.pid = i;
		proc.priority = (i == 0) ? NUM_PRIORITIES - 1 : (i - 1) % (NUM_PRIORITIES - 1);
		proc.stack_size = STACKS_SIZE;
		sp += STACKS_SIZE;
		proc.start_sp = (uint32_t*)sp;
		switch(i) {
		 	case 0:
				proc.process  = (uint32_t*) nullProc;
				break;
			case 1:
				proc.process  = (uint32_t*) test_process_1;
				break;
			case 2:
				proc.process  = (uint32_t*) test_process_2;
				break;	
			case 3:
				proc.process  = (uint32_t*) test_process_3;
				break;
			case 4:
				proc.process  = (uint32_t*) test_process_4;
				break;
			case 5:
				proc.process  = (uint32_t*) test_process_5;
				break;
			case 6:
				proc.process  = (uint32_t*) test_process_6;
				break;						
			default:
				proc.process  = (uint32_t*) nullProc;
				break;
		}

		proc_init_table[i] = proc;
	}

}

// zero-initialization (just in case)
void init_ready_queue() {
	int i;
	for (i = 0; i < NUM_PRIORITIES; ++i) {
	 	ready_queue[i].head = NULL;
		ready_queue[i].tail = NULL;
	}
}

void process_init() 
{
    volatile int i;
	uint32_t * sp;
	int procIndex;
	int priority;

	init_processe_table();
	init_ready_queue();

	//  For all the processes
	for (procIndex = 0; procIndex < NUM_PROCESSES; ++procIndex) {
		ProcessControlBlock process;

		//  Set up the process control block
		process.processId = proc_init_table[procIndex].pid;
		process.currentState = NEW;
		// must mod 4 so that the priorities don't go over 3 (only null process can be level 4)
		process.processPriority =  proc_init_table[procIndex].priority;
		
		sp  = proc_init_table[procIndex].start_sp;
		
		// 8 bytes alignement adjustment to exception stack frame
		// TODO: figure out why we want sp to have 4 right-aligned non-zero bits before 
		// decrementing it.
		if (!(((uint32_t)sp) & 0x04)) {
		    --sp; 
		}

		*(--sp) = INITIAL_xPSR;      // user process initial xPSR  

		// Set the entry point of the process
		*(--sp) = (uint32_t) proc_init_table[procIndex].process;
		
		for (i = 0; i < 6; i++) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}

		process.processStackPointer = sp;

		pcb_array[procIndex] = process;
	}

	// queue up all processes as ready
 	for (i = 0; i < NUM_PROCESSES; ++i) {
		priority = pcb_array[i].processPriority;
		// Pass the priority's head node and the pcb
	 	enqueue(&(ready_queue[priority]), &(pcb_array[i]));
	}

	//  To start off, set the current process to the null process
	pCurrentProcessPCB = &pcb_array[0];
}

// --------------------------------------------------------------------------------------




// -------------------------------------------------------------------------
//                 Release process and context switch
// -------------------------------------------------------------------------


void block_current_process() {
 	assert(pCurrentProcessPCB->currentState == RUN, "Error: Attempted to block a non-running process");

    pCurrentProcessPCB->currentState = BLOCKED_ON_MEMORY;

	enqueue(&blocked_queue[pCurrentProcessPCB->processPriority], pCurrentProcessPCB);
}

ProcessControlBlock* getNextReadyProcess(void) {
	int i;
	// Look for highest priority ready process.
	for (i = 0; i < NUM_PRIORITIES; i++) {
	 	if (ready_queue[i].head != NULL) {
			return ready_queue[i].head;	
		}
	}
	// return null process.. null process should have been ready, so assert
	assert(0, "ERROR: null process was not in ready queue");
	return &pcb_array[0];
}

ProcessControlBlock* getBlockedProcess() {
	int i;
	// Look for highest priority ready process.
	for (i = 0; i < NUM_PRIORITIES; i++) {
	 	if (blocked_queue[i].head != NULL) {
			return blocked_queue[i].head;	
		}
	}
	return NULL; 	
}

ProcessControlBlock* getRunningProcess() {
	int i;
	ProcessControlBlock* runningProcess = NULL;
	for (i = 0; i < NUM_PROCESSES; i++) {
		if (pcb_array[i].currentState == RUN){
			assert(runningProcess == NULL, "Error: multiple processes with state RUN.");
			runningProcess = &pcb_array[i];	
		}
	}
	assert(runningProcess != NULL, "Error: there are no running processes.");
	return runningProcess;
}

ProcessControlBlock* scheduler(ProcessControlBlock* pOldPCB, ProcessControlBlock* pNewPCB) {
	assert(pOldPCB != NULL && pNewPCB != NULL, "ERROR: Attempted to schedule NULL");

	assert(pNewPCB->currentState == RDY || pNewPCB->currentState == NEW,
		"ERROR: Scheduler attempted to schedule a non-ready or non-new process.");

	assert(pOldPCB->currentState != RDY,
		"ERROR: Scheduler encountered an unexpected state for the old (current) process.");

 	if (pNewPCB->processPriority <= pOldPCB->processPriority) {
		return pNewPCB;
	} else {
		if (pOldPCB->currentState == RUN || pOldPCB->currentState == NEW) {
		 	return pOldPCB;	
		} else {
			return pNewPCB;
		}
	}

}

void context_switch(ProcessControlBlock* pOldProcessPCB, ProcessControlBlock* pNewProcessPCB) {
	pCurrentProcessPCB = pNewProcessPCB;

	// If the scheduler decided to run the same process,
	// set state to RUN if it's NEW
	if (pCurrentProcessPCB == pOldProcessPCB) {
		if (pCurrentProcessPCB->currentState == NEW) {
			pCurrentProcessPCB->currentState = RUN;
			__rte(); 	
		}

	// Context switch due to release memory
	} else if (pCurrentProcessPCB->currentState == BLOCKED_ON_MEMORY){
		assert(pOldProcessPCB->currentState == RUN, "Error: The old process is not in a running state.");
		pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		// Set to MSP to the process' stack which is about to run.
		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);

	// Otherwise, we must switch from the old process to the new one
	} else {

		/* -- Updating old process -- */

		if (pOldProcessPCB->currentState == RUN) {
			pOldProcessPCB->currentState = RDY;
			// Put old process back in his appropriate priority queue
			enqueue(&(ready_queue[pOldProcessPCB->processPriority]), pOldProcessPCB);
		}

		// Don't save the MSP if the process is NEW because it was not running,
		// so there should be nowhere it sensibly returns to
		if (pOldProcessPCB->currentState != NEW) {
			pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		}
		

		/* -- Updating new process -- */
		if (pCurrentProcessPCB->currentState == RDY || pCurrentProcessPCB->currentState == NEW) {
		   // We remove running processes from the ready queue
			pNewProcessPCB = dequeue(&(ready_queue[pCurrentProcessPCB->processPriority]));
			assert(pCurrentProcessPCB == pNewProcessPCB, "ERROR: ready queue and process priorities not in sync");
		}

		// Set to MSP to the process' stack which is about to run.
		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
		
		// NOTE: __rte() exits. That is why we assign RUN twice.
		if (pCurrentProcessPCB->currentState == NEW) {
			pCurrentProcessPCB->currentState = RUN;
			// pop exception stack frame from the stack for new processes (assembly function in hal.c)
			__rte(); //EXITTING CALL
		}
		pCurrentProcessPCB->currentState = RUN;
	    
	}
}

	
/**	  
 * TODO: @return -1 on error and zero on success
 * POST: pCurrentProcessPCB gets updated
 */
int k_release_processor(void){
	volatile int idOfNextProcessToRun;
	volatile proc_state_t state;

	ProcessControlBlock* pOldProcessPCB = pCurrentProcessPCB;
	
	// Get next ready process
	ProcessControlBlock* pNewProcessPCB = getNextReadyProcess();

	assert((pNewProcessPCB->currentState == RDY || pNewProcessPCB->currentState == NEW),
 	"Error: We have a process that is not in a ready or new state in the ready queue.");

	// Decide what should run next
	pNewProcessPCB = scheduler(pOldProcessPCB, pNewProcessPCB);

	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()),"Deadlock:  All processes are in blocked state.");

	// The Context Switch
	context_switch(pOldProcessPCB, pNewProcessPCB);

	return 0;
}

// ------------------------------------------------------------------------
