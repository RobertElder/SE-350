#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"
#include "memory.h"
#include "process.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

  // TODO is this proper?
#define MAX_ALLOWED_BLOCKS 0x1E

int num_blocks_to_request = 0;
int mem_request_attempt_made = 0;

int  * last_block_allocated;
int * pTestPointer1 = 0;

// NOTE: Keep this section up to date so we are all aware of what's is going on.
const int ORDER_LENGTH = 26;
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
///
3
};
// Further test TODOs: test if all processes are blocked, null process should run (keep as last test)
int actual_run_order[ORDER_LENGTH];
int cur_index = 0;

int order_checker(int order_length_sofar) {
	volatile int i;
	for (i = 0; i < order_length_sofar; i++) {
	 	if (actual_run_order[i] != expected_run_order[i]) {
			return 0;
		}
	}
	return 1;
}

void nullProc() {
	while(1) {
		uart0_put_string("p0\n\r");
		release_processor();
	}
}

void test_process_1() {

	int * block;


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
}

void test_process_3() {

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

	actual_run_order[cur_index] = 3;

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
	
		//  Is math broken?
		if (!(result == 0x1B79 && order_checker(cur_index))) {
			break;
		}
	}

	if(i == testCases ){
		uart0_put_string("G015_test: test 13 OK\n\r");
		uart0_put_string("G015_test: 13/13 tests OK\n\r");
		uart0_put_string("G015_test: END\n\r");

	} else {
		uart0_put_string("G015_test: test 13 FAIL\n\r");
	}



	while (1) {
		uart0_put_string("G015_test: END\n\r");
	 	release_processor();
	}

}

void test_process_4() {

}



void test_process_5() {

	int * block;
	int * block2;

	actual_run_order[cur_index] = 5;
	cur_index++;
	release_processor();

	actual_run_order[cur_index] = 5;
	cur_index++;
	release_processor();

	actual_run_order[cur_index] = 5;
	cur_index++;
	release_processor();

	actual_run_order[cur_index] = 5;
	cur_index++;

	//test the priority of test_proc_1
	if (get_process_priority(1) == 1 && order_checker(cur_index)) {
		uart0_put_string("G015_test: test 2 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 2 FAIL\n\r");
	}
	
	//should not preempt
	release_processor();

	actual_run_order[cur_index] = 5;
	cur_index++;

	if (order_checker(cur_index)) {
		uart0_put_string("G015_test: test 3 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 3 FAIL\n\r");
	}

	set_process_priority(5, 1);
	// shouldn't get preempted by test_proc_2
	actual_run_order[cur_index] = 5;
	cur_index++;
	
	if (get_process_priority(5) == 1 && order_checker(cur_index)) {
		uart0_put_string("G015_test: test 4 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 4 FAIL\n\r");
	}
	release_processor();

	//should come here after test_proc_1 is blocked on memory
	actual_run_order[cur_index] = 5;
	cur_index++;

	//requested block number 32 - will get blocked
	//proc 2 will run next
	block = request_memory_block();
	*block = 0;

	//test_proc_6 was preempted after releasing a block - test_proc_5 now got the block it requested
	actual_run_order[cur_index] = 5;
	cur_index++;

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
	actual_run_order[cur_index] = 5;
	cur_index++;

	if (order_checker(cur_index)) {
		uart0_put_string("G015_test: test 12 OK\n\r");
	} else {
		uart0_put_string("G015_test: test 12 FAIL\n\r");
	}

	set_process_priority(5, 1);
	actual_run_order[cur_index] = 5;
	set_process_priority(3, 0);



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

}

