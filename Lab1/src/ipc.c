#include "utils.h"
#include "ipc.h"
#include "process.h"
#include "memory.h"

#define KERNEL_POINTERS_OFFSET                0
#define SENDER_PID_OFFSET                     KERNEL_POINTERS_OFFSET + sizeof(ListNode)
#define DESTINATION_PID_OFFSET                SENDER_PID_OFFSET + sizeof(int)
#define MESSAGE_TYPE_OFFSET                   DESTINATION_PID_OFFSET + sizeof(int)
#define MESSAGE_DATA_OFFSET                   MESSAGE_TYPE_OFFSET + sizeof(int)

extern int numMessagesSent = 0;
extern int numMessagesReceived = 0;
TrackedEnvelope recentlySentMessages[NUM_MESSAGES_TO_TRACK];
TrackedEnvelope recentlyReceivedMessages[NUM_MESSAGES_TO_TRACK];

void copyEnvelope(TrackedEnvelope * toCopy, Envelope * fromCopy) {
	int i = 0;
	toCopy->trackedEnvelope.sender_pid = fromCopy->sender_pid;
	toCopy->trackedEnvelope.receiver_pid = fromCopy->receiver_pid;
	toCopy->trackedEnvelope.message_type = fromCopy->message_type;
	toCopy->trackedEnvelope.message_data = fromCopy->message_data;
	for(i = 0; i < MEMORY_BLOCK_SIZE; i++){
		unsigned char c1 = ((unsigned char *)fromCopy)[i];
		(toCopy->savedData)[i] = c1;	
	}
}

void trackSentMessage(Envelope * env) {
	if(env == NULL) {
		return;
	}

	if(numMessagesSent < NUM_MESSAGES_TO_TRACK) {
		copyEnvelope(&recentlySentMessages[numMessagesSent], env);
	 	numMessagesSent++;
	} else {
		int i;

		for(i = 1; i < NUM_MESSAGES_TO_TRACK; ++i) {
			recentlySentMessages[i - 1] = recentlySentMessages[i]; //Shift all left	
		}

		copyEnvelope(&recentlySentMessages[NUM_MESSAGES_TO_TRACK - 1], env); //Append new message
	}	
}	

void trackReceivedMessage(Envelope * env) {
	if(env == NULL) {
		return;
	}

	if(numMessagesReceived < NUM_MESSAGES_TO_TRACK) {
		copyEnvelope(&recentlyReceivedMessages[numMessagesReceived], env);
	 	numMessagesReceived++;
	} else {
		int i;

		for(i = 1; i < NUM_MESSAGES_TO_TRACK; ++i) {
			recentlyReceivedMessages[i - 1] = recentlyReceivedMessages[i]; //Shift all left	
		}

		copyEnvelope(&recentlyReceivedMessages[NUM_MESSAGES_TO_TRACK - 1], env); //Append new message
	}	
}




// ------------------------------------------------------------
//                 Kernel primitives (user-facing API)
// ------------------------------------------------------------
int k_send_message(int target_pid, void* envelope) {
	Envelope* env = (Envelope*) envelope;
	ListNode* node = &env->node_pointer;
	ProcessControlBlock* targetProcess = get_process_pointer_from_id(target_pid);

	if (env->sender_pid == NULL) {
		env->sender_pid = pCurrentProcessPCB->processId;
	}

	if (env->receiver_pid == NULL) {
		env->receiver_pid = target_pid;
	}

	//add envelope to the message queue of Target Process
	node->data = env;
	enqueue(&(targetProcess->waitingMessages), node);

	if (targetProcess->currentState == BLOCKED_ON_RECEIVE) {
	 	targetProcess->currentState = RDY;
		
		//remove process from BLOCKED_ON_RECEIVE queue
		remove_node(&blocked_receive_queue[targetProcess->processPriority], (void*)targetProcess);
		
		// i-procs don't get blocked on receive; since sys procs are part of normal queues enqueue them as well
		if ( !is_i_proc(targetProcess->processId)) {
		//if(is_usr_proc(targetProcess->processId)){
			enqueue(
				&ready_queue[targetProcess->processPriority],
				get_node_of_process(targetProcess->processId)
			);
		}
	}

	// Prempt to a user proc if he has higher priority OR if it is a user proc
	// Should also preempt to system processes....

	if (((targetProcess->processPriority < pCurrentProcessPCB->processPriority && is_usr_proc(targetProcess->processId)))
		&& is_ready_or_new(targetProcess->currentState))
	{
		context_switch(pCurrentProcessPCB, targetProcess);
	}

	trackSentMessage(env);
	return 0;
}

void* k_receive_message(int* sender_ID) {
	Envelope* env = NULL;
	ListNode* node;
	
	//don't want to block iprocesses (block only user procs pid 0 to 6, and sys procs pid 12 onward)
	//insert into blocked_on_receive queue
	if (pCurrentProcessPCB->waitingMessages.head == NULL && !is_i_proc(pCurrentProcessPCB->processId)) {
		enqueue(&blocked_receive_queue[pCurrentProcessPCB->processPriority], 
				get_node_of_process(pCurrentProcessPCB->processId));
	}

	//send_message removes the process from the blocked on receive queue
	while (pCurrentProcessPCB->waitingMessages.head == NULL && !is_i_proc(pCurrentProcessPCB->processId)) {
	 	pCurrentProcessPCB->currentState = BLOCKED_ON_RECEIVE;
		k_release_processor();
	}
	

	node = dequeue(&pCurrentProcessPCB->waitingMessages);
	if (node != NULL) {
		env = (Envelope*)node->data;
		if (sender_ID != NULL) {
			*sender_ID = env->sender_pid;
		}
	}

	trackReceivedMessage(env);
	return env;	
}

// -----------------------------------------------------------

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

void set_message_bytes(void * p_message, void * data_to_copy, int bytes_to_copy){
	int i = 0;
	char * destination;
	char * source;
	assert(MESSAGE_DATA_OFFSET + bytes_to_copy <= MEMORY_BLOCK_SIZE,
		"Attempt to write outside the range of a memory block.");
	for(i = 0; i < bytes_to_copy; i++){
		destination = (char *) p_message + MESSAGE_DATA_OFFSET + i;
		source = (char *)data_to_copy + i;
		*destination = *source;
	}
}

void set_message_words(void* p_message, void* data_to_copy, int words_to_copy){
	int i = 0;
	int word_size = sizeof(uint32_t);
	uint32_t * destination;									
	uint32_t * source;
	assert(MESSAGE_DATA_OFFSET + word_size * words_to_copy <= MEMORY_BLOCK_SIZE,
		"Attempt to write outside the range of a memory block.");
	for(i = 0; i < words_to_copy; i++){
		destination = (uint32_t *) p_message + i + (MESSAGE_DATA_OFFSET) / word_size;
		source = (uint32_t *)data_to_copy + i;
		*destination = *source;
	}
}
