#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "utils.h"
#include "process.h"
#include "rtx.h"
#include "iprocess.h"
#include "system_proc.h"

/* Variable definitions */
ProcessControlBlock* pCurrentProcessPCB  = NULL;

ProcessEntry proc_init_table[NUM_USR_PROCESSES + NUM_SYS_PROCESSES];							   
ProcessControlBlock pcb_array[NUM_USR_PROCESSES + NUM_SYS_PROCESSES];
ListNode node_array[NUM_USR_PROCESSES + NUM_SYS_PROCESSES];


// --------------------------------------------------------------------------
//                  Priority API
// --------------------------------------------------------------------------

int is_ready_or_new(proc_state_t state ) {
	return state == RDY || state == NEW;
}

int has_more_important_process(int priority) {
	// only system processes are of priority 0; should start check at priority 1
	 int i = 1;
	 for (; i < priority; i++) {
	  	if (ready_queue[i].head != NULL) {
		 	return 1;
		}
	 }
	 return 0;
}

//User processes can only have priorities 1,2,3
int is_valid_priority(int priority){
	return priority > 0 && priority < NUM_PRIORITIES - 1;
}

int k_set_process_priority (int process_ID, int newPriority) {	
	ProcessControlBlock * process = get_process_pointer_from_id(process_ID);

	// not applicabale now - always have sys procs of higher priority, decided not to preempt now
	//assert(!has_more_important_process(pCurrentProcessPCB->processPriority), "Error: running process is not of highest priority");
	assert(process != NULL, "Invalid process ID in set process priority.");
	assert(process_ID != 0, "Error: cannot change the priority of the NULL process.");
	assert(is_valid_priority(newPriority),"Error: the set priority is invalid.");

	if (is_ready_or_new(process->currentState)) {
	 	 ListNode *node = remove_node(&ready_queue[process->processPriority], (void*)process);
		 enqueue(&ready_queue[newPriority], node);
	} else if (process->currentState == BLOCKED_ON_MEMORY) {
		 ListNode *node = remove_node(&blocked_memory_queue[process->processPriority], (void*)process);
		 enqueue(&blocked_memory_queue[newPriority], node);
	}
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

int is_pid_valid(int process_ID) {
	return process_ID > 0 && (process_ID < NUM_USR_PROCESSES ||
			process_ID == get_crt_pcb()->processId ||
			process_ID == get_kcd_pcb()->processId ||
			process_ID == get_uart_pcb()->processId ||
			process_ID == get_timer_pcb()->processId ||
			process_ID == get_clock_pcb()->processId||
			process_ID == get_priority_process_pcb()->processId);

}


ProcessControlBlock * get_process_pointer_from_id(int process_ID) {
	if (process_ID < NUM_USR_PROCESSES) {
		return (process_ID >= 0) ? &pcb_array[process_ID] : NULL;
	} else {
		if (process_ID == get_crt_pcb()->processId) return get_crt_pcb();
		if (process_ID == get_kcd_pcb()->processId) return get_kcd_pcb();
		if (process_ID == get_uart_pcb()->processId) return get_uart_pcb();
		if (process_ID == get_timer_pcb()->processId) return get_timer_pcb();
		if (process_ID == get_clock_pcb()->processId) return get_clock_pcb();
		if (process_ID == get_priority_process_pcb()->processId) return get_priority_process_pcb();
		assert(0,"error invalid process id.");
		return NULL;
	}
}

ProcessControlBlock* get_interrupted_process() {
 	int i;
	for(i = 0; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; i++){	
	 	if (pcb_array[i].currentState == INTERRUPTED) return &pcb_array[i];
	}
	return NULL;
}

ListNode* get_node_of_process(int process_ID) {
	ListNode *node = (process_ID > NUM_USR_PROCESSES - 1) ? NULL : &node_array[process_ID];

	if(node == NULL && is_sys_proc(process_ID)) {	 
		node = &node_array[process_ID - USR_SYS_ID_DIFF];
	}

	assert(node != NULL, "ERROR: process does not have a list node");
	assert(((ProcessControlBlock*)node->data)->processId == process_ID, 
		"ERROR: list node does not contain the right process"); 	

	return node;
}

int is_usr_proc(int process_id) {
 	return process_id < NUM_USR_PROCESSES;
}

uint8_t is_sys_proc(int proc_id) {
 	return proc_id == get_kcd_pcb()->processId || proc_id == get_crt_pcb()->processId || proc_id == get_clock_pcb()->processId || proc_id == get_priority_process_pcb()->processId;
}

int is_process_blocked(int processId){
	//  Right now this is the only blocking state
	return (pcb_array[processId].currentState == BLOCKED_ON_MEMORY);
}

int is_deadlocked(){
	int i;
	//  We don't need to check the null process, pid 0
	for(i = 1; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; i++){
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
	for(i = 1; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; i++){
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
		return proc_init_table[proc_id - USR_SYS_ID_DIFF].start_sp;
		
	if(proc_id == get_crt_pcb()->processId)
		return proc_init_table[proc_id - USR_SYS_ID_DIFF].start_sp;   
		
	if(proc_id == get_clock_pcb()->processId)
		return proc_init_table[proc_id - USR_SYS_ID_DIFF].start_sp;

	if(proc_id == get_priority_process_pcb()->processId)
		return proc_init_table[proc_id - USR_SYS_ID_DIFF].start_sp;
																	  
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

// We will have 5 priorities --> 0 for system procs; 1,2,3  are for normal processes; 4 is for the NULL process
void init_processe_table() {
	unsigned int sp = free_mem;
   	int i;

	// increase number of processes to include sys procs
	// on sys procs, override the id and priority values in the case statement 
	for( i = 0; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; i++ ){
		ProcessEntry proc;
		int priority;

		proc.pid = i;
		// want to only assign priorities 1 2 3 to user processes 
		priority = (i == 0) ? NUM_PRIORITIES - 1 : i % 3;
		if (priority == 0) { priority = 3; }
		proc.priority = priority;

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
			case 7:
				proc.process  = (uint32_t*) test_proc_A;
				proc.priority = 3;
				break;
			case 8:
				proc.process  = (uint32_t*) test_proc_B;
				proc.priority = 3;
				break;
			case 9:
				proc.process  = (uint32_t*) test_proc_C;
				proc.priority = 3;
				break;
			case 10:
				proc.process = (uint32_t*) crt_display;
				proc.pid  = 0xC;
				proc.priority = 0;
				break;
			case 11:
				proc.process = (uint32_t*) keyboard_command_decoder;
				proc.pid  = 0xD;
				proc.priority = 0;
				break;
			case 12: 
				proc.process = (uint32_t*) wall_clock;
				proc.pid = 0xE;
				proc.priority = 1;
				break;	
			case 13: 
				proc.process = (uint32_t*) priority_process;
				proc.pid = 0xF;
				proc.priority = 1;
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

	// For all user and system processes
	for (procIndex = 0; procIndex < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; ++procIndex) {
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
		//Double word alignment
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
 	for (i = 0; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; ++i) {
		ListNode *node = &node_array[i];
		priority = pcb_array[i].processPriority;
		// Pass the priority's head node and the ListNode that contains a pcb
		node->data = &(pcb_array[i]);
		node->next = NULL;
	 	enqueue(&(ready_queue[priority]), node);
	}

	//  To start off, set the current process to the null process
	pCurrentProcessPCB = &pcb_array[0];
}

ProcessControlBlock* get_crt_pcb() {
	return &pcb_array[NUM_USR_PROCESSES];
}

ProcessControlBlock* get_kcd_pcb() {
	return &pcb_array[NUM_USR_PROCESSES + 1];
}

ProcessControlBlock* get_clock_pcb() {
	return &pcb_array[NUM_USR_PROCESSES + 2];
}
	 
ProcessControlBlock* get_priority_process_pcb() {
	return &pcb_array[NUM_USR_PROCESSES + 3];
}

// --------------------------------------------------------------------------------------



// -------------------------------------------------------------------------
//                 Release process and context switch
// -------------------------------------------------------------------------


void block_current_process() {
	ListNode *node;

	assert(!is_i_proc(pCurrentProcessPCB->processId),"ERROR: attempting to block an iprocess.");
	node = get_node_of_process(pCurrentProcessPCB->processId);
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
	// Look for highest priority blocked process.
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
	for (i = 0; i < NUM_USR_PROCESSES + NUM_SYS_PROCESSES; i++) { 
		if (pcb_array[i].currentState == RUN){
			assert(runningProcess == NULL, "Error: multiple processes with state RUN.");
			runningProcess = &pcb_array[i];	
		}
	}

	if (get_timer_pcb()->currentState == RUN) {
		assert(runningProcess == NULL, "Error: multiple processes with state RUN.");
		runningProcess = get_timer_pcb();
	}

	if (get_uart_pcb()->currentState == RUN) {
		assert(runningProcess == NULL, "Error: multiple processes with state RUN.");
	 	runningProcess = get_uart_pcb();
	}

	assert(runningProcess != NULL, "Error: there are no running processes.");
	return runningProcess;
}

ProcessControlBlock* scheduler(ProcessControlBlock* pOldPCB, ProcessControlBlock* pNewPCB) {
	ProcessControlBlock* interruptedPCB = NULL;
	
	assert(pOldPCB != NULL && pNewPCB != NULL, "ERROR: Attempted to schedule NULL");

	assert(is_ready_or_new(pNewPCB->currentState),
		"ERROR: Scheduler attempted to schedule a non-ready or non-new process.");

	assert(pOldPCB->currentState != RDY,
		"ERROR: Scheduler encountered an unexpected state for the old (current) process.");

	// If there is an interrupted process with higher priority, it should run next
	interruptedPCB  = get_interrupted_process();

	if (interruptedPCB != NULL && (is_sys_proc(pNewPCB->processId) || pNewPCB->processPriority >= interruptedPCB->processPriority)) {
		return interruptedPCB;
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

int has_stack_overflow(uint32_t * p, int processId){
	/*  The stack grows down so if the sp is lower than the start of the previous stack there is a problem */
	return p <= get_start_stack(processId) - (STACKS_SIZE/ sizeof(int));
}


int has_stack_underflow(uint32_t * p, int processId){
	/*  The stack grows down so if the sp is before the start stack there was an underflow */
	return p > get_start_stack(processId);
}

void c_context_switch(ProcessControlBlock* pOldProcessPCB, ProcessControlBlock* pNewProcessPCB) {
	uint32_t * tmpMSP = 0;
	pCurrentProcessPCB = pNewProcessPCB;

	if (pCurrentProcessPCB == pOldProcessPCB) {
		// If the scheduler decided to run the same process,
		// set state to RUN if it's NEW
		if (pCurrentProcessPCB->currentState == NEW) {
			goto set_to_run_and_rte; 	
		} else if (pCurrentProcessPCB->currentState == INTERRUPTED) {
		 	goto set_to_run_and_exit;
		}
		goto exit;
	} else if (pCurrentProcessPCB->currentState == BLOCKED_ON_MEMORY){
		// Context switch due to release memory
		assert(pOldProcessPCB->currentState == RUN, "Error: The old process is not in a running state.");
		goto save_old_and_set_new_MSP;
	} else if (pCurrentProcessPCB->currentState == INTERRUPTED) {
		assert(is_i_proc(pOldProcessPCB->processId), "ERROR: Unexpected switch from non_iproc to interrupted.");
		pOldProcessPCB->currentState = RDY;
		// We are switching from an iprocess to an interrupted process
		goto save_old_and_set_new_MSP;
	} else {
		/* Otherwise, we must switch from the old process to the new one
		Switching from an interrupted process to an iprocess
		or to a higher priority process	 */
		goto save_old_process;
	}

	save_old_process:
		if (pOldProcessPCB->currentState == RUN) {
			// "default" switch case (no interrupted processes to consider)
			pOldProcessPCB->currentState = RDY;
			if (!is_i_proc(pOldProcessPCB->processId)) {
				// Put old process back in his appropriate priority queue
				enqueue(&(ready_queue[pOldProcessPCB->processPriority]),get_node_of_process(pOldProcessPCB->processId)); 
			}
		}else if(pOldProcessPCB->currentState == INTERRUPTED) {
			// check if new process is a user process
			if (!is_i_proc(pCurrentProcessPCB->processId)) {
				assert(is_usr_proc(pOldProcessPCB->processId), "ERROR: Unexpected interrupted sys proc");
				pOldProcessPCB->currentState = RDY;
				enqueue(&(ready_queue[pOldProcessPCB->processPriority]),get_node_of_process(pOldProcessPCB->processId));	
			}
		}

		/* Don't save the MSP if the process is NEW because it was not running,
		 so there should be nowhere it sensibly returns to	 */
		if (pOldProcessPCB->currentState != NEW) {
			uint32_t * tmpMSP1 = get_start_stack(pOldProcessPCB->processId);
			tmpMSP = (uint32_t *)__get_MSP();
			if(has_stack_underflow(tmpMSP,pOldProcessPCB->processId)) {
				 	assert(0, "Old process has stack underflow."); 
			}
			if(has_stack_overflow(tmpMSP,pOldProcessPCB->processId)) {
				 	assert(0, "Old process has stack overflow."); 
			}
			pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		}
		goto set_up_new_process;
	set_up_new_process:
		if (is_ready_or_new(pCurrentProcessPCB->currentState) && !is_i_proc(pCurrentProcessPCB->processId)){
			// We remove user and system processes from the ready queue
			assert(pCurrentProcessPCB == (ProcessControlBlock*)dequeue(&(ready_queue[pCurrentProcessPCB->processPriority]))->data,
				"ERROR: ready queue and process priorities not in sync");	
		}

		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
		
		if (pCurrentProcessPCB->currentState == NEW) {
			if (is_i_proc(pCurrentProcessPCB->processId)) {
				goto set_to_run_and_new_iproc_return;
			} else {
				goto set_to_run_and_rte;
			}
		}
		goto set_to_run_and_exit;
	save_old_and_set_new_MSP:
		// Put this in a tmp valariable because calling getmsp as a fcn param might have unexpected consequences
		tmpMSP = (uint32_t *)__get_MSP();
		if(has_stack_underflow(tmpMSP,pOldProcessPCB->processId)) {
			 	assert(0, "Old process has stack underflow."); 
		}
		if(has_stack_overflow(tmpMSP,pOldProcessPCB->processId)) {
			 	assert(0, "Old process has stack overflow."); 
		}

		pOldProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		__set_MSP((uint32_t) pCurrentProcessPCB->processStackPointer);
		goto exit;
	set_to_run_and_rte:
		if(has_stack_underflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack underflow on rte.  Probably initialized wrong."); 
		}
		if(has_stack_overflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack overflow on rte.  Probably initialized wrong."); 
		}
		pCurrentProcessPCB->currentState = RUN;
		__rte();
	set_to_run_and_new_iproc_return:
		if(has_stack_underflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack underflow on __new_iproc_return.  Probably initialized wrong."); 
		}
		if(has_stack_overflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack overflow on __new_iproc_return.  Probably initialized wrong."); 
		}
		pCurrentProcessPCB->currentState = RUN;
		__new_iproc_return();
	set_to_run_and_exit:
		pCurrentProcessPCB->currentState = RUN;
		goto exit;
	exit:
		if(has_stack_underflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack underflow in context switch function."); 
		}
		if(has_stack_overflow(pCurrentProcessPCB->processStackPointer,pCurrentProcessPCB->processId)) {
			 	assert(0, "Process has stack overflow in context switch function."); 
		}

		return;
}

	
/**	  
 * POST: pCurrentProcessPCB gets updated
 */
int k_release_processor(void){
	volatile int idOfNextProcessToRun;
	volatile proc_state_t state;

	ProcessControlBlock* pOldProcessPCB = pCurrentProcessPCB;
	
	// Get next ready process																						
	ProcessControlBlock* pNewProcessPCB = getNextReadyProcess();	// get's any ready process from the READY queue :)

	assert(is_ready_or_new(pNewProcessPCB->currentState),
 		"Error: We have a process that is not in a ready or new state in the ready queue.");

	// Decide what should run next
	pNewProcessPCB = scheduler(pOldProcessPCB, pNewProcessPCB); // sys procs are in RDY queue, remove special methods

	//  Make sure we are not deadlocked
	assert(!(is_deadlocked()), "Deadlock:  All processes are in blocked state.");

	// The Context Switch
	context_switch(pOldProcessPCB, pNewProcessPCB);

	return 0;
}

// ------------------------------------------------------------------------
