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
ProcessControlBlock* pCurrentProcessPCB  = NULL;
int isMemBlockJustReleased = 0;

ProcessEntry proc_init_table[NUM_PROCESSES];
ProcessControlBlock pcb_array[NUM_PROCESSES];
QueueHead ready_queue[NUM_PRIORITIES];
QueueHead blocked_queue[NUM_PRIORITIES];

extern unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;

// --------------------------------------------------------------------------
//                  Priority API
// --------------------------------------------------------------------------

int k_set_process_priority (int process_ID, int priority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in set process priority.");

   	if(priority >= 0 && priority < NUM_PRIORITIES) {	
		process->processPriority = priority;
		k_release_processor();
		return 0;
	} else {
	 	assert(0, "Error: the set priority is invalid.");
	}
	return -1;
}

int get_process_priority (int process_ID) {
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

// -------------------------------------------------------------------------
//           Helpers
// -------------------------------------------------------------------------

void enqueue(QueueHead* qHead, ProcessControlBlock* pcb) {
	ProcessControlBlock* oldTail = (*qHead).tail;
	(*qHead).tail = pcb;
	(*pcb).next = NULL; // TODO what if pcb is NULL?

	if (oldTail != NULL) {
		(*oldTail).next = pcb;
	}

	if ((*qHead).head == NULL) {
	 	(*qHead).head = pcb;
	}
}

ProcessControlBlock* dequeue(QueueHead* qHead) {
	ProcessControlBlock* firstIn = (*qHead).head;
	if (firstIn == NULL) return NULL;
	
	(*qHead).head = (*firstIn).next;
	(*firstIn).next = NULL;

	if ((*qHead).head == NULL) {
	 	(*qHead).tail = NULL;
	}

	return firstIn;
}

// --------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
//
//                       Initialize process management
//
// --------------------------------------------------------------------------------------

	 // We will have 4 priorities --> 0,1,2  are for normal processes; 3 is for the NULL process
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
				proc.process  = (uint32_t*) p1;
				break;
			case 2:
				proc.process  = (uint32_t*) p2;
				break;	
			case 3:
				proc.process  = (uint32_t*) p3;
				break;
			case 4:
				proc.process  = (uint32_t*) p4;
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

/**
 * @brief: initialize all processes in the system

 *       TODO: We should have also used an initialization table which contains the entry
 *       points of each process so that this code does not have to hard code
 *       proc1 and proc2 symbols. We leave all these imperfections as excercies to the reader 
 */
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



/*@brief: scheduler, pick the pid of the next to run process
 *@return: pid of the next to run process
 *         selects null process by default
 *POST: if pCurrentProcessPCB was NULL, then it gets set to &pcb1.
 *      No other effect on other global variables.
 */	 /*

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
	
}	  */


// -------------------------------------------------------------------------
//                 Release process and context switch
// -------------------------------------------------------------------------

// TODO: REPLACE OLD SCHEDULER WITH THIS ONE WHEN TESTING IS DONE
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

	
/**
 * @brief release_processor(). 
 * @return -1 on error and zero on success
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
	pCurrentProcessPCB = scheduler(pOldProcessPCB, pNewProcessPCB);

	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()),"Deadlock:  All processes are in blocked state.");


	// The Context Switch

	// If the scheduler decided to run the same process,
	// set state to RUN if it's NEW
	if (pCurrentProcessPCB == pOldProcessPCB) {
		if (pCurrentProcessPCB->currentState == NEW) {
			pCurrentProcessPCB->currentState = RUN;
			__rte(); 	
		}

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

		// We remove running processes from the ready queue
		pNewProcessPCB = dequeue(&(ready_queue[pCurrentProcessPCB->processPriority]));
		assert(pCurrentProcessPCB == pNewProcessPCB, "ERROR: ready queue and process priorities not in sync");

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

	return 0;
}

// ------------------------------------------------------------------------
