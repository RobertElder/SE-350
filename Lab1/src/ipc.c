#include "ipc.h"
#include "process.h"
#include "memory.h"

int k_send_message(int target_pid, void* envelope) {
	//atomic(on);
	Envelope* env = (Envelope*) envelope;
	ListNode* node;
	ProcessControlBlock* targetProcess = get_process_pointer_from_id(target_pid);

	env->sender_pid = pCurrentProcessPCB->processId;
	env->receiver_pid = target_pid;
	// TODO set some message

	//add envelope to the message queue of Target Process
	node->data = env;
	enqueue_ll(&targetProcess->waitingMessages, node);

	if (targetProcess->currentState == BLOCKED_ON_RECEIVE) {
	 	targetProcess->currentState = RDY;
		//TODO remove process from BLOCKED_ON_RECEIVE queue
		enqueue(&ready_queue[targetProcess->processPriority], targetProcess);
	}

	//atomic(off);
	// Target process can preempt the current process if it is of a higher priority
	if (targetProcess->processPriority > pCurrentProcessPCB->processPriority) {
		release_processor();
	}
	return 0;
}

void* k_receive_message(int* sender_ID) {
	// atomic(on)

	Envelope* env;
	
	while (pCurrentProcessPCB->waitingMessages.head == NULL) {
	 	pCurrentProcessPCB->currentState = BLOCKED_ON_RECEIVE;
		release_processor();
	}
	
	env = (Envelope*)dequeue_ll(&pCurrentProcessPCB->waitingMessages)->data;
	//atomic(off)
	return env;	
}
