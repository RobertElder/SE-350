/* @brief: rtx.c kernel API implementations, this is a skeleton only
 * @author: Yiqing Huang
 * @date: 2012/01/08
 */

#include "rtx.h"
#include "utils.h"

//Variable declarations 

<<<<<<< HEAD
#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

// To better strcture your code, you may want to split these functions
// into different files. For example, memory related kernel APIs in one file
// and process related API(s) in another file.

int maxNumberOfMemoryBlocksEverAllocatedAtOnce = 0;
extern int numberOfMemoryBlocksCurrentlyAllocated = 0;
=======
QueueHead ready_queue[NUM_PRIORITIES];
QueueHead blocked_queue[NUM_PRIORITIES];

unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;


// -------------------------------------------------------------------------
//           Helpers
// -------------------------------------------------------------------------
>>>>>>> d53f7d43deb1ad29c3b6cb666d985c3e20a470fb

void enqueue(QueueHead* qHead, ProcessControlBlock* pcb) {
	ProcessControlBlock* oldTail = (*qHead).tail;
	(*qHead).tail = pcb;
	(*pcb).next = NULL; // TODO what if pcb is NULL?		 -- OOPS!

<<<<<<< HEAD
void * get_address_of_memory_block_at_index(int memoryBlockIndex) {
	//  taken up by pMaxNumberOfMemoryBlocksEverAllocated plus the offset of the byte for that memory block
	return (void*)(START_OF_ALLOCATABLE_MEMORY + (memoryBlockIndex * MEMORY_BLOCK_SIZE));
}

char * get_address_of_memory_block_allocation_status_at_index(int memoryBlockIndex){
	return (char *)(START_OF_MEMORY_ALLOCATION_TABLE + (sizeof(char) * memoryBlockIndex));
}

void * allocate_memory_block_at_index(int memoryBlockIndex){
	// memoryBlockIndex is the 0-based index addressing the block of memory
	char * pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	// Set the status of this block to allocated (1)
	*pAllocationStatusByte = 1;
	return get_address_of_memory_block_at_index(memoryBlockIndex);
}

void request_mem_block_helper() {
	if (pCurrentProcessPCB->currentState == BLOCKED_ON_MEMORY) {
		ProcessControlBlock* runningProcess = getRunningProcess();

		ProcessControlBlock* pNewProcessPCB = dequeue(&(blocked_queue[pCurrentProcessPCB->processPriority]));
		assert(pCurrentProcessPCB == pNewProcessPCB, "ERROR: blocked queue and process priorities not in sync.");	

		if (runningProcess->processPriority <= pCurrentProcessPCB->processPriority) {
		 	pCurrentProcessPCB->currentState = RDY;
			enqueue(&(ready_queue[pCurrentProcessPCB->processPriority]), pCurrentProcessPCB);
			context_switch(pCurrentProcessPCB, runningProcess);
		} else {
			runningProcess->currentState = RDY;
			enqueue(&(ready_queue[runningProcess->processPriority]), runningProcess);
			pCurrentProcessPCB->currentState = RUN;	
		}
=======
	if (oldTail != NULL) {
		(*oldTail).next = pcb;
	}
>>>>>>> d53f7d43deb1ad29c3b6cb666d985c3e20a470fb

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

void remove_proc(QueueHead* qHead, ProcessControlBlock* pcb) {
	ProcessControlBlock* curr = (*qHead).head;	

	if (curr == pcb) {
		if (pcb->next == NULL) {
		 	qHead->tail = NULL;
		}
		qHead->head = pcb->next;
		return;
	}

	while (curr != NULL) {
		if (curr->next == pcb) {
			if (pcb == qHead->tail) {
				assert(pcb->next == NULL, "ERROR: Tail next is not null.");
				qHead->tail = curr;
			}
			curr->next = pcb->next;
			break;
		}
		curr = curr->next;
	}
}

// --------------------------------------------------------------------------

