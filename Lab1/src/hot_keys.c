#include "system_proc.h"
#include "rtx.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "hot_keys.h"
#include "ipc.h"
#include "uart_polling.h"

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
void do_print_process(ProcessControlBlock* pcb, unsigned char * message) {
	uart0_polling_put_string("\r\n-- ");
	uart0_polling_put_string(message);
	uart0_polling_put_string("\r\n");
	uart0_polling_put_string("+================================================+\r\n");
	if(pcb != NULL) {
		uart0_polling_put_string("| ->  Id: ");
		print_unsigned_integer(pcb->processId);
		uart0_polling_put_string(" .. State: ");
		print_unsigned_integer(pcb->currentState);
		uart0_polling_put_string(" .. Priority: ");
		print_unsigned_integer(pcb->processPriority);
		uart0_polling_put_string("           |\r\n");
	} else {
		uart0_polling_put_string("    No Process found.                            |\r\n");	
	}
	uart0_polling_put_string("+================================================+\r\n");
}

void do_print_processes(LinkedList linkedListArray[],unsigned char * queueName){
	int i = 0;
	uart0_polling_put_string("\r\n-- ");
	uart0_polling_put_string(queueName);
	uart0_polling_put_string(" process queues (by priority) --\r\n");
	uart0_polling_put_string("+================================================+\r\n");
	for(i = 0; i < NUM_PRIORITIES; i++){
		ListNode * current_ready_queue_node = linkedListArray[i].head;
		uart0_polling_put_string("| Priority ");
	    print_unsigned_integer(i);
		uart0_polling_put_string(" Queue:                              |\r\n");
		if(current_ready_queue_node){
			while(current_ready_queue_node){
				ProcessControlBlock* currentPCB = current_ready_queue_node->data;
				uart0_polling_put_string("| ->  Id: ");
				print_unsigned_integer(currentPCB->processId);
				uart0_polling_put_string(" .. State: ");
				print_unsigned_integer(currentPCB->currentState);
				uart0_polling_put_string(" .. Priority: ");
				print_unsigned_integer(currentPCB->processPriority);
				uart0_polling_put_string("           |\r\n");
				current_ready_queue_node = current_ready_queue_node->next;
			}
		}else{
			uart0_polling_put_string("|   Queue is empty.                              |\r\n");	
		}
		uart0_polling_put_string("+================================================+\r\n");
	}	

}

void do_print_messages(TrackedEnvelope messagesToPrint[],unsigned char * typeName) {
	int i = 0;
	int j = 0;

	uart0_polling_put_string("\n-- Recently ");
	uart0_polling_put_string(typeName);
	uart0_polling_put_string(" messages --\r\n");
	uart0_polling_put_string("+================================================+\r\n");
	for(i = 0; i < numMessagesSent; ++i) {
		uart0_polling_put_string("|   Sender ID: ");
		print_unsigned_integer(messagesToPrint[i].trackedEnvelope.sender_pid);
		uart0_polling_put_string("                                 |\r\n");
		
		uart0_polling_put_string("|   Receiver ID: ");
		print_unsigned_integer(messagesToPrint[i].trackedEnvelope.receiver_pid);
		uart0_polling_put_string("                              |\r\n");

		uart0_polling_put_string("|   Message type: ");
		print_unsigned_integer(messagesToPrint[i].trackedEnvelope.message_type);
		uart0_polling_put_string("                              |\r\n");

		for(j = 0; j < MEMORY_BLOCK_SIZE; j++){
			if(j == 0){
				uart0_polling_put_string("|");
			}else if(j % 16 == 0)
				uart0_polling_put_string(" |\r\n|");
			else
				uart0_polling_put_string(" ");

			print_hex_byte(messagesToPrint[i].savedData[j]);

		}
		uart0_polling_put_string(" |\r\n+------------------------------------------------+\r\n");
		for(j = 0; j < MEMORY_BLOCK_SIZE; j++){
			if(j == 0){
				uart0_polling_put_string("|");
			}else if(j % 16 == 0)
				uart0_polling_put_string(" |\r\n|");
			else
				uart0_polling_put_string(" ");

			print_printable_character(messagesToPrint[i].savedData[j]);
		}
		uart0_polling_put_string(" |\r\n");
		uart0_polling_put_string("+================================================+\r\n");
	}
}

uint8_t do_hot_key(char c){
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
		}case '$' :{
			do_print_process(pCurrentProcessPCB, "Currently Running Process:");
			do_print_process(get_interrupted_process(), "Currently Interrupted Process:");
			break;
		}
		case '~' : {
			do_print_messages(recentlySentMessages,"sent");
			do_print_messages(recentlyReceivedMessages,"received");
			break;
		}
		default: {
			return 0;
		}
	}
	
	uart0_polling_put_string("Press Enter to continue.");
	while (1) {
		char c = uart0_get_char();
		if (c == 0xD) return 1;
	}
	#else
		return 0;
	#endif
}
