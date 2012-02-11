#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "utils.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0


int num_blocks_to_request = 0;
int mem_request_attempt_made = 0;

int  * last_block_allocated;
int * pTestPointer1 = 0;


void nullProc() {
	while(1) {
		uart0_put_string("p0\n\r");
		release_processor();
	}
}

void pp1() {
  while(1) {
  		void* block;
  		uart0_put_string("pp1\n\r");
		block = request_memory_block();
		uart0_put_string("pp1_block_request_returned\n\r");
		release_processor();
		uart0_put_string("pp1_run1\n\r");
		release_memory_block(block);
		uart0_put_string("pp1_release_continue_run\n\r");
		release_processor();
	}
}

void pp2() {
  while(1) {

  		uart0_put_string("pp2\n\r");
		request_memory_block();
		uart0_put_string("pp2_block_requested\n\r");
		release_processor();
	}
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
		uart0_put_string("G099_procA: START\n\r");
		uart0_put_string("G099_procA: total 3 tests\n\r");
		uart0_put_string("G099_procA: test 1 OK\n\r");
		uart0_put_string("G099_procA: test 2 OK\n\r");
		uart0_put_string("G099_procA: test 3 OK\n\r");
		uart0_put_string("G099_procA: 3/3 tests OK\n\r");
		uart0_put_string("G099_procA: 0/3 tests FAIL\n\r");
		uart0_put_string("G099_procA: END\n\r");

		uart0_put_string("\n\r");
		release_processor();
	}
    
}

void proc2(void){

	while (1) {
		uart0_put_string("G099_AWESOME_TEST: START\n\r");
		uart0_put_string("G099_AWESOME_TEST: total 3 tests\n\r");
		uart0_put_string("G099_AWESOME_TEST: test 1 OK\n\r");
		uart0_put_string("G099_AWESOME_TEST: test 2 OK\n\r");
		uart0_put_string("G099_AWESOME_TEST: test 3 OK\n\r");
		uart0_put_string("G099_AWESOME_TEST: 3/3 tests OK\n\r");
		uart0_put_string("G099_AWESOME_TEST: 0/3 tests FAIL\n\r");
		uart0_put_string("G099_AWESOME_TEST: END\n\r");

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
