#include "rtx.h"
#include "uart.h"
#include "system_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"



char current_command_buffer[MAX_COMMAND_LENGTH];
int current_command_length = 0;


// These are dummy variables so this code will compile.  This code is nowhere near finished
uint8_t g_UART0_TX_empty_placeholder=1;

void unregister_all_commands(){
	number_of_registered_commands = 0;
}

void register_command(char * s, int process_id){
	int i = 0;

	registered_processes[number_of_registered_commands] = process_id;
	for(i = 0; i < MAX_COMMAND_LENGTH; i++){
		assert(i < MAX_COMMAND_LENGTH,"Invalid attempt to register a command that is too long.");

		registered_commands[number_of_registered_commands][i]  = s[i];
		//  We want to copy everything up to and including the null
		if(s[i] == 0)
			break;

	}
	assert(number_of_registered_commands < MAX_NUMBER_OF_REGISTERABLE_COMMANDS, "Too many commands registered");
	number_of_registered_commands++;

}

int get_index_of_matching_command(){
	/*  Returns the first index of a complete command that
	 exists as a prefix of the current command buffer
	 */
	int i = 0;
	int j = 0;
 	for(i = 0; i < number_of_registered_commands; i++){
		for(j = 0; j < MAX_NUMBER_OF_REGISTERABLE_COMMANDS; j++){
			if(registered_commands[i][j] == 0){
				//  We are at the end of the command so it must have matched so far.
				assert(j > 0,"We just matched an empty command.  This is probably not right.");
				return i;
			}else if(j >= current_command_length){
				//  We reached the end of the command buffer, there is nothing to check
				break;
			}else if(!(registered_commands[i][j] == current_command_buffer[i])){
				// This command did not match
				break;
			}	
		}
	}
	//  No matching command found
	return -1;
} 

void keyboard_command_decoder(void * message){

	int aaa = 0;
	int sender_id = -1;

	// Get our new message
	//void * message = k_receive_message(&sender_id);
	//  Point to the data
	char * pChar = get_message_data(message);
	aaa = sender_id;
	// If the buffer is full, they are not part of any valid command so we don't care
	if(current_command_length <= MAX_COMMAND_LENGTH){
		//  Put the character we received into the buffer
		current_command_buffer[current_command_length] = *pChar;
		current_command_length++;
	}

	// Did they type a newline?
	if(*pChar == 10){
		int matched_command = get_index_of_matching_command();

		//  Does the thing in the buffer match a command that was registered? 
		if(matched_command > -1){
		 	  //  The user type a command that we recognized
			  aaa++;
		}

		// Reset the buffer for new commands
		current_command_length = 0;
	}

	release_memory_block(message);	
}

void crt_display(void){
 	while(1){
		int sender_id = -1;
		void * message = k_receive_message(&sender_id);

		if (message != NULL) {
			LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
			// Our message data should be a null terminated string
			uint8_t * current_character = get_message_data(message);
	
		    while ( *current_character != 0 ) {
			    // THRE status, contain valid data  
			    while ( !(g_UART0_TX_empty_placeholder & 0x01) );
				//  Setting this value makes the interrupt fire at a later time	
			    pUart->THR = *current_character;
				g_UART0_TX_empty_placeholder = 0;  // not empty in the THR until it shifts out
			    current_character++;
		    }
			
			// We don't want that memory block anymore
			k_release_memory_block(message);
		}		
	}
}
