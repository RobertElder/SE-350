#include "system_proc.h"
#include "rtx.h"
#include "uart.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "iprocess.h"
#include "ipc.h"
#include "hot_keys.h"


typedef struct cmd {
	uint32_t registered_pid;
	char command_string[MAX_COMMAND_LENGTH] ;
	uint32_t message_type;
} RegisteredCommand;

int number_of_registered_commands = 0;
RegisteredCommand registered_commands[MAX_NUMBER_OF_REGISTERABLE_COMMANDS];

char current_command_buffer[MAX_COMMAND_LENGTH];
int current_command_length = 0;

char time[] = "00:00:00\r\n";

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
		int enter_pressed = 0;
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

				//do_hot_key(*pChar);
				// Did they type a carriage return?
				if(*pChar == 0xD){
					int indexOfMatchedCommand = get_index_of_matching_command();
					enter_pressed = 1;

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

					//Append a newline
					current_command_buffer[current_command_length] = '\r';
					current_command_length++;
					current_command_buffer[current_command_length] = '\n';
					current_command_length++;

					set_message_type(clockMsg, UNPAUSE_CLOCK);
				} else {
					set_message_type(clockMsg, PAUSE_CLOCK);
				}

				//Pause or resume clock
				send_message(get_clock_pcb()->processId, clockMsg);

			   	//Null terminate
				current_command_buffer[current_command_length] = 0;
	
				if(enter_pressed) {
				 	// Reset the buffer for new commands
					current_command_length = 0;
					enter_pressed = 0;
				}

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

#define CLOCK_WIDTH 40
#define CLOCK_HEIGHT 10
char clock_ui[CLOCK_HEIGHT][CLOCK_WIDTH + 3];


void draw_hand(double angle, double padding_ratio, char c){
	double rise = sine(angle);
	double run = cosine(angle);
	double current_x = CLOCK_WIDTH / 2;
	double current_y = CLOCK_HEIGHT / 2;

	while(
		current_x > (padding_ratio * (double)CLOCK_WIDTH)  && current_x < CLOCK_WIDTH - (padding_ratio * (double)CLOCK_WIDTH) &&
		current_y > (padding_ratio * (double)CLOCK_HEIGHT) && current_y < CLOCK_HEIGHT - (padding_ratio * (double)CLOCK_HEIGHT)
	){
		clock_ui[(int)current_y][(int)current_x] = c;
		current_x += run;
		current_y -= rise;	
	}
}


double get_angle(double n, double d){
	return -(
		((double) 2 * PI_CONSTANT)*
		((double)n/(double)d)
	)
	+ (PI_CONSTANT / (double)2);
}

void print_the_time(int clock_time){
	/* USE THIS CODE IF THE CLOCK BREAKS
	char* time_string = get_formatted_time_from_seconds(clock_time);
	uart0_put_string((char *)get_erase_display_sequence());
	uart0_put_string(time_string);
	*/
	int seconds = clock_time % 60;
	int minutes = clock_time / 60;
	int hours = clock_time / 3600;
	double second_angle = get_angle(seconds,60);
	double minute_angle = get_angle(minutes,60);
	double hour_angle = get_angle(hours,12);

	int i = 0;
	int j = 0;
	char* time_string = get_formatted_time_from_seconds(clock_time);

	for(i = 0; i < CLOCK_HEIGHT; i++){
		clock_ui[i][CLOCK_WIDTH] = 10;
		clock_ui[i][CLOCK_WIDTH + 1] = 13;
		// Null terminate the strings
		clock_ui[i][CLOCK_WIDTH + 2] = 0;	
	}

	for(i = 0; i < CLOCK_WIDTH; i++){
		for(j = 0;j < CLOCK_HEIGHT; j++){
			//space
			clock_ui[j][i] = 46;
		}
	}

	draw_hand(second_angle,0.1,115);
	draw_hand(minute_angle,0.2,109);
	draw_hand(hour_angle,0.3,104);

	clock_ui[CLOCK_HEIGHT/2][CLOCK_WIDTH/2] = 35;

	uart0_put_string((char *)get_erase_display_sequence());
	for(i = 0; i < CLOCK_HEIGHT; i++){
		uart0_put_string(&(clock_ui[i][0]));
	}


	uart0_put_string(time_string);
}

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
						print_the_time(clock_time);
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
						char* errorMsg = "\r\nInvalid time input\r\n";
						send_error_message(get_clock_pcb()->processId, errorMsg);				 	
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

void priority_process() {
	Envelope* env = (Envelope*)request_memory_block();
	uint8_t CMD_SIZE = 4;//bytes
	char* cmd = "%C";
	int sender_id = 0;

	env->sender_pid = get_priority_process_pcb()->processId;
	env->receiver_pid = get_kcd_pcb()->processId;
	env->message_type = COMMAND_REGISTRATION;
	set_message_bytes(env, cmd, CMD_SIZE);
	send_message(env->receiver_pid, env);

	while(1) {
		Envelope * env = (Envelope *)receive_message(&sender_id);
		if(env->message_type == COMMAND_MATCHED) {

				char* msg = get_message_data(env);
				int pid = get_int_from_string(msg + 3 * sizeof(char));
				int priority;				

				if(!is_pid_valid(pid)) {
					char * errorMsg = "\r\nInvalid process ID\r\n";
					send_error_message(get_priority_process_pcb()->processId, errorMsg);
				} else {
				 	if(pid < 10) {
						priority = get_int_from_string(msg + 5 * sizeof(char)); 	
					} else {
						priority = get_int_from_string(msg + 6 * sizeof(char));	
					}

					if(!is_valid_priority(priority)) {
					 	char * errorMsg = "\r\nInvalid priority\r\n";
						send_error_message(get_priority_process_pcb()->processId, errorMsg);
					} else {
						set_process_priority(pid, priority);
					}
				}
		}

		release_memory_block(env);
	}
}

void send_error_message(int sender, char* errorMsg) {
 	Envelope* errorEnv = (Envelope *)request_memory_block();

	errorEnv->sender_pid = sender;
	errorEnv->receiver_pid = get_crt_pcb()->processId;
	errorEnv->message_type = OUTPUT_STRING;
	set_message_bytes(errorEnv, errorMsg, sizeof(char) * string_len(errorMsg));
	
	send_message(errorEnv->receiver_pid, errorEnv);
}

// --------------------------------------------------------

int get_int_from_string(char *str) {
	int i = 0;
	int x = 0;
	int temp = str[0];

	//Check for end of string (NUL, CR, or SPACE)
	if(temp == 0 || temp == 0xD || temp == 0x20) {
	 	return -1;
	}

	while(!(temp == 0 || temp == 0xD || temp == 0x20)) {	
		temp = str[i] - 0x30;

		if(temp > 9 || temp < 0) {
		 	return -1;
		}

		x += temp;
		temp = str[++i];
	}

	return x;
}

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
	if(h1 < 0x30 || h1 > 0x32
		|| h2 < 0x30 || h2 > 0x39
		|| m1 < 0x30 || m1 > 0x36
		|| m2 < 0x30 || m2 > 0x39
		|| s1 < 0x30 || s1 > 0x36
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

	time[0] = (h / 10) + 0x30;
	time[1] = (h % 10) + 0x30;
	time[3] = (m / 10) + 0x30;
	time[4] = (m % 10) + 0x30;
	time[6] = (s / 10) + 0x30;
	time[7] = (s % 10) + 0x30;
	
	return time; 
}
