#include "system_proc.h"
#include "rtx.h"
#include "uart.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "iprocess.h"
#include "ipc.h"
#include "hot_keys.h"


//ProcessControlBlock crt_pcb;
//ProcessControlBlock kcd_pcb;
//ProcessControlBlock clock_pcb;


ProcessControlBlock* get_new_sys_proc() {
	//if (crt_pcb.currentState == NEW) return &crt_pcb;
	//if (kcd_pcb.currentState == NEW) return &kcd_pcb;
	//if (clock_pcb.currentState == NEW) return &clock_pcb;
	return NULL;
}

ProcessControlBlock* get_waiting_sys_proc() {
 	//if (kcd_pcb.waitingMessages.head != NULL) return &kcd_pcb;
	//if (crt_pcb.waitingMessages.head != NULL) return &crt_pcb;
	//if (clock_pcb.waitingMessages.head != NULL) return &clock_pcb;
	return NULL;
}

typedef struct cmd {
	uint32_t registered_pid;
	char command_string[MAX_COMMAND_LENGTH] ;
	uint32_t message_type;
} RegisteredCommand;

int number_of_registered_commands = 0;
RegisteredCommand registered_commands[MAX_NUMBER_OF_REGISTERABLE_COMMANDS];

char current_command_buffer[MAX_COMMAND_LENGTH];
int current_command_length = 0;

char time[] = "\r\n00:00:00\r\n";

volatile extern uint8_t g_UART0_TX_empty;

void unregister_all_commands(){
	number_of_registered_commands = 0;
}

void register_command(char * s, int process_id) {
	int i = 0;

	registered_commands[number_of_registered_commands].registered_pid = process_id;
	registered_commands[number_of_registered_commands].message_type = COMMAND_MATCHED;

	for(i = 0; i < MAX_COMMAND_LENGTH; i++){
		assert(i < MAX_COMMAND_LENGTH, "Invalid attempt to register a command that is too long.");

		registered_commands[number_of_registered_commands].command_string[i]  = s[i];

		//  We want to copy everything up to and including the null
		if(s[i] == 0) break;

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

 	for(i = 0; i < number_of_registered_commands; i++){
		for(j = 0; j < MAX_COMMAND_LENGTH; j++){
			char c = registered_commands[i].command_string[j];
			if(c == 0){
				//  We are at the end of the command so it must have matched so far.
				assert(j > 0,"We just matched an empty command.  This is probably not right.");

				return i;
			} else if(c != current_command_buffer[j] || j >= current_command_length) {
				//No match
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
		Envelope* message = (Envelope*)receive_message(&sender_id);	   // will get BLOCKED_ON_RECEIVE while no messages
		char * pChar = get_message_data(message);
		int message_type = get_message_type(message);
		destination = get_destination_PID(message);

		assert(sender_id == message->sender_pid,
			 "ERROR: receive_message did not supply sender_id");
		assert(destination == get_kcd_pcb()->processId,
			 "ERROR: Message destination did not match with KCD pid");

		switch(message_type) {
			case COMMAND_REGISTRATION:
				register_command(pChar, get_sender_PID(message));
				release_memory_block(message); 
				break;
			case KEYBOARD_INPUT:{
				//Init message to wall clock
				Envelope * clockMsg = (Envelope *)request_memory_block();
				set_sender_PID(clockMsg, get_kcd_pcb()->processId);
				set_destination_PID(clockMsg, get_clock_pcb()->processId);

				/* If the buffer is full, they are not part of any valid command so 
				   we don't care (also < because we want space for the terminating null)
				*/



				if(current_command_length < MAX_COMMAND_LENGTH){
					//  Put the character we received into the buffer
					current_command_buffer[current_command_length] = *pChar;
					current_command_length++;
				}

				do_hot_key(*pChar);
				// Did they type a carriage return?
				if(*pChar == 0xD){
					int indexOfMatchedCommand = get_index_of_matching_command();

					//  Does the thing in the buffer match a command that was registered? 
					if(indexOfMatchedCommand > -1) {
						Envelope * responseEnvelope = (Envelope *)request_memory_block();
						RegisteredCommand * registeredCommand = &registered_commands[indexOfMatchedCommand];

						current_command_buffer[current_command_length] = 0;

					   	//Send a message to the registered process
						set_sender_PID(responseEnvelope, get_kcd_pcb()->processId);
						set_destination_PID(responseEnvelope, registeredCommand->registered_pid);
						set_message_type(responseEnvelope, registeredCommand->message_type);
						set_message_bytes(responseEnvelope, &current_command_buffer, current_command_length * sizeof(char)); //Current buffer

						send_message(registeredCommand->registered_pid, responseEnvelope); //to registered process
					}
			
					// Reset the buffer for new commands
					current_command_length = 0;

					set_message_type(clockMsg, UNPAUSE_CLOCK);
				} else {
					set_message_type(clockMsg, PAUSE_CLOCK);
				}

				//Pause or resume clock
				send_message(get_clock_pcb()->processId, clockMsg);

			   	//Null terminate
				current_command_buffer[current_command_length] = 0;
	
				// edit msg and forward to crt to echo
				message->sender_pid = get_kcd_pcb()->processId;
				message->receiver_pid = get_crt_pcb()->processId;
				message->message_type = OUTPUT_STRING;
				send_message(get_crt_pcb()->processId, message);

				break;
			}
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

		Envelope* message = (Envelope*)receive_message(&sender_id);	 // if has none, gets BLOCKED_ON_RECEIVE
		// will continuously be blocked on receive - so user procs should be able to run

		uint8_t * current_character = get_message_data(message);

		assert(message != NULL, "ERROR: CRT received a NULL message");

		destination = get_destination_PID(message);

		assert(sender_id == message->sender_pid,
			"ERROR(crt): receive_message did not supply sender_id");
		assert(destination == get_crt_pcb()->processId,
			"ERROR: Message destination did not match with CRT pid");

		pUart->IER = IER_THR_Empty | IER_Receive_Line_Status;
		// Our message data should be a null terminated string
	    while ( *current_character != 0 ) {
		    // THRE status, contain valid data  
		    while ( !(g_UART0_TX_empty & 0x01) );
			//  Setting this value makes the interrupt fire at a later time	
		    pUart->THR = *current_character;
			g_UART0_TX_empty = 0;  // not empty in the THR until it shifts out
		    current_character++;
	    }
		pUart->IER = IER_THR_Empty | IER_Receive_Line_Status | IER_Receive_Data_Available;	// Re-enable IER_Receive_Data_Available		

		// We don't want that memory block anymore
		release_memory_block(message);
	}
}

//TODO: make into user proc
void wall_clock() {
	int doCount = 0;
	int displayClock = 0;
	int clock_time = 0;
	int sender_id = 0;
		  
	Envelope* env = (Envelope*)request_memory_block();
	uint8_t CMD_SIZE = 4;//bytes
	char* cmd = "%WS";
	char* cmd2 = "%WT";

	env->sender_pid = get_clock_pcb()->processId;
	env->receiver_pid = get_kcd_pcb()->processId;
	env->message_type = COMMAND_REGISTRATION;
	set_message_bytes(env, cmd, CMD_SIZE);
	send_message(env->receiver_pid, env);
	
	env = (Envelope*)request_memory_block();	
	env->sender_pid = get_clock_pcb()->processId;
	env->receiver_pid = get_kcd_pcb()->processId;
	env->message_type = COMMAND_REGISTRATION;
	set_message_bytes(env, cmd2, CMD_SIZE);
	send_message(env->receiver_pid, env);	 

	while(1) {
		Envelope * env = (Envelope *)receive_message(&sender_id);
		switch(env->message_type) {
			case CLOCK_TICK:
				start_tick:
				if(doCount) {
					Envelope * tickEnv = (Envelope *)request_memory_block();

					if(++clock_time >= 86400) { //tick
						clock_time = 0;
					}

					if(displayClock) {
						uint8_t TIME_LEN = 12; //bytes
						char* time_string = get_formatted_time_from_seconds(clock_time);

						Envelope* timeEnv = (Envelope *)request_memory_block();
						
					 	timeEnv->sender_pid = get_clock_pcb()->processId;
						timeEnv->receiver_pid = get_crt_pcb()->processId;
						timeEnv->message_type = OUTPUT_STRING;

						set_message_bytes(timeEnv, time_string, TIME_LEN + 1);

						send_message(timeEnv->receiver_pid, timeEnv); //to CRT for display
					}

					//Enqueue next tick
					set_sender_PID(tickEnv, get_clock_pcb()->processId);
					set_destination_PID(tickEnv, get_clock_pcb()->processId);
					set_message_type(tickEnv, CLOCK_TICK);
					delayed_send(get_clock_pcb()->processId, tickEnv, 1000);
				}
				break;
			case STOP_CLOCK:
				doCount = 0;
				break;
			case START_CLOCK:
				clock_time = (int)env->message_data;	
				doCount = displayClock = 1;

				//Start ticking
				goto start_tick; 
			case PAUSE_CLOCK:
				displayClock = 0;
				break;
			case UNPAUSE_CLOCK:
				displayClock = 1;
				break;
			case COMMAND_MATCHED: {
				char* msg = get_message_data(env);

				if(*(msg + 2) == 'S') {
					clock_time = get_seconds_from_formatted_time((msg + 3));
					if(clock_time > -1) {
						doCount = 1;
						displayClock = 1;	

						//Start ticking
						goto start_tick;
					} else {
						//Error(?)					 	
					}
				} else if(*(msg + 2) == 'T') {
					clock_time = 0;
					doCount = 0;
					displayClock = 0;		
				}

				break;
			}
			default:
				break;
		}

		release_memory_block(env);
	}
}

// --------------------------------------------------------

/*void init_sys_procs() {
	//int procIndex;
	int i;

	uint32_t* sp;
	uint32_t* stacks_start[1];

	ProcessControlBlock* sys_procs[1];	 
	uint32_t* funcPointers[] = {  
		(uint32_t*)crt_display,
		(uint32_t*)keyboard_command_decoder,
		(uint32_t*)wall_clock
	};

	//sys_procs[0] = &crt_pcb;
	//sys_procs[1] = &kcd_pcb;
	sys_procs[0] = &clock_pcb;

	//stacks_start[0] = CRT_START_STACK;
	//stacks_start[1] = KCD_START_STACK;
	stacks_start[0] = CLOCK_START_STACK;

	sys_procs[0]->processId = 0xE;
	sys_procs[0]->currentState = NEW;
	sys_procs[0]->waitingMessages.head = NULL;
	sys_procs[0]->waitingMessages.tail = NULL;
	sys_procs[0]->processPriority = 0;

	sp = stacks_start[0];

	if (!(((uint32_t)sp) & 0x04)) {
	    --sp; 
	}
													   
	*(--sp) = INITIAL_xPSR;      // user process initial xPSR  

	// Set the entry point of the process
	*(--sp) = (uint32_t) (uint32_t*)wall_clock;
	
	for (i = 0; i < 6; i++) { // R0-R3, R12 are cleared with 0
		*(--sp) = 0x0;
	}
	
	sys_procs[0]->processStackPointer = sp;

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

} */

int get_seconds_from_formatted_time(char *c){
	int h1 = c[0];	
	int h2 = c[1]; 
	int m1 = c[3]; 
	int m2 = c[4]; 
	int s1 = c[6]; 
	int s2 = c[7];

	//looking for xx:xx:xx([NULL] or RTN)
	if(c[2] != 0x3A || c[5] != 0x3A || (c[8] != 0x0D && c[8] != 0x0)) {
	 	return -1;
	}

	//check between 0-9
	if(h1 < 0x30 || h1 > 0x39
		|| h2 < 0x30 || h2 > 0x39
		|| m1 < 0x30 || m1 > 0x39
		|| m2 < 0x30 || m2 > 0x39
		|| s1 < 0x30 || s1 > 0x39
		|| s2 < 0x30 || s2 > 0x39) {

		return -1;
	 	
	}

	h1 = (h1 - 0x30) * 10 * 60 * 60;
	h2 = (h2 - 0x30) * 60 * 60;	 
	m1 = (m1 - 0x30) * 10 * 60;	
	m2 = (m2 - 0x30) * 60;
	s1 = (s1 - 0x30) * 10;	
	s2 = s2 - 0x30;

	return h1 + h2 + m1 + m2 + s1 + s2;	
}

char* get_formatted_time_from_seconds(int seconds) {
	int h, m, s;

	h = (int)(seconds / 3600);
	seconds -= (h * 3600);
	m = (int)(seconds / 60);
	seconds -= (m * 60);
	s = seconds;

	time[2] = (h / 10) + 0x30;
	time[3] = (h % 10) + 0x30;
	time[5] = (m / 10) + 0x30;
	time[6] = (m % 10) + 0x30;
	time[8] = (s / 10) + 0x30;
	time[9] = (s % 10) + 0x30;
	
	return time; 
}
