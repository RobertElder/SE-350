#include "rtx.h"
#include "timer.h"
#include "ipc.h"

LinkedList delayed_messages;

typedef struct delayedMessage {
	int pid;
	Envelope * envelope;
	int expiry_time;
} DelayedMessage;

extern volatile uint32_t g_timer_count;

uint32_t get_current_time() {
	return g_timer_count;
}

void expiry_sorted_enqueue(LinkedList* listHead, ListNode* node) {
	ListNode* currentNode = listHead->head;

	if(currentNode == NULL) {
	 	(* listHead).head = node;
		(* listHead).tail = node;
	} 
	else 
	{

	 	while(currentNode != NULL) {
			if(currentNode->next == NULL) {
			 	(* currentNode).next = node;
				(* listHead).tail = node;
				return;
			} 
			else if (((DelayedMessage *)currentNode->data)->expiry_time < ((DelayedMessage *)node->data)->expiry_time) {
				(* node).next = currentNode->next;
				(* currentNode).next = node;	
				return;			
			}		 	
		}

		currentNode = currentNode->next;
	}

	return;
}

int delayed_send(int pid, Envelope * envelope, int delay) {
	DelayedMessage * m;
	ListNode * node;

	m->pid = pid;
	m->envelope = envelope;
	m->expiry_time = get_current_time() + delay;
	
	node->data = &m;
	node->next = NULL;

	//add node containing m to timeoutProcess message queue
	expiry_sorted_enqueue(&delayed_messages, node);
	//send_message(pid, env) //send itself a message??

	return 0; //What should the return value be?
}

void timeout_i_process() {
	Envelope * env = receive_message(NULL);
	ListNode * node;
	int receiver_pid;

	while(env != NULL) {
		node->data = &(env->message_data); //Get DelayedMessage data out of envelope
		node->next = NULL;

	 	expiry_sorted_enqueue(&delayed_messages, node);

		env = receive_message(NULL);
	}

	if(delayed_messages.head != NULL) {
		while (((DelayedMessage *)(delayed_messages.head->data))->expiry_time <= get_current_time())
		{
			env = ((DelayedMessage *)dequeue(&delayed_messages)->data)->envelope;
			receiver_pid = env->receiver_pid;
			send_message( receiver_pid, env ); //forward msg to destination
		}						
	}

}
