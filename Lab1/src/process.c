#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "utils.h"
#include "process.h"
#include "rtx.h"
#include "iprocess.h"
#include "system_proc.h"

/* Variable definitions */
ProcessControlBlock* pCurrentProcessPCB  = NULL;

ProcessEntry proc_init_table[NUM_USR_PROCESSES];							   ;
ProcessControlBlock pcb_array[NUM_USR_PROCESSES];
ListNode node_array[NUM_USR_PROCESSES];


// --------------------------------------------------------------------------
//                  Priority API
// --------------------------------------------------------------------------

int is_ready_or_new(proc_state_t state ) {
	return state == RDY || state == NEW;
}

int has_more_important_process(int priority) {
	 int i = 0;
	 for (; i < priority; i++) {
	  	if (ready_queue[i].head != NULL) {
		 	return 1;
		}
	 }
	 return 0;
}

int is_valid_priority(int priority){
	return priority >= 0 && priority < NUM_PRIORITIES - 1;
}

int k_set_process_priority (int process_ID, int newPriority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(!has_more_important_process(pCurrentProcessPCB->processPriority), "Error: running process is not of highest priority");
	assert(process != NULL, "Invalid process ID in set process priority.");
	assert(process_ID != 0, "Error: cannot change the priority of the NULL process.");
	assert(is_valid_priority(newPriority),"Error: the set priority is invalid.");

	if (is_ready_or_new(process->currentState)) {
	 	 ListNode *node = remove_node(&ready_queue[process->processPriority], (void*)process);
		 enqueue(&ready_queue[newPriority], node);
	} else if (process->currentState == BLOCKED_ON_MEMORY) {
		 ListNode *node = remove_node(&blocked_memory_queue[process->processPriority], (void*)process);
		 enqueue(&blocked_memory_queue[newPriority], node);
	} // TODO: need an else if for BLOCKED_ON_RECEIVE queue
	process->processPriority = newPriority;
	if (
		(has_more_important_process(newPriority) && pCurrentProcessPCB->processId == process_ID) ||
		(newPriority < pCurrentProcessPCB->processPriority && is_ready_or_new(process->currentState))
	){
	  	k_release_processor();
	}
	return 0;

}

int k_get_process_priority (int process_ID) {
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	assert(process != NULL, "Invalid process ID in get process priority.");

	return process->processPriority;
}

// -----------------------------------------------------------------------------------


ProcessControlBlock * get_process_pointer_from_id(int process_ID) {
	if (process_ID < NUM_USR_PROCESSES) {
		return (process_ID >= 0) ? &pcb_array[process_ID] : NULL;
	} else {
		if (process_ID == get_uart_pcb()->processId) return get_uart_pcb();
		if (process_ID == get_timer_pcb()->processId) return get_timer_pcb();
		if (process_ID == get_crt_pcb()->processId) return get_crt_pcb();
		if (process_ID == get_kcd_pcb()->processId) return get_kcd_pcb();
		if (process_ID == get_clock_pcb()->processId) return get_clock_pcb();
		assert(0,"error invalid process id.");
		return NULL;
	}
}

ProcessControlBlock* get_interrupted_process() {
 	int i;
	for(i = 0; i < NUM_USR_PROCESSES; i++){	
	 	if (pcb_array[i].currentState == INTERRUPTED) return &pcb_array[i];
	}
	//  These are other process that can be interrupted
	if (get_crt_pcb()->currentState == INTERRUPTED) return get_crt_pcb();
	if (get_kcd_pcb()->currentState == INTERRUPTED) return get_kcd_pcb();
	if (get_clock_pcb()->currentState == INTERRUPTED) return get_clock_pcb();

	return NULL;
}

ListNode* get_node_of_process(int process_ID) {
	ListNode *node = (process_ID > NUM_USR_PROCESSES - 1) ? NULL : &node_array[process_ID];

	assert(node != NULL, "ERROR: process does not have a list node");
	assert(((ProcessControlBlock*)node->data)->processId == process_ID, 
		"ERROR: list node does not contain the right process"); 	

	return node;
}

int is_usr_proc(int process_id) {
 	return process_id < NUM_USR_PROCESSES;
}

uint8_t is_sys_proc(int proc_id) {
 	return proc_id == get_kcd_pcb()->processId || proc_id == get_crt_pcb()->processId || proc_id == get_clock_pcb()->processId;
}

int is_process_blocked(int processId){
	//  Right now this is the only blocking state
	return (pcb_array[processId].currentState == BLOCKED_ON_MEMORY);
}

int is_deadlocked(){
	int i;
	//  We don't need to check the null process, pid 0
	for(i = 1; i < NUM_USR_PROCESSES; i++){
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
	for(i = 1; i < NUM_USR_PROCESSES; i++){
		if(is_process_blocked(i)){
			return 1;
		}
	}
	//  nothing is blocked
	return 0;
}

uint8_t is_i_proc(int proc_id) {
 	return proc_id == get_timer_pcb()->processId || proc_id == get_uart_pcb()->processId;
}

uint32_t * get_start_stack(int proc_id){
	if(proc_id < NUM_USR_PROCESSES){
		return proc_init_table[proc_id].start_sp;
	}

 	if(proc_id == get_kcd_pcb()->processId)
		return KCD_START_STACK;
		
	if(proc_id == get_crt_pcb()->processId)
		return CRT_START_STACK;
		
	if(proc_id == get_clock_pcb()->processId)
		return CLOCK_START_STACK;

	if(proc_id == get_timer_pcb()->processId)
		return TIMER_START_STACK;
		
	if(proc_id == get_uart_pcb()->processId)
		return UART_START_STACK;


	return (uint32_t *)0;
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

	for( i = 0; i < NUM_USR_PROCESSES; i++ ){
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
void zero_init_queue(LinkedList qHead[], int len) {
	int i;
	for (i = 0; i < len; ++i) {
	 	qHead[i].head = qHead[i].tail = NULL;
	}
}

void process_init() 
{
    volatile int i;
	uint32_t * sp;
	int procIndex;
	int priority;

	init_processe_table();
	zero_init_queue(ready_queue, NUM_PRIORITIES);
	zero_init_queue(blocked_memory_queue, NUM_PRIORITIES);
	zero_init_queue(blocked_receive_queue, NUM_PRIORITIES);

	//  For all the processes
	for (procIndex = 0; procIndex < NUM_USR_PROCESSES; ++procIndex) {
		ProcessControlBlock process;

		//  Set up the process control block
		process.processId = proc_init_table[procIndex].pid;
		process.currentState = NEW;
		process.waitingMessages.head = NULL;
		process.waitingMessages.tail = NULL;
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
 	for (i = 0; i < NUM_USR_PROCESSES; ++i) {
		ListNode *node = &node_array[i];
		priority = pcb_array[i].processPriority;
		// Pass the priority's head node and theListNode that contains a pcb
		node->data = &(pcb_array[i]);
		node->next = NULL;
	 	enqueue(&(ready_queue[priority]), node);
	}

	//  To start off, set the current process to the null process
	pCurrentProcessPCB = &pcb_array[0];
}

// --------------------------------------------------------------------------------------




// -------------------------------------------------------------------------
//                 Release process and context switch
// -------------------------------------------------------------------------


void block_current_process() {
	//TODO  make sure that we are not trying to block a process that is not allowed to block!!!!
	ListNode *node = get_node_of_process(pCurrentProcessPCB->processId);
 	assert(pCurrentProcessPCB->currentState == RUN, "Error: Attempted to block a non-running process");

    pCurrentProcessPCB->currentState = BLOCKED_ON_MEMORY; 
	enqueue(&blocked_memory_queue[pCurrentProcessPCB->processPriority], node);
}													

ProcessControlBlock* getNextReadyProcess(void) {
	int i;
	// Look for highest priority ready process.
	for (i = 0; i < NUM_PRIORITIES; i++) {
	 	if (ready_queue[i].head != NULL) {
			ListNode *node = ready_queue[i].head;
			return (ProcessControlBlock*)node->data;	
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
	 	if (blocked_memory_queue[i].head != NULL) {
			ListNode *node = blocked_memory_queue[i].head;
			return (ProcessControlBlock*)node->data;	
		}
	}
	return NULL; 	
}

ProcessControlBlock* getRunningProcess() {
	int i;
	ProcessControlBlock* runningProcess = NULL;
	//run for all processes; need to make sure no more than 1 process is running
	for (i = 0; i < NUM_USR_PROCESSES; i++) { 
		if (pcb_array[i].currentState == RUN){
			assert(runningProcess == NULL, "Error: multiple processes with state RUN.");
			runningProcess = &pcb_array[i];	
		}
	}
	assert(runningProcess != NULL, "Error: there are no running processes.");
	return runningProcess;
}

ProcessControlBlock* scheduler(ProcessControlBlock* pOldPCB, ProcessControlBlock* pNewPCB) {
	ProcessControlBlock* interruptedPCB = NULL;
	ProcessControlBlock * newSysProc = NULL;
	ProcessControlBlock * waitingSysProc = NULL;
	
	assert(pOldPCB != NULL && pNewPCB != NULL, "ERROR: Attempted to schedule NULL");

	assert(is_ready_or_new(pNewPCB->currentState),
		"ERROR: Scheduler attempted to schedule a non-ready or non-new process.");

	assert(pOldPCB->currentState != RDY,
		"ERROR: Scheduler encountered an unexpected state for the old (current) process.");

	// If there is an interrupted process with higher priority, it should run next
	interruptedPCB  = get_interrupted_process();
	
	// Otherwise, if there is a new sys proc, we should schedule it
	newSysProc = get_new_sys_proc();

	// Otherwise, if there are any system procs that need to run (have waiting messages), they have priority
	waitingSysProc = get_waiting_sys_proc();

	if (interruptedPCB != NULL && pNewPCB->processPriority >= interruptedPCB->processPriority) {
		return interruptedPCB;
 	} else if (newSysProc != NULL) {
		assert(newSysProc->currentState == NEW, "ERROR: New sys proc was not new.");
		return newSysProc;
	} else if (waitingSysProc != NULL) { 
		assert(
			waitingSysProc->currentState == RDY || waitingSysProc->currentState == NEW,
			"ERROR: New sys proc was not new."
		);
		return waitingSysProc;
	} else if (pNewPCB->processPriority <= pOldPCB->processPriority) {
		return pNewPCB;
	} else {
		if (pOldPCB->currentState == RUN || pOldPCB->currentState == NEW) {
		 	return pOldPCB;	
		} else {
			return pNewPCB;
		}
	}

}

__asm void __new_iproc_return(void) {
	POP {r0-r4,r12,pc}	;needed to pop what was pushed in i-proc initialisation
}

__asm void context_switch(ProcessControlBlock* pOldProcessPCB, ProcessControlBlock* pNewProcessPCB)
{
	PRESERVE8
	IMPORT c_context_switch
	PUSH{r0-r11, lr}
	BL c_context_switch
	POP{r0-r11, pc}
} 

void c_context_switch(ProcessControlBlock* pOldProcessPCB, ProcessControlBlock* pNewProcessPCB) {
	

	pCurrentProcessPCB = pNewProcessPCB;

	if (pCurrentProcessPCB == pOldProcessPCB) {
		// If the scheduler decided to run the same process,
		// set state to RUN if it's NEW
		if (pCurrentProcessPCB->currentState == NEW) {
			goto set_to_run_and_rte; 	
		} else if (pCurrentProcessPCB->currentState == INTERRUPTED) {
		 	pCurrentProcessPCB->currentState = RUN;
		}
	} else if (pCurrentProcessPCB->currentState == BLOCKED_ON_MEMORY){
		// Context switch due to release memory
		assert(pOldProcessPCB->currentState == RUN, "Error: The old process is not in a running state.");
		goto save_old_and_set_new_MSP;

	} else if (pCurrentProcessPCB->currentState == INTERRUPTED) {
		// We are switching from an iprocess to an interrupted process
		goto save_old_and_set_new_MSP;

	} else {
		/* Otherwise, we must switch from the old process to the new one
		Switching from an interrupted process to an iprocess
		or to a higher priority process	 */
		if (pOldProcessPCB->currentState == INTERRUPTED) {
			goto on_old_process_interrupted;
		} 

		// "default" switch case (no interrupted processes to consider)
		
		if (pOldProcessPCB->currentState == RUN) {
			pOldProcessPCB->currentState = RDY;
			if (pOldProcessPCB->processId < NUM_USR_PROCESSES) {
				// Put old process back in his appropriate priority queue
				enqueue(&(ready_queue[pOldProcessPCB->processPriority]), 
				get_node_of_process(pOldProcessPCB->processId)); 
			}
		}
		
		// Don't save the MSP if the process is NEW because it was not running,
		// so there should be nowhere it sensibly returns to
		if (pOldProcessPCB->currentState != NEW) {
			pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		}
		goto set_up_next_ready_process;
	}


	//  Don't delete this return even though it is tempting
	return;

	on_old_process_interrupted:
		pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		
		// check if new process is a user process
		if (!is_i_proc(pCurrentProcessPCB->processId)) {
			pOldProcessPCB->currentState = RDY;
			assert(is_usr_proc(pOldProcessPCB->processId), "ERROR: Unexpected interrupted sys proc");
			enqueue(&(ready_queue[pOldProcessPCB->processPriority]), 
				get_node_of_process(pOldProcessPCB->processId));	
		}
		goto set_up_next_ready_process;
	set_up_next_ready_process:
		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
		
		if (
			is_ready_or_new(pCurrentProcessPCB->currentState) && 
			pCurrentProcessPCB->processId < NUM_USR_PROCESSES) 
		{
			// We remove processes from the ready queue
			assert(
				pCurrentProcessPCB == (ProcessControlBlock*)dequeue(&(ready_queue[pCurrentProcessPCB->processPriority]))->data,
				"ERROR: ready queue and process priorities not in sync"
			);	
		}
		
		if (pCurrentProcessPCB->currentState == NEW) {
			if (is_i_proc(pCurrentProcessPCB->processId)) {
				pCurrentProcessPCB->currentState = RUN;
				__new_iproc_return();
			} else {
				goto set_to_run_and_rte;
			}
		}
		pCurrentProcessPCB->currentState = RUN;
		return;

	save_old_and_set_new_MSP:
		assert((
			(uint32_t *) __get_MSP() <= get_start_stack(pOldProcessPCB->processId) &&
			(uint32_t *) __get_MSP() > get_start_stack(pOldProcessPCB->processId) - (STACKS_SIZE/ sizeof(int))
		),"Process has stack overflow.");
		pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
		return;
	
	set_to_run_and_rte:
		pCurrentProcessPCB->currentState = RUN;
		__rte();
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

	assert(is_ready_or_new(pNewProcessPCB->currentState),
 		"Error: We have a process that is not in a ready or new state in the ready queue.");

	// Decide what should run next
	pNewProcessPCB = scheduler(pOldProcessPCB, pNewProcessPCB);

	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()), "Deadlock:  All processes are in blocked state.");

	// The Context Switch
	context_switch(pOldProcessPCB, pNewProcessPCB);

	return 0;
}

// ------------------------------------------------------------------------
