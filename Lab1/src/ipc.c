#include "utils.h"
#include "ipc.h"
#include "process.h"
#include "memory.h"

#define KERNEL_POINTERS_OFFSET                0
#define SENDER_PID_OFFSET                     KERNEL_POINTERS_OFFSET + sizeof(int *)
#define DESTINATION_PID_OFFSET                SENDER_PID_OFFSET + sizeof(int)
#define MESSAGE_TYPE_OFFSET                   DESTINATION_PID_OFFSET + sizeof(int)
#define MESSAGE_DATA_OFFSET                   MESSAGE_TYPE_OFFSET + sizeof(int)


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
	enqueue(&targetProcess->waitingMessages, node);

	if (targetProcess->currentState == BLOCKED_ON_RECEIVE) {
	 	targetProcess->currentState = RDY;
		//TODO remove process from BLOCKED_ON_RECEIVE queue
		enqueue(&ready_queue[targetProcess->processPriority],
			get_node_of_process(targetProcess->processId));
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
	
	//don't want to block system calls or iprocesses (block only user procs pid 0 to 6)
	while (pCurrentProcessPCB->processId > NUM_PROCESSES && pCurrentProcessPCB->waitingMessages.head == NULL) {
	 	pCurrentProcessPCB->currentState = BLOCKED_ON_RECEIVE;
		release_processor();
	}
	
	env = (Envelope*)dequeue(&pCurrentProcessPCB->waitingMessages)->data;
	if (sender_ID != NULL) {
		*sender_ID = env->sender_pid;
	}
	//atomic(off)
	return env;	
}

/*
	A Message  

--------------------------
|     Kernel Pointers    |
--------------------------
|       Sender PID       |
--------------------------
|    Destination PID     |
--------------------------
|      Message Type      |
--------------------------
|       Message Data     |
--------------------------
*/

//  We will assume that a message is a memory block size
int get_sender_PID(void * p_message){
	return *((int * )((char *)p_message + SENDER_PID_OFFSET));
}

void set_sender_PID(void * p_message, int value){
	*((int * )((char *)p_message + SENDER_PID_OFFSET)) = value;
}

int get_destination_PID(void * p_message){
	return *((int * )((char *)p_message + DESTINATION_PID_OFFSET));
}

void set_destination_PID(void * p_message, int value){
	*((int * )((char *)p_message + DESTINATION_PID_OFFSET)) = value;
}

int get_message_type(void * p_message){
	return *((int * )((char *)p_message + MESSAGE_TYPE_OFFSET));
}

void set_message_type(void * p_message, int value){
	*((int * )((char *)p_message + MESSAGE_TYPE_OFFSET)) = value;
}

void * get_message_data(void * p_message){
	return (char *) p_message + MESSAGE_DATA_OFFSET;
}

void set_message_data(void * p_message, void * data_to_copy, int bytes_to_copy){
	int i = 0;
	char * destination;
	char * source;
	for(i = 0; i < bytes_to_copy; i++){
		assert(MESSAGE_DATA_OFFSET + i <= MEMORY_BLOCK_SIZE, "Attempt to write outside the range of a memory block.");
		destination = (char *) p_message + MESSAGE_DATA_OFFSET + i;
		source = (char *)data_to_copy + i;
		*destination = *source;
	}
}


