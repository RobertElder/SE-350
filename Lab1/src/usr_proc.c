#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"
#include "ipc.h"
#include "iprocess.h"
#include "unit_tests.h"
#include "timer.h"


#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

#define MAX_ALLOWED_BLOCKS MAX_ALLOWED_MEMORY_BLOCKS

int num_blocks_to_request = 0;
int mem_request_attempt_made = 0;

int  * last_block_allocated;
int * pTestPointer1 = 0;

// NOTE: Keep this section up to date so we are all aware of what's is going on.
const int ORDER_LENGTH = 33;
int expected_run_order[] = 
// Test case 1: Basic context switching between 2 processes
{1, 5, 1, 5, 1, 5,
// Test case 2:	Lowering your own priority and getting pre-empted because of it
1,5,
// Test case 3:	When there's 1 high priority process, it should not get pre-empted when releasing processor
5,
// Test case 4: High priority process lowers his priority, but should not get pre-empted because there's no higher
// priority processes
5,
// Test case 5:	Now that there are processes with priorities equal to the running process,
// release_processor should cause pre-emption of the running process
2,
// Test case 6: A process can request a memory block (without blocking the process)
2,
// Test case 7: Data can be written and read to and from a requested memory block
2,
// Test case 8: A process can release a memory block (no pre-emption)
2,
// Test case 9:	A process can request 30 blocks without blocking. Any other memory requests cause the
// requesting process to be blocked. If a process releases some of his blocks, the blocked processes
// receive the memory block but do not preempt unless their priorities are higher.
6, 1, 5, 2, 6, 6,
// Test case 10: Give highest priority to a blocked process. There should be no preemption because it is
// blocked.
6,
// Test case 11: Releasing a memory block causes a higher priority blocked process to not only receive
// the memory, but preempt the currently executing process.
5,
// Test case 12: Additional blocking/premption/priority testing
2, 1, 6, 5,
///	TODO: document this test case
3,
// Test case 14: Proc4 blocks on receive_msg, proc3 executes and sends a message with a delay of 0
// This means proc4 should be unblocked right away and continue executing (Basic delayed_send preemption test)
// proc4 sends itself delayed messages with delays 0, 10, 50
4, 3, 4, 4, 4, 4
};
// Further test TODOs: test if all processes are blocked, null process should run (keep as last test)
int actual_run_order[ORDER_LENGTH];
int cur_index = 0;


int memory_tests_passed(void){
	int i = 0;
	int testCases = 500;
	int currentTestCase = 0;
	int tmpCounter = 0;
	int testValue1 = 6;
	int * pTestPointer1 = 0;
	int testsPassed = 0;
	int testsFailed = 0;

	int numberOfPointersYouCanPutInOneBlockOfMemory = MEMORY_BLOCK_SIZE / sizeof(int);
	if(numberOfMemoryBlocksCurrentlyAllocated < MAX_ALLOWED_MEMORY_BLOCKS) {

		//Allocate one block of memory
		pTestPointer1 = request_memory_block();
		assert((int)pTestPointer1, "Request memory returned null");
		*pTestPointer1 = testValue1;
		assert((*pTestPointer1) == testValue1, "the first memory test failed.");

		// Make sure this memory block is clean, even thought I think it already is
		for(tmpCounter = 0; tmpCounter < numberOfPointersYouCanPutInOneBlockOfMemory; tmpCounter++){
			pTestPointer1[tmpCounter] = 0;
		}

		for(currentTestCase = 1; currentTestCase <= testCases; currentTestCase++){
			// Randomly pick an int that we will consider to be a pointer to another memory block
			int currentMemoryBlockIndex = get_random() % numberOfPointersYouCanPutInOneBlockOfMemory;
			//  Treat the int at that location as a pointer to memory somewhere
			int ** pointerToCurrentMemoryBlockPointer = (int**)&(pTestPointer1[currentMemoryBlockIndex]);
			//  If that address points to nothing, allocate new memory and put the pointer there
			if(*pointerToCurrentMemoryBlockPointer == (int *)0) {
				if(numberOfMemoryBlocksCurrentlyAllocated < MAX_ALLOWED_MEMORY_BLOCKS) {
					*pointerToCurrentMemoryBlockPointer = (int *)request_memory_block();

					for(tmpCounter = 0; tmpCounter < numberOfPointersYouCanPutInOneBlockOfMemory; tmpCounter++){
						// Fill this new block of memory with the ints that are the address of the block itself plus the offset(tmpCounter) of the that int 
						(*pointerToCurrentMemoryBlockPointer)[tmpCounter] = ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter;
					}		
				}
			}else{
				//  This block does point to something, verify that the data there is in the expected format, then delete it
				for(tmpCounter = 0; tmpCounter < numberOfPointersYouCanPutInOneBlockOfMemory; tmpCounter++){
					// Is what we put there still the same?
					if(!((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter)) {
						testsFailed++;
					}
					assert((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter,"Memory test failure: block failed sanity check.");
				}
				//  Delete this block
				release_memory_block(*pointerToCurrentMemoryBlockPointer);
				*pointerToCurrentMemoryBlockPointer = 0;
			}
			testsPassed++;
		}

		//  At this point there are possibly still pointers in the block that have memory allocated
		for(i = 0; i < numberOfPointersYouCanPutInOneBlockOfMemory; i++){
			// Look at every pointer in the block
			int currentMemoryBlockIndex = i;
			//  Treat the int at that location as a pointer to memory somewhere
			int ** pointerToCurrentMemoryBlockPointer = (int**)&(pTestPointer1[currentMemoryBlockIndex]);
			//  If that address points to something delete it
			if(!(*pointerToCurrentMemoryBlockPointer == (int *)0)){		
				//  This block does point to something, verify that the data there is in the expected format, then delete it
				for(tmpCounter = 0; tmpCounter < numberOfPointersYouCanPutInOneBlockOfMemory; tmpCounter++){
					// Is what we put there still the same?
					assert((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter,"Memory test failure: block failed sanity check.");
				}
				//  Delete this block
				release_memory_block(*pointerToCurrentMemoryBlockPointer);
				*pointerToCurrentMemoryBlockPointer = 0;
			}
		}

		release_memory_block(pTestPointer1);
		// Everything should be in the same state as when we entered this function
	}
	return 1;
}

int order_checker(int order_length_sofar) {
	volatile int i;
	for (i = 0; i < order_length_sofar; i++) {
	 	if (actual_run_order[i] != expected_run_order[i]) {
			return 0;
		}
	}
	return 1;
}

int message_checker(Envelope * env, int sender, int receiver, int messageType, char msg) {
	return ((char)env->message_data == msg 
		&& (env->sender_pid == sender)
		&& (env->receiver_pid == receiver)
		&& (env->message_type == messageType));
}

void nullProc() {
	while(1) {
		uart0_put_string("p0\n\r");
		release_processor();
	}
}

void test_process_1() {

	int * block;

	assert(unit_tests_passed() && memory_tests_passed(), "Unit tests failed.");

	uart0_put_string("G015_test: START\n\r");
	actual_run_order[cur_index] = 1;
	cur_index++;
	release_processor();

	actual_run_order[cur_index] = 1;
	cur_index++;
	release_processor();

	actual_run_order[cur_index] = 1;
	cur_index++;
	release_processor();

	//Compare our actual running sequence to expected running sequence
	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 1 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 1 FAIL\n\r");
	}
	actual_run_order[cur_index] = 1;
	cur_index++;
	set_process_priority(1, 1);
	// should get preempted by proc 5

	actual_run_order[cur_index] = 1;
	cur_index++;

	//test_process_1 should get blocked - it requested block number 31 (we have max 30 blocks)
	//test_process_5 should run next
	block = request_memory_block();
	*block = 0;

	//test_proc_1 got its mem block - but test_proc_6 had same priority and did not get preempted
	//came here after test_proc_5 (higher priority) got blocked and test_proc_2 released processor
	actual_run_order[cur_index] = 1;
	cur_index++;

	//request mem block number 32 - will get blocked
	block = request_memory_block();
	*block = 0;
	assert(0,"Should not reach this point. End of proc 1");
}

void test_process_2() {

	int* block;
	int i = 0;
	int alloc_bad = 0;

	actual_run_order[cur_index] = 2;
	cur_index++;

	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 5 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 5 FAIL\n\r");
	}
	
	block = request_memory_block();

	//should not be blocked - first to request the block
	actual_run_order[cur_index] = 2;
	cur_index++;

	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 6 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 6 FAIL\n\r");
	}

	for (i = 0; i < 16; i++) {
		*block = i;
		block++;
	}
	
	block -= 16;
	for (i = 0; i < 16; i++) {
	 	if ( *block != i) {
			alloc_bad += 1;
		}
		block++;	
	}
	
	actual_run_order[cur_index] = 2;
	cur_index++;

	if (alloc_bad > 0) {
		uart0_put_string("G015_test: test 7 FAIL\n\r");
	} else {
	 	uart0_put_string("G015_test: test 7 OK\n\r");
	}
	
	block -= 16;
	release_memory_block(block);  
		
	// no test_processes were blocked on memory, should not preempt
	actual_run_order[cur_index] = 2;
	cur_index++;

	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 8 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 8 FAIL\n\r");
	}

	release_processor();

	//comes here after test_proc_5 is blocked on memory
	actual_run_order[cur_index] = 2;
	cur_index++;

	//should switch to test_proc_1
	release_processor();

	//comes here after test_proc_1 is blocked on memory
	actual_run_order[cur_index] = 2;
	cur_index++;

	//should switch to test_proc_6
	release_processor();
	assert(0,"Should not reach this point. End of proc 2");
}

void test_process_3() {
	Envelope * env_test15;
	char message_test15;

	int a = 0;
	int b = 1;
	int c = 2;
	int d = 3;
	int e = 4;
	int f = 5;
	int g = 6;
	int h = 7;
	int ii = 8;
	int j = 9;
	int k = 10;

	int randomWait = 0;
	int i;
	int result;
	int testCases = 100;

	actual_run_order[cur_index++] = 3;

	for(i = 0; i < testCases; i++){
		randomWait = get_random() % 1000;
		//  Introduce randomness into the timing
		while(randomWait) {randomWait--;}

		result = 1
		*1+2-2+7%2-3*1%6+8/c-5/b*6+5+8/2*7+8-9-1%1+5%2%5%7%8%5%5-3+4
		+2/7-7-7-6+4*8+9-5%5%5*1%8*4%3-1/7-3+3/6*3-2/9%4-8-2*9%2+9%8
		*1/5-6-9-9*8/7*1+5+4-5/d*5*1%4-4%7+1*1%1/6%7-6-5/4+5*5-6+7*4
		*4+8/6*2-1+4/6-2-6-3-6*0*0*9+2*6*8-7-4/1%5/6%9/5*4*9-2+8+7%5
		%8%2+1-0%3/7%3-9/1/3/7/3+4*1*6+2/2-4+8+7/5/6%8*2*4%4/9/5%7%2
		-7+5+9+4*6/9*8%6%1/8-7+8/4%9%9*2*4/6*9+3%2*3/3%1/6/6-3%8*7-4
		-9-7/5-2-9%6+9-4/6+3+2%5%4/7%9%9*9%4/4%5-4/8%5-3%3*7-2*6/6+2
		*0+7%6%9+6%8/6%e%5-f-6%a*7%9+6%g/7%8%6-0/8-4+9*8*7*5*2+8-7+3
		+7%6%5%3%5*3-3/6+3*3*3*3/8-3-7-6/6+6-8%7%1+2+6/6/7+6/6*8+4%6
		%4-8-6+7*3/9-3/2-5/1/7+7*0/4-9/h/4-3*7+7%9-1+5*9/3+0+4+0-7/2
		+7+5-1%2+2+9*2*4+1+0*7%4/7%5+8/ii*8+8+8+1+3*5*3*1%8-6-0-8-0/6
		%2%3%5*4/6*7+5*6*5*6*2+9+9%1+9*1/3*6/3/9+3-8*0/9%5+4/6%2%3%4
		-2-1/8-2-4-6%1-8%1+3%2/2-5+4+4+7/3-5*2+2/1*5*1+7/5-7*4%6%7-9
		/1+9%1/6*4%3/2*7/6+0+4%7-5/4%6%5*5+6*5/4*6-2%6+0%9%6/5-3%5%8
		%4%9+5*0/3/2*9%1/4/9/2%5/9/5*4+8/8/1-3-9*5+3%5*4+4%4%6%9-7%7
		/9-8*0-8-0+7+1+7%5+5*4*7+8-6/7*8%9+4*4-0/4/2-2%2*8%4*7/3/6+1
		/2+3+3+3/5+1-0/3/1/5-6-0*3%6-4%5*2/8*6+6%8-4+1+6*7+2+4%8*2+6
		%6-1+6-8+7*3*7+2-0/k-2+8-3+4-6%2%2*4-0%9/7-1*6*9%2*7+0/2+8-6
		*5/6+0*4/1%4*5*7*0*0/6%9-5/5-5%4/1*1%8/2%7/5-4+8%7*5+9+3%9-5
		/9*6-3/8*1+6%4*6+8-2/7+9%6/1*7-8*1%6*4-3+0%9%1/8%4-9*7-3-8-7
		*7*7*6/9-1-7-1*1%4*6+2/3*j+2/7-1*0%5-3*7-5/8%8+8+2%2*2-8+9%8
		%2*7-7*4+0-2%2%7*0+0/2*4-8%5+0-3%6+6-0-5%3*0*7*2%2-2*6%2/1/4
		+9+1+0/6+1/2%1+9%9%3+4%8/6-8-9*7*5%6-4-2*3+3/9%1%5%6*6+4%4*1
		/7*9*7-1+5+6/8-8*6-7-0%1%5/9-5*7*2+1-9+1-8-2-5%8+9/2%5-4+4*0
		-3+3*0*5%1/3/1%1*1%2/5-3+8/7+4+8-8-3*1*8-9-9*8-1+9-8+7-4+0-5
		/2/5+1*1/3*5/8*4%6%6%8-9%6+1*8*1%8*7+8%5*9%3%7*6-6+0%2/2-1+2
		/7+2-2%5/9*4%1*2/2%7-5/7/1/8/3*6-0-2%4%2%4+8/2-0%7-0-6+9*1/6
		*0/3%2-4/5*2-9/9*6*8+4%1/9/8+0*2%4-5*6-4/2%1%3+5%1-7+3*4+8*1
		%6%5/4/6%4%7-7*5/5/4-0%6/8%6%9-9-9%7%7+6/1%6+8*8-7*6/9%4+0-7
		-8+9%2-4/9*1%9%7%8+6+4/5-0%7%2%9%7%8/1/2-6%3*4+9%6%7*5+2+1/7
		+8%4+7%3+0+5/6*6-9-6+7+5%9+1%7*6-9*1/5+2-5-9-9-9/5*4%3/1*7+6
		-3/4+3-1+7+7*4-3*1/2%6+9%9*1-3+0%1-9*0-7%3/5+6+6-5%5/7*0-3%9
		+8-8*3/9*4%4/9-3+1+9%5/2+4%8+5%1%1+0-9%1+7%4%4*0-2+3*4*4*3/2
		/3-3+4*8-5+9+3*2/5*6;
	
		//  Did the calculation work?
		if (!(result == 0x1B79)) {
			break;
		}
	}

	if(i == testCases && order_checker(cur_index)){
		uart0_put_string("G015_test: test 13 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 13 FAIL\n\r");
	}

	//GOTO process 6
	set_process_priority(6, 0);
	release_processor();

	// comes in from proc4
	actual_run_order[cur_index++] = 3;

	//Send message to proc 4
  	message_test15 = 'a';	

	env_test15 = (Envelope *)request_memory_block();

	set_sender_PID(env_test15, 3);
	set_destination_PID(env_test15, 4);
	set_message_type(env_test15, DELAYED_SEND);
	set_message_bytes(env_test15, &message_test15, sizeof(char));

	send_message(4, env_test15);

	// Comes in here from test process 4. Will loop until proc4 gets his message. (does this twice)
	while (1) {}

}

void test_process_4() {

	int* sender_id;
	Envelope * env;
	int delay_time;
	char message_test15;
	int test_passed = 1;
	
	// STARTING TEST CASE 14 
	actual_run_order[cur_index++] = 4;

	// This blocks (process 3 takes over)
	env = (Envelope *)receive_message(sender_id);

   	// Comes in from proc3
	actual_run_order[cur_index++] = 4;

	//Set up message 1 (delay 0)
	message_test15 = 'b';	
	set_sender_PID(env, 4);
	set_destination_PID(env, 4);
	set_message_type(env, DELAYED_SEND);
	set_message_bytes(env, &message_test15, sizeof(char));

	//Send msg 1
	delay_time = get_current_time();
	delayed_send(4, env, 0);

	// This will block until the message comes. Proc 3 will run in his while loop
	env = (Envelope *)receive_message(sender_id); //Send message to itself, receive later

	//Check message contents
	//Add 1 to delay_time in order to account for context switches and stuff
	//(too late) || (too early) || (Check message contents)
	if(get_current_time() > delay_time + 1 || get_current_time() < delay_time || !message_checker(env, 4, 4, DELAYED_SEND, 'b')) {
		test_passed = 0;
	}

	actual_run_order[cur_index++] = 4;

	//Set up message 1 (delay 10)
	message_test15 = 'c';	
	set_sender_PID(env, 4);
	set_destination_PID(env, 4);
	set_message_type(env, DELAYED_SEND);
	set_message_bytes(env, &message_test15, sizeof(char));

	//Send msg 1
	delay_time = get_current_time();

	// Send message to self. Will receive later
	delayed_send(4, env, 10);

	// Blocks until message to self is received.
	env = (Envelope *)receive_message(sender_id);

	//(too late) || (too early) || (Check message contents)
	if(get_current_time() > (delay_time + 10 + 1) || get_current_time() < delay_time || !message_checker(env, 4, 4, DELAYED_SEND, 'c')) {
		test_passed = 0;
	}

	actual_run_order[cur_index++] = 4;

	//Set up message 1 (delay 50)
	message_test15 = 'd';	
	set_sender_PID(env, 4);
	set_destination_PID(env, 4);
	set_message_type(env, DELAYED_SEND);
	set_message_bytes(env, &message_test15, sizeof(char));

	//Send msg 1
	delay_time = get_current_time();
	// sends msg to self
	delayed_send(4, env, 50);
	// blocks on receive, proc3 will run in the meantime
	env = (Envelope *)receive_message(sender_id);

	//(too late) || (too early) || (Check message contents)
	if(get_current_time() > (delay_time + 50 + 1) || get_current_time() < delay_time || !message_checker(env, 4, 4, DELAYED_SEND, 'd')) {
		test_passed = 0;
	}

	actual_run_order[cur_index++] = 4;

	
	// TODO check message is good
	if (order_checker(cur_index) && test_passed) {
		uart0_put_string("G015_test: test 14 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 14 FAIL\n\r");
	}
   
	uart0_put_string("G015_test: END\n\r");

	while (1) {
		int i = 999999;
		while (i) { i--; }
		release_processor();
		
	}

}



void test_process_5() {

	int * block;
	int * block2;

	actual_run_order[cur_index++] = 5;
	release_processor();

	actual_run_order[cur_index++] = 5;
	release_processor();

	actual_run_order[cur_index++] = 5;
	release_processor();

	actual_run_order[cur_index++] = 5;

	//test the priority of test_proc_1
	if (get_process_priority(1) == 1 && order_checker(cur_index)) {
		uart0_put_string("G015_test: test 2 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 2 FAIL\n\r");
	}
	
	//should not preempt
	release_processor();

	actual_run_order[cur_index++] = 5;

	if (order_checker(cur_index)) {
		uart0_put_string("G015_test: test 3 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 3 FAIL\n\r");
	}

	set_process_priority(5, 1);
	// shouldn't get preempted by test_proc_2
	actual_run_order[cur_index++] = 5;
	
	if (get_process_priority(5) == 1 && order_checker(cur_index)) {
		uart0_put_string("G015_test: test 4 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 4 FAIL\n\r");
	}
	release_processor();

	//should come here after test_proc_1 is blocked on memory
	actual_run_order[cur_index++] = 5;

	//requested block number 32 - will get blocked
	//proc 2 will run next
	block = request_memory_block();
	*block = 0;

	//test_proc_6 was preempted after releasing a block - test_proc_5 now got the block it requested
	actual_run_order[cur_index++] = 5;

	if (order_checker(cur_index)) {
		uart0_put_string("G015_test: test 11 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 11 FAIL\n\r");
	}

	//requested block number 31 - will get blocked
	//proc 2 will run next
	block2 = request_memory_block();
	*block2 = 0;

	//test_proc_6 was preempted after releasing a block - test_proc_5 now got the block it requested
	actual_run_order[cur_index++] = 5;

	if (order_checker(cur_index)) {
		uart0_put_string("G015_test: test 12 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 12 FAIL\n\r");
	}

	set_process_priority(5, 1);
	actual_run_order[cur_index] = 5;
	set_process_priority(3, 0);
	assert(0,"Should not reach this point. End of proc 5");
}

void test_process_6() {	

	void* blocks[MAX_ALLOWED_BLOCKS];
	void* block;
	int i = 0;

	actual_run_order[cur_index] = 6;
	cur_index++;

	// want to test allocating max number of blocks, blocking a process on
	// a block request and releasing processes on block release
	for (i = 0; i < MAX_ALLOWED_BLOCKS; i++) {
	 	block = request_memory_block();
		blocks[i] = block;
	}

	//process 1 should be the next to run
	release_processor();

	//gets here after test_proc_2 releases processor
	//here test_proc_1 and test_proc_5 are blocked
	actual_run_order[cur_index] = 6;
	cur_index++;

	//should not be preempted after release - test_proc_1 has the same priority
	release_memory_block(blocks[--i]);

	actual_run_order[cur_index] = 6;
	cur_index++;

	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 9 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 9 FAIL\n\r");
	}

	// test_proc_5 is now of highest priority - but it is blocked - should not preempt here
	set_process_priority(5, 0);

	actual_run_order[cur_index] = 6;
	cur_index++;

	if(order_checker(cur_index)){
		uart0_put_string("G015_test: test 10 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 10 FAIL\n\r");
	}

	// should be preempted after release - test_proc_5 has a higher priority
	release_memory_block(blocks[--i]);

	actual_run_order[cur_index] = 6;
	cur_index++;

	//test_proc_1 and test_proc_5 are blocked on memory
	//after release, test_proc_5 should get memory block and preempt - it has higher priority
	release_memory_block(blocks[--i]);

	// Comes in from process 3

	// Release all the memory blocks
	while(i > 0) {
		release_memory_block(blocks[--i]);
	}
	
	set_process_priority(4, 0);
	set_process_priority(1, 3);
	set_process_priority(2, 3);
	set_process_priority(3, 1);
	set_process_priority(5, 3);
	set_process_priority(6, 3);
	release_processor();
	assert(0,"Should not reach this point. End of proc 6");
}


