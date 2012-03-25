#include "rtx.h"
#include "timer.h"
#include "ipc.h"
#include "iprocess.h"
#include "process.h"
#include "utils.h"
#include "memory.h"
#include "uart.h"
#include "system_proc.h"
#include "uart_polling.h"

LinkedList delayed_messages;
ProcessControlBlock i_uart_pcb;
ProcessControlBlock i_timer_pcb;

ProcessControlBlock* get_uart_pcb() {
	return &i_uart_pcb;
}

ProcessControlBlock* get_timer_pcb() {
	return &i_timer_pcb;
}

typedef struct delayedMessage {
	int pid;
	Envelope * envelope;
	int expiry_time;
} DelayedMessage;

void expiry_sorted_enqueue(LinkedList* listHead, ListNode* node) {
	ListNode* currentNode = listHead->head;
	int node_expiry = ((DelayedMessage *)node->data)->expiry_time;

	if(currentNode == NULL) {
	 	(* listHead).head = node;
		(* listHead).tail = node;
		node->next = NULL;
	} else if (((DelayedMessage *)currentNode->data)->expiry_time > node_expiry) {
		(*node).next = listHead->head;
		(* listHead).head = node;		 
		goto expiry_sorted_enqueue_error_check;
	} else {
	 	while(currentNode != NULL) {
			if(currentNode->next == NULL) {
			 	(* currentNode).next = node;
				(* listHead).tail = node;
				node->next = NULL;
				goto expiry_sorted_enqueue_error_check;
			} 
			else if (((DelayedMessage *)currentNode->next->data)->expiry_time > node_expiry) 
			{
				(* node).next = currentNode->next;
				(* currentNode).next = node;	
				goto expiry_sorted_enqueue_error_check;			
			}		 	

			currentNode = currentNode->next;
		}		
	}

expiry_sorted_enqueue_error_check:
	assert(node != node->next,"ERROR node that was enqueued had a circular reference.");

	return;
}

int k_delayed_send(int pid, Envelope * envelope, int delay) {
	DelayedMessage m;
	Envelope * env;

	// If this is an iprocess, we cannot block
	if(numberOfMemoryBlocksCurrentlyAllocated == MAX_ALLOWED_MEMORY_BLOCKS && is_i_proc(pCurrentProcessPCB->processId)){
		uart0_polling_put_string("Insufficient memory: Unable to create envelope for delayed send.");
		return -1;	
	}
   	env = (Envelope *)k_request_memory_block();

	m.pid = pid;
	m.envelope = envelope;
	m.expiry_time = get_current_time() + delay/TIMEMULTIPLIER; //Delay is in ms, TIMEMULTIPLIER compensates for simulator

	set_sender_PID(env, pid);  //sender should be original sender
	set_destination_PID(env, get_timer_pcb()->processId);
	set_message_type(env, DELAYED_SEND);
	set_message_words(env, &m, sizeof(m) / sizeof(uint32_t));

	//add node containing m to timeoutProcess message queue
	//expiry_sorted_enqueue(&delayed_messages, node);
	k_send_message(get_timer_pcb()->processId, env); //send message to timeout_i_process

	return 0; //What should the return value be?
}

void timeout_i_process() {
	while(1) {
		int senderId = 0;
		ProcessControlBlock* interrupted_proc = get_interrupted_process();
		
		Envelope * env = k_receive_message(&senderId);
		ListNode * node = &env->node_pointer;
		int receiver_pid;
	
		while(env != NULL) {
			node->data = &(env->message_data); //Get DelayedMessage data out of envelope
			node->next = NULL;
	
		 	expiry_sorted_enqueue(&delayed_messages, node);
	
			env = k_receive_message(&senderId);
		}
	
		if(delayed_messages.head != NULL) {
			while (((DelayedMessage *)(delayed_messages.head->data))->expiry_time
				 <= get_current_time())
			{
				ListNode* node = dequeue(&delayed_messages);
				Envelope* envelope = ((DelayedMessage *)node->data)->envelope;
				receiver_pid = envelope->receiver_pid;
				k_send_message( receiver_pid, envelope ); //forward msg to destination
				k_release_memory_block(node);
			}
		} 
		context_switch(pCurrentProcessPCB, interrupted_proc);
	}
}

void uart0_i_process() {
	while(1) {
		ProcessControlBlock* interrupted_proc = get_interrupted_process();
		assert(interrupted_proc != NULL,"ERROR, trying to switch to a null process.");
		execute_uart();
		context_switch(pCurrentProcessPCB, interrupted_proc);
	}
}

// Initiate iprocesses and their stacks
void init_i_processes() {
	int procIndex;

	uint32_t *sp;
	uint32_t* stacks_start[2];

	ProcessControlBlock* iproc[2];	 
	uint32_t* funcPointers[] = {  (uint32_t*)uart0_i_process,
		 (uint32_t*)timeout_i_process };

	iproc[0] = &i_uart_pcb;
	iproc[1] = &i_timer_pcb;
	stacks_start[0] = UART_START_STACK;
	stacks_start[1] = TIMER_START_STACK;

	//get_process_pointer_from_id(NUM_USR_PROCESSES - 1)->processStackPointer;

	for (procIndex = 0; procIndex < 2; procIndex++) {
		int i;
		iproc[procIndex]->processId = 10 + i;
		iproc[procIndex]->currentState = NEW;
		iproc[procIndex]->waitingMessages.head = NULL;
		iproc[procIndex]->waitingMessages.tail = NULL;
		iproc[procIndex]->processPriority = 0;
	
		sp = stacks_start[procIndex];
	
		if (!(((uint32_t)sp) & 0x04)) {
		    --sp; 
		}
														   
		*(--sp) = INITIAL_xPSR;      // user process initial xPSR  
	
		// Set the entry point of the process
		*(--sp) = (uint32_t) funcPointers[procIndex];
		
		for (i = 0; i < 6; i++) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		
		iproc[procIndex]->processStackPointer = sp;
	}
}
