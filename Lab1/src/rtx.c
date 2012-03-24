#include "rtx.h"
#include "utils.h"

//Variable declarations 


LinkedList ready_queue[NUM_PRIORITIES];
LinkedList blocked_memory_queue[NUM_PRIORITIES];
LinkedList blocked_receive_queue[NUM_PRIORITIES];

unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;


// -------------------------------------------------------------------------
//           Helpers
// -------------------------------------------------------------------------


void enqueue(LinkedList* listHead, ListNode* node) {
	ListNode* oldTail = (*listHead).tail;
	(*listHead).tail = node;
	(*node).next = NULL;

	if (oldTail != NULL) {
		(*oldTail).next = node;
	}

	if ((*listHead).head == NULL) {
	 	(*listHead).head = node;
	}
}

ListNode* dequeue(LinkedList* qHead) {
	ListNode* firstIn = (*qHead).head;
	if (firstIn == NULL) return NULL;
	
	(*qHead).head = (*firstIn).next;
	(*firstIn).next = NULL;

	if ((*qHead).head == NULL) {
	 	(*qHead).tail = NULL;
	}

	return firstIn;
}


ListNode* remove_node(LinkedList* qHead, void* node_data) {
	ListNode* curr = (*qHead).head;	

	if (curr->data == node_data) {
		if (curr->next == NULL) {
		 	qHead->tail = NULL;
		}
		qHead->head = curr->next;
		return curr;
	}

	while (curr != NULL) {
		if (curr->next->data == node_data) { 
			ListNode* return_node;
			if (node_data == qHead->tail->data) {
				assert(qHead->tail->next == NULL, "ERROR: Tail next is not null.");
				qHead->tail = curr;
			}
			return_node = curr->next;
			curr->next = curr->next->next;
			return return_node;
		}
		curr = curr->next;
	}
	return NULL;
}

// --------------------------------------------------------------------------

