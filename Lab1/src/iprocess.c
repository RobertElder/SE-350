#include "rtx.h"
#include "timer.h"
#include "ipc.h"
#include "iprocess.h"
#include "process.h"
#include "utils.h"
#include "memory.h"
#include "uart.h"


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

	if(currentNode == NULL) {
	 	(* listHead).head = node;
		(* listHead).tail = node;
	} else {

	 	while(currentNode != NULL) {
			if(currentNode->next == NULL) {
			 	(* currentNode).next = node;
				(* listHead).tail = node;
				return;
			} else if (((DelayedMessage *)currentNode->data)->expiry_time 
				< ((DelayedMessage *)node->data)->expiry_time) 
			{
				(* node).next = currentNode->next;
				(* currentNode).next = node;	
				return;			
			}		 	
		}

		currentNode = currentNode->next;
	}

	return;
}

int k_delayed_send(int pid, Envelope * envelope, int delay) {
	DelayedMessage m;
	Envelope * env = (Envelope *)k_request_memory_block();

	m.pid = pid;
	m.envelope = envelope;
	m.expiry_time = get_current_time() + delay;

	set_sender_PID(env, 11);  //sender should be original sender
	set_destination_PID(env, 11);								 //dont harcode 11 like noob
	set_message_type(env, DELAYED_SEND);
	set_message_words(env, &m, sizeof(m) / sizeof(uint32_t));

	//add node containing m to timeoutProcess message queue
	//expiry_sorted_enqueue(&delayed_messages, node);
	k_send_message(11, env); //send message to timeout_i_process

	return 0; //What should the return value be?
}

void timeout_i_process() {
	int i = 0;
	while(1) {
		int* senderId;
		ProcessControlBlock* interrupted_proc = get_interrupted_process();
		
		Envelope * env = k_receive_message(senderId);
		ListNode * node = &env->dummyVar;
		int receiver_pid;
	
		while(env != NULL) {
			node->data = &(env->message_data); //Get DelayedMessage data out of envelope
			node->next = NULL;
	
		 	expiry_sorted_enqueue(&delayed_messages, node);
	
			env = k_receive_message(senderId);
		}
	
		if(delayed_messages.head != NULL) {
			while (((DelayedMessage *)(delayed_messages.head->data))->expiry_time
				 <= get_current_time())
			{
				Envelope* envelope = ((DelayedMessage *)dequeue(&delayed_messages)->data)->envelope;
				receiver_pid = envelope->receiver_pid;
				k_send_message( receiver_pid, envelope ); //forward msg to destination
			}						
		} 	 
		i++;
		context_switch(pCurrentProcessPCB, interrupted_proc);
	}
}

void uart0_i_process() {
	int i = 0;
	while(1) {
		ProcessControlBlock* interrupted_proc = get_interrupted_process();
		execute_uart();
		i++;
		context_switch(pCurrentProcessPCB, interrupted_proc);
	}
}

// Initiate iprocesses and their stacks
void init_i_processes() {
	int procIndex;
	uint32_t *sp;
	ProcessControlBlock* iproc[2];	 
	uint32_t* funcPointers[] = {  (uint32_t*)uart0_i_process,
		 (uint32_t*)timeout_i_process };
	iproc[0] = &i_uart_pcb;
	iproc[1] =  &i_timer_pcb ;

	sp = get_process_pointer_from_id(NUM_PROCESSES - 1)->processStackPointer;

	for (procIndex = 0; procIndex < 2; procIndex++) {
		int i;
		iproc[procIndex]->processId = 10 + i;
		iproc[procIndex]->currentState = NEW;
		iproc[procIndex]->waitingMessages.head = NULL;
		iproc[procIndex]->waitingMessages.tail = NULL;
		iproc[procIndex]->processPriority = 0;
	
		sp += STACKS_SIZE;
	
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
