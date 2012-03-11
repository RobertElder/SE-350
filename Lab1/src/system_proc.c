#include "system_proc.h"
#include "rtx.h"
#include "uart.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"


ProcessControlBlock crt_pcb;
ProcessControlBlock kcd_pcb;
ProcessControlBlock clock_pcb;

ProcessControlBlock* get_crt_pcb() {
	return &crt_pcb;
}

ProcessControlBlock* get_kcd_pcb() {
	return &kcd_pcb;
}

ProcessControlBlock* get_clock_pcb() {
	return &clock_pcb;
}

ProcessControlBlock* get_new_sys_proc() {
	if (crt_pcb.currentState == NEW) return &crt_pcb;
	if (kcd_pcb.currentState == NEW) return &kcd_pcb;
	if (clock_pcb.currentState == NEW) return &clock_pcb;
	return NULL;
}

ProcessControlBlock* get_waiting_sys_proc() {
 	if (kcd_pcb.waitingMessages.head != NULL) return &kcd_pcb;
	if (crt_pcb.waitingMessages.head != NULL) return &crt_pcb;
	if (clock_pcb.waitingMessages.head != NULL) return &clock_pcb;
	return NULL;
}

int number_of_registered_commands = 0;
char registered_commands[MAX_NUMBER_OF_REGISTERABLE_COMMANDS][MAX_COMMAND_LENGTH];
int registered_processes[MAX_NUMBER_OF_REGISTERABLE_COMMANDS];

char current_command_buffer[MAX_COMMAND_LENGTH];
int current_command_length = 0;

char time[] = "00:00:00";

volatile extern uint8_t g_UART0_TX_empty;

void unregister_all_commands(){
	number_of_registered_commands = 0;
}

void register_command(char * s, int process_id) {
	int i = 0;

	registered_processes[number_of_registered_commands] = process_id;
	for(i = 0; i < MAX_COMMAND_LENGTH; i++){
		assert(i < MAX_COMMAND_LENGTH, "Invalid attempt to register a command that is too long.");

		registered_commands[number_of_registered_commands][i]  = s[i];
		//  We want to copy everything up to and including the null
		if(s[i] == 0)
			break;

	}
	assert(number_of_registered_commands < MAX_NUMBER_OF_REGISTERABLE_COMMANDS, "ERROR: Too many commands registered");
	number_of_registered_commands++;
								
}

int get_index_of_matching_command(){
	/*  Returns the first index of a complete command that
	 exists as a prefix of the current command buffer
	 */
	int i = 0;
	int j = 0;
 	for (i = 0; i < number_of_registered_commands; i++) {
		for (j = 0; j < MAX_NUMBER_OF_REGISTERABLE_COMMANDS; j++) {
			if(registered_commands[i][j] == 0){
				//  We are at the end of the command so it must have matched so far.
				assert(j > 0,"We just matched an empty command.  This is probably not right.");
				return i;
			} else if(j >= current_command_length) {
				//  We reached the end of the command buffer, there is nothing to check
				break;
			} else if(!(registered_commands[i][j] == current_command_buffer[j])) {
				// This command did not match
				break;
			}	
		}
	}
	//  No matching command found
	return -1;
} 

// ------------------------------------------
//     System Processes         
// ------------------------------------------ 

void keyboard_command_decoder(){
	while (1) {
		int sender_id = -1;
		int destination = -1;

		// Get our new message
		Envelope* message = (Envelope*)receive_message(&sender_id);
		char * pChar = get_message_data(message);
		int message_type = get_message_type(message);
		destination = get_destination_PID(message);

		assert(sender_id == message->sender_pid,
			 "ERROR: receive_message did not supply sender_id");
		assert(destination == kcd_pcb.processId,
			 "ERROR: Message destination did not match with KCD pid");

		switch(message_type) {
			case COMMAND_REGISTRATION:
				register_command(pChar, destination);
				release_memory_block(message); 
				break;
			case KEYBOARD_INPUT:
				/* If the buffer is full, they are not part of any valid command so 
				   we don't care (also < because we want space for the terminating null)
				*/
				if(current_command_length < MAX_COMMAND_LENGTH){
					//  Put the character we received into the buffer
					current_command_buffer[current_command_length] = *pChar;
					current_command_length++;
				}
			
				// Did they type a carriage return?
				if(*pChar == 0xD){
					int matched_command = get_index_of_matching_command();
			
					//  Does the thing in the buffer match a command that was registered? 
					if(matched_command > -1){
					 	//  The user type a command that we recognized
						current_command_buffer[current_command_length] = 0;
						uart0_put_string("A command was matched:\r\n");
						uart0_put_string((unsigned char *)&current_command_buffer);
						uart0_put_string("\r\n");
					}
			
					// Reset the buffer for new commands
					current_command_length = 0;
				}
	
				//edit msg and forward to crt to echo
				message->sender_pid = get_kcd_pcb()->processId;
				message->receiver_pid = get_crt_pcb()->processId;
				message->message_type = OUTPUT_STRING;
				send_message(get_crt_pcb()->processId, message);
				break;
			default:
				assert(0, "ERROR, invalid message sent to KCD");
				break;		
		}
	}		
}

void crt_display(){
	while (1) {
		int sender_id = -1;
		int destination = -1;
		LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;

		Envelope* message = (Envelope*)receive_message(&sender_id);

		uint8_t * current_character = get_message_data(message);

		assert(message != NULL, "ERROR: CRT received a NULL message");

		destination = get_destination_PID(message);

		assert(sender_id == message->sender_pid,
			"ERROR(crt): receive_message did not supply sender_id");
		assert(destination == crt_pcb.processId,
			"ERROR: Message destination did not match with CRT pid");

		LPC_UART0->IER = IER_THR_Empty | IER_Receive_Line_Status;
		// Our message data should be a null terminated string
	    while ( *current_character != 0 ) {
		    // THRE status, contain valid data  
		    while ( !(g_UART0_TX_empty & 0x01) );
			//  Setting this value makes the interrupt fire at a later time	
		    pUart->THR = *current_character;
			g_UART0_TX_empty = 0;  // not empty in the THR until it shifts out
		    current_character++;
	    }
		LPC_UART0->IER = IER_THR_Empty | IER_Receive_Line_Status | IER_Receive_Data_Available;	// Re-enable IER_Receive_Data_Available		

		// We don't want that memory block anymore
		release_memory_block(message);
	}
}

void wall_clock() {
	int doCount = 1;
	int displayClock = 1;
	int clock_time = 0;
	int* sender_id;
	Envelope * env;

	while(1) {
	// TODO: RELEASE ME PROPERLY (after uncommenting and fixing clock_tick case)
		env = (Envelope *)receive_message(sender_id);
		release_memory_block(env); // temporary fix. Remove this line once proc is fixed
		switch(env->message_type) {
			case CLOCK_TICK:
				if(doCount) {
					clock_time++; //tick

					if(displayClock) {
						char* time_string = get_formatted_time_from_seconds(clock_time);

						//release_memory_block(env);
						/*env = (Envelope *)request_memory_block_debug(0xd);

						set_sender_PID(env, get_clock_pcb()->processId);
						set_destination_PID(env, get_kcd_pcb()->processId);
						set_message_type(env, OUTPUT_STRING);
						set_message_bytes(env, time_string, sizeof(time_string));

						k_send_message(get_kcd_pcb()->processId, env); //to KCD for display	  */
					}
				}
				break;
			case STOP_CLOCK:
				doCount = 0;
				break;
			case START_CLOCK:
				clock_time = (int)env->message_data;	
				doCount = displayClock = 1;
				break;
			case PAUSE_CLOCK:
				displayClock = 0;
				break;
			case UNPAUSE_CLOCK:
				displayClock = 1;
				break;
			default:
				break;
		}

		//release_memory_block(env); //commented until this proc is fixed
	}
}

// --------------------------------------------------------

void init_sys_procs() {
	int procIndex;

	uint32_t* sp;
	uint32_t* stacks_start[3];

	ProcessControlBlock* sys_procs[3];	 
	uint32_t* funcPointers[] = {  
		(uint32_t*)crt_display,
		(uint32_t*)keyboard_command_decoder,
		(uint32_t*)wall_clock
	};

	sys_procs[0] = &crt_pcb;
	sys_procs[1] = &kcd_pcb;
	sys_procs[2] = &clock_pcb;

	stacks_start[0] = (uint32_t*)(START_STACKS + (NUM_USR_PROCESSES + NUM_I_PROCESSES) * STACKS_SIZE + STACKS_SIZE);
	stacks_start[1] = stacks_start[0] + (STACKS_SIZE) / sizeof(uint32_t);
	stacks_start[2] = stacks_start[1] + (STACKS_SIZE) / sizeof(uint32_t);

	for (procIndex = 0; procIndex < 3; procIndex++) {
		int i;
		sys_procs[procIndex]->processId = 0xC + i;
		sys_procs[procIndex]->currentState = NEW;
		sys_procs[procIndex]->waitingMessages.head = NULL;
		sys_procs[procIndex]->waitingMessages.tail = NULL;
		sys_procs[procIndex]->processPriority = 0;

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
		
		sys_procs[procIndex]->processStackPointer = sp;
	}
}

int get_seconds_from_formatted_time(char *c){
	int h1 = (c[0] - 0x30) * 10 * 60 * 60;	
	int h2 = (c[1] - 0x30) * 60 * 60;	
	int m1 = (c[3] - 0x30) * 10 * 60;	
	int m2 = (c[4] - 0x30) * 60;
	int s1 = (c[6] - 0x30) * 10;	
	int s2 = c[7] - 0x30;
	
	return h1 + h2 + m1 + m2 + s1 + s2;	
}

char* get_formatted_time_from_seconds(int seconds) {
	int h, m, s;

	h = (int)(seconds / 3600);
	seconds -= (h * 3600);
	m = (int)(seconds / 60);
	seconds -= (m * 60);
	s = seconds;

	time[0] = (h / 10) + 0x30;
	time[1] = (h % 10) + 0x30;
	time[3] = (m / 10) + 0x30;
	time[4] = (m % 10) + 0x30;
	time[6] = (s / 10) + 0x30;
	time[7] = (s % 10) + 0x30;
	
	return time; 
}
