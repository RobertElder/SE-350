#include "system_proc.h"
#include "rtx.h"
#include "uart.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "hot_keys.h"
#include "ipc.h"

/*
  As well, the UART i-process is used to provide debugging services which will be used during the demonstration. Upon receiving
specific characters (hot keys - your choice, e.g., !) as input, the UART i-process will print the following to the RTX system
console:
1. The processes currently on the ready queue(s) and their priority.
2. The processes currently on the blocked on memory queue(s) and their priorities.
3. The processes currently on the blocked on receive queue(s) and their priorities.
As well, you are free to implement other hot keys to help in debugging. For example, a hot key which lists the processes, their
priorities, their states; or another which prints out the number of memory blocks available. Like all other debug prints, the hot
key implementation should be wrapped in
#ifdef _DEBUG_HOTKEYS
...
#endif
preprocessor statements and should be turned off during automated testing. If the automated test processes fail, you may be
asked to turn the hot keys on again in determining why the test processes are failing.
Another hotkey debug printout may be used to display recent interprocess message passing. A (circular) log buffer keeps track
of the 10 most recent send_message and receive_message invocations made by the processes; upon receiving a specific
hotkey, these most recent10 sent and 10 received messages are printed to the debug console. The number 10 is used only
as an example. The information printed could contain information such as:
1. Sender process id
2. Destination process id
3. Message type
4. First 16 bytes of the message text
5. The time stamp of the transaction (using the RTX clock)



*/

//1. The processes currently on the ready queue(s) and their priority.
/*
extern LinkedList ready_queue[NUM_PRIORITIES];
extern LinkedList blocked_memory_queue[NUM_PRIORITIES];
extern LinkedList blocked_receive_queue[NUM_PRIORITIES];

typedef struct pcb {
  LinkedList waitingMessages;

  // stack pointer of the process
  uint32_t* processStackPointer;

  // process id      
  uint32_t processId;

  // state of the process 		
  proc_state_t currentState;

  //Priority of the process  
  uint32_t processPriority;      

} ProcessControlBlock;
*/
void do_print_processes(LinkedList linkedListArray[],unsigned char * queueName){
	int i = 0;
	uart0_put_string_emergency("\n-- ");
	uart0_put_string_emergency(queueName);
	uart0_put_string_emergency(" process queues (by priority) --\r\n");
	for(i = 0; i < NUM_PRIORITIES; i++){
		ListNode * current_ready_queue_node = linkedListArray[i].head;
		uart0_put_string_emergency("Priority ");
	    print_unsigned_integer(i);
		uart0_put_string_emergency("->  ");
		while(current_ready_queue_node){
			ProcessControlBlock* currentPCB = current_ready_queue_node->data;
			uart0_put_string_emergency("Id: ");
			print_unsigned_integer(currentPCB->processId);
			uart0_put_string_emergency(" .. State: ");
			print_unsigned_integer(currentPCB->currentState);
			uart0_put_string_emergency(" .. Priority: ");
			print_unsigned_integer(currentPCB->processPriority);
			uart0_put_string_emergency(" || ");
			current_ready_queue_node = current_ready_queue_node->next;
		}
		uart0_put_string_emergency("\r\n__________________________________________\r\n");
	}	

}

void do_print_messages() {
	int i = 0;
	uart0_put_string_emergency("\n-- Recently sent messages --");
	for(i = 0; i < numMessagesSent; ++i) {
		uart0_put_string_emergency("\nSender ID: ");
		print_unsigned_integer(recentlySentMessages[i].sender_pid);
		
		uart0_put_string_emergency("\nReceiver ID: ");
		print_unsigned_integer(recentlySentMessages[i].receiver_pid);

		uart0_put_string_emergency("\nMessage type: ");
		print_unsigned_integer(recentlySentMessages[i].message_type);

		uart0_put_string_emergency("\n__________________________________________\n");
	}

	uart0_put_string_emergency("\n-- Recently received messages --");
	for(i = 0; i < numMessagesReceived; ++i) {
		uart0_put_string_emergency("\nSender ID: ");
		print_unsigned_integer(recentlyReceivedMessages[i].sender_pid);
		
		uart0_put_string_emergency("\nReceiver ID: ");
		print_unsigned_integer(recentlyReceivedMessages[i].receiver_pid);

		uart0_put_string_emergency("\nMessage type: ");
		print_unsigned_integer(recentlyReceivedMessages[i].message_type);

	   	uart0_put_string_emergency("\n__________________________________________\n");
	}

}

void do_hot_key(char c){
	#ifdef _DEBUG_HOTKEYS
	switch(c){

		case '!' :{
			do_print_processes(ready_queue,"Ready");
			break;
		}case '@' :{
			do_print_processes(blocked_memory_queue,"Blocked On Memory");
			break;
		}case '#' :{
			do_print_processes(blocked_receive_queue,"Blocked On Receive");
			break;
		}
		case '~' : {
			do_print_messages();
			break;
		}
		default: {
			return;

		}
	}
	#endif
}
