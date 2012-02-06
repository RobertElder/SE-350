#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "utils.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0


extern int get_process_priority(int);
extern int set_process_priority(int, int);

void nullProc() {
	while(1) {
		release_processor();
	}
}

void proc1(void)
{

    volatile int i =0;
	volatile int ret_val = 10;
	
    while ( 1) {
        if (i != 0 && i % 5 == 0 ) {
            ret_val = release_processor();
#ifdef DEBUG_0
		    printf("\n\rproc1: ret_val=%d. ", ret_val);
#else
		  	uart0_put_string("\n\r");
#endif // DEBUG_0
        }
        uart0_put_char('A' + i%26);
        i++;
    }

}

void proc2(void){
    volatile int i =0;		  
	volatile int ret_val = 20;
    while ( 1) {
        if (i!=0 &&i%5 == 0 ) {
            ret_val = release_processor();
#ifdef DEBUG_0
	    	printf("\n\rproc2: ret_val=%d. ", ret_val);
#else
			uart0_put_string("\n\r");
#endif // DEBUG_0
        }
        uart0_put_char('a' + i%26);
        i++;
    }
}

void run_memory_tests(void){
	while(1) {
		int i = 0;
		int testCases = 6;
		int currentTestCase = 0;
		int tmpCounter = 0;
		int testValue1 = 6;
		int testValue2 = 57;
		int * pTestPointer1 = 0;
		int * pTestPointer2 = 0;
		int testsPassed = 0;
		int testsFailed = 0;
	
		int numberOfPointersYouCanPutInOneBlockOfMemory = MEMORY_BLOCK_SIZE / sizeof(int);
	
		uart0_put_string("G015_memory_test: START\n\r");
		uart0_put_string("G015_memory_test: total 6 tests\n\r");

		//Allocate one block of memory
		pTestPointer1 = request_memory_block();
		assert((int)pTestPointer1, "Request memory returned null");
		*pTestPointer1 = testValue1;
		assert((*pTestPointer1) == testValue1, "the first memory test failed.");
	
		//Allocate a second block of memory
		pTestPointer2 = request_memory_block();
		assert((int)pTestPointer2, "Request memory returned null");
		*pTestPointer2 = testValue2;
		assert((*pTestPointer2) == testValue2, "the second memory test failed.");
	
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
			if(*pointerToCurrentMemoryBlockPointer == (int *)0){
				*pointerToCurrentMemoryBlockPointer = (int *)request_memory_block();
				//uart0_put_string("Test case ");print_unsigned_integer(currentTestCase);	uart0_put_string(" of "); print_unsigned_integer(testCases);uart0_put_string(": ");
				//uart0_put_string("Allocated memory block: ");print_unsigned_integer((int)(*pointerToCurrentMemoryBlockPointer));uart0_put_string("\n\r");
				for(tmpCounter = 0; tmpCounter < numberOfPointersYouCanPutInOneBlockOfMemory; tmpCounter++){
					// Fill this new block of memory with the ints that are the address of the block itself plus the offset(tmpCounter) of the that int 
					(*pointerToCurrentMemoryBlockPointer)[tmpCounter] = ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter;
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

		release_memory_block(pTestPointer2);
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
