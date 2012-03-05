/* @brief: rtx.c kernel API implementations, this is a skeleton only
 * @author: Yiqing Huang
 * @date: 2012/01/08
 */

#include "rtx.h"
#include "utils.h"

//Variable declarations 

QueueHead ready_queue[NUM_PRIORITIES];
QueueHead blocked_queue[NUM_PRIORITIES];

unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;


// -------------------------------------------------------------------------
//           Helpers
// -------------------------------------------------------------------------

void enqueue(QueueHead* qHead, ProcessControlBlock* pcb) {
	ProcessControlBlock* oldTail = (*qHead).tail;
	(*qHead).tail = pcb;
	(*pcb).next = NULL; // TODO what if pcb is NULL?		 -- OOPS!

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

