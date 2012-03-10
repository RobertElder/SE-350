#include "unit_tests.h"
#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"





int message_api_tests_passed(){

	void * p = request_memory_block();
	int i = 0;
	int testCases = 100;

	for(i = 0; i < testCases; i++){
		set_sender_PID(p, i);
		assert(get_sender_PID(p) == i,"message api function is broken");

		set_destination_PID(p, i);
		assert(get_destination_PID(p) == i,"message api function is broken");

		set_message_type(p, i);
		assert(get_message_type(p) == i,"message api function is broken");

		assert(get_message_data(p) != 0,"message api function is broken");

		assert(get_message_data(p) != 0,"message api function is broken");

		//set_message_data(p,p, 10);
	}
   
	release_memory_block(p);
	return 1;
}

int command_tests_passed(){
	int i = 0;
	void * p;
	char cmd[] = "%WS12:45:00\r";

   	for(i = 0; i < 100; i++){
		register_command("afdsasd",1);
		register_command("%mkdir",1);
		register_command("asfdsafsdd",1);
		register_command("afdsasd",1);
		register_command("afdsafsdsd",1);
		register_command("afdsafsdsd",1);
		register_command("afsdafsdasd",1);
		register_command("asfdsafsdd",1);
		register_command("fWS",1);
		register_command("afdsafdsfsdfsd",1);
		unregister_all_commands();
	}

	register_command("%WS",1);

	for(i = 0; i < 12; i++){
		p = k_request_memory_block();
		set_message_bytes(p , &(cmd[i]) , 1);
		keyboard_command_decoder(p);
	}
	return 1;
}


int unit_tests_passed(void){
	return (
		message_api_tests_passed() &&
		command_tests_passed()	
	);
}

