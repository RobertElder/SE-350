#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"

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
const int ORDER_LENGTH = 25;
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
2, 1, 6, 5};
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

	uart0_put_string("G015_test: 12/12 tests OK\n\r");
	uart0_put_string("G015_test: END\n\r");

	while (1) {
		uart0_put_string("G015_test: END\n\r");
	 	release_processor();
	}

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

void pp1() {
}

void pp2() {
}


void p1() {
  	while(1) {
		uart0_put_string("p1\n\r");
		release_processor();
		uart0_put_string("p1_return_from_release\n\r");
		set_process_priority(1, 1);
		uart0_put_string("p1_lower_priority\n\r");
	}
}

void p2() {
   while(1) {
   		uart0_put_string("p2\n\r");
		release_processor();
	}
}

void p3() {
  while(1) {
  		uart0_put_string("p3\n\r");
		release_processor();
	}
}

void p4() {
   while(1) {
   		uart0_put_string("p4\n\r");
		release_processor();
	}
}

void proc1(void)
{

	while (1) {
		uart0_put_string("G015_procA: START\n\r");
		uart0_put_string("G015_procA: total 3 tests\n\r");
		uart0_put_string("G015_procA: test 1 OK\n\r");
		uart0_put_string("G015_procA: test 2 OK\n\r");
		uart0_put_string("G015_procA: test 3 OK\n\r");
		uart0_put_string("G015_procA: 3/3 tests OK\n\r");
		uart0_put_string("G015_procA: 0/3 tests FAIL\n\r");
		uart0_put_string("G015_procA: END\n\r");

		uart0_put_string("\n\r");
		release_processor();
	}
    
}

void proc2(void){

	while (1) {
		uart0_put_string("G015_AWESOME_TEST: START\n\r");
		uart0_put_string("G015_AWESOME_TEST: total 3 tests\n\r");
		uart0_put_string("G015_AWESOME_TEST: test 1 OK\n\r");
		uart0_put_string("G015_AWESOME_TEST: test 2 OK\n\r");
		uart0_put_string("G015_AWESOME_TEST: test 3 OK\n\r");
		uart0_put_string("G015_AWESOME_TEST: 3/3 tests OK\n\r");
		uart0_put_string("G015_AWESOME_TEST: 0/3 tests FAIL\n\r");
		uart0_put_string("G015_AWESOME_TEST: END\n\r");

		uart0_put_string("\n\r");
		release_processor();
	}
    
}

void run_memory_tests(void){

	while(1) {
		int i = 0;
		int testCases = 6;
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
		
			uart0_put_string("G015_memory_test: START\n\r");
			uart0_put_string("G015_memory_test: total 6 tests\n\r");
	
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
						//assert((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter,"Memory test failure: block failed sanity check.");
						if(!((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter)) {
							uart0_put_string("G015_memory_test: test ");
							print_unsigned_integer(currentTestCase);
							uart0_put_string(" FAIL\n\r");
							testsFailed++;
						}
					}
					//  Delete this block
					//uart0_put_string("Test case ");print_unsigned_integer(currentTestCase);	uart0_put_string(" of "); print_unsigned_integer(testCases);uart0_put_string(": ");
					//uart0_put_string("Memory block ");print_unsigned_integer((int)(*pointerToCurrentMemoryBlockPointer));uart0_put_string(" looks OK, releasing.\n\r");
					release_memory_block(*pointerToCurrentMemoryBlockPointer);
					*pointerToCurrentMemoryBlockPointer = 0;
				}
				uart0_put_string("G015_memory_test: test ");
				print_unsigned_integer(currentTestCase);
				uart0_put_string(" OK\n\r");
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
					//uart0_put_string("Test case ");print_unsigned_integer(currentTestCase);	uart0_put_string(" of "); print_unsigned_integer(testCases);uart0_put_string(": ");
					//uart0_put_string("Memory block ");print_unsigned_integer((int)(*pointerToCurrentMemoryBlockPointer));uart0_put_string(" looks OK, releasing.\n\r");
					release_memory_block(*pointerToCurrentMemoryBlockPointer);
					*pointerToCurrentMemoryBlockPointer = 0;
				}
			}
	
			release_memory_block(pTestPointer1);
			//  Everything should be in the same state as when we entered this function
			
			uart0_put_string("G015_memory_test: ");
			print_unsigned_integer(testsPassed);
			uart0_put_string("/");
			print_unsigned_integer(testCases);
			uart0_put_string(" OK\n\r");
	
			uart0_put_string("G015_memory_test: ");
			print_unsigned_integer(testsFailed);
			uart0_put_string("/");
			print_unsigned_integer(testCases);
			uart0_put_string(" FAIL\n\r");
	
			uart0_put_string("G015_memory_test: END\n\r");
		}

		release_processor();
	}
}

void run_priority_tests(void) {

	while(1) {
		int procIndex;
		int testsPassed = 0;
		int testsFailed = 0;
		int priority;
		ProcessControlBlock * process;

		uart0_put_string("G015_priority_test: START\n\r");
		uart0_put_string("G015_priority_test: total 2 tests\n\r");
	
		for(procIndex = 0; procIndex < NUM_PROCESSES; ++procIndex) {
			process = get_process_pointer_from_id(procIndex);
			priority = get_process_priority(procIndex);

			if((procIndex == 0 && priority != 4) || (procIndex != 0 && priority != process->processPriority)) {
			 	testsPassed--;
				testsFailed++;

				uart0_put_string("G015_priority_test: test ");
				print_unsigned_integer(1);
				uart0_put_string(" FAIL\n\r");

				break;
			}
		}

		if(testsPassed == 0) {
			testsPassed++;
		 	uart0_put_string("G015_priority_test: test ");
			print_unsigned_integer(1);
			uart0_put_string(" OK\n\r");
		}
	
		for(procIndex = 1; procIndex < NUM_PROCESSES - 1; ++procIndex) {
			priority = get_process_priority(procIndex);

			if((set_process_priority(procIndex, 3) != 0) || (get_process_priority(procIndex) != 3)) {
			 	testsPassed--;
				testsFailed++;

				uart0_put_string("G015_priority_test: test ");
				print_unsigned_integer(2);
				uart0_put_string(" FAIL\n\r");

				break;	
			}

			//Reset priority
		   	set_process_priority(procIndex, priority);
		}

		if(testsPassed == 1) {
			testsPassed++;
		 	uart0_put_string("G015_priority_test: test ");
			print_unsigned_integer(2);
			uart0_put_string(" OK\n\r");
		}


		uart0_put_string("G015_priority_test: ");
		print_unsigned_integer(testsPassed);
		uart0_put_string("/");
		print_unsigned_integer(2);
		uart0_put_string(" OK\n\r");

		uart0_put_string("G015_priority_test: ");
		print_unsigned_integer(testsFailed);
		uart0_put_string("/");
		print_unsigned_integer(2);
		uart0_put_string(" FAIL\n\r");

		uart0_put_string("G015_priority_test: END\n\r");
	
		release_processor();
	}	   
}

void run_block_memory_test() {
	while(1) {
		int * block;
		ProcessControlBlock * mem_request_proc;

		num_blocks_to_request = MAX_ALLOWED_MEMORY_BLOCKS - numberOfMemoryBlocksCurrentlyAllocated;

		block = (int *)request_memory_block();		

		mem_request_attempt_made = 0;

		while(!mem_request_attempt_made) {
		 	 release_processor();
		}

		mem_request_proc = get_process_pointer_from_id(4);

		if(mem_request_proc->currentState != BLOCKED_ON_MEMORY) {
			uart0_put_string("Blocking did not occur. Test failed.\n\r");
		} else {
			uart0_put_string("Blocking occurred. Test passed.\n\r");
		}

		release_memory_block(block);

		release_processor();
	}	
}

void memory_request_process() {
 	while(1) {
		int i;
		int * p;
		
		if(num_blocks_to_request > 0) {
		 	int * blocks[MAX_ALLOWED_MEMORY_BLOCKS];

			mem_request_attempt_made = 1;

			for(i = 0; i < num_blocks_to_request; ++i) {
				p = (int *)request_memory_block();
				blocks[i] = p; 	
			}

			for(i = 0; i < num_blocks_to_request; ++i) {
				release_memory_block(blocks[i]); 	
			}

			num_blocks_to_request = 0;
			
		}		
		
		release_processor();
	}
}
