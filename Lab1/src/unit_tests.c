#include "unit_tests.h"
#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"
#include "system_proc.h"


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
		
	}

	register_command("%WS",1);
	register_command("%WT",1);

	unregister_all_commands();
	return 1;
}

int wall_clock_tests_passed(){
	assert(get_seconds_from_formatted_time("00:00:01") == 1,"Wall clock 00:00:01 broke.");
	assert(get_seconds_from_formatted_time("00:01:00") == 60,"Wall clock 00:01:00 broke.");
	assert(get_seconds_from_formatted_time("00:60:01") == 3601,"Wall clock 00:60:01 broke.");
	assert(get_seconds_from_formatted_time("23:59:59") == 86399,"Wall clock 23:59:59 broke.");
	assert(get_seconds_from_formatted_time("10:00:00") == 36000,"Wall clock 10:00:00 broke.");
	return 1;
}


int unit_tests_passed(void){
	return (
		message_api_tests_passed() &&
		//command_tests_passed() &&
		wall_clock_tests_passed()	
	);
}

