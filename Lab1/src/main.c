#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "utils.h"
#include "rtx.h"





void run_memory_tests(){

	int testCases = 100;
	int currentTestCase = 0;
	int tmpCounter = 0;
	int testValue1 = 6;
	int testValue2 = 57;
	int * pTestPointer1 = 0;
	int * pTestPointer2 = 0;

	int numberOfPointersYouCanPutInOneBlockOfMemory = MEMORY_BLOCK_SIZE / sizeof(int);

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

	for(currentTestCase = 0; currentTestCase < testCases; currentTestCase++){
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
				assert((*pointerToCurrentMemoryBlockPointer)[tmpCounter] == ((int)(*pointerToCurrentMemoryBlockPointer)) + tmpCounter,"Memory test failure: block failed sanity check.");
			}
			//  Delete this block
			//uart0_put_string("Test case ");print_unsigned_integer(currentTestCase);	uart0_put_string(" of "); print_unsigned_integer(testCases);uart0_put_string(": ");
			//uart0_put_string("Memory block ");print_unsigned_integer((int)(*pointerToCurrentMemoryBlockPointer));uart0_put_string(" looks OK, releasing.\n\r");
			release_memory_block(*pointerToCurrentMemoryBlockPointer);
			*pointerToCurrentMemoryBlockPointer = 0;
		}
	}
}

void print_some_numbers(){
	int i;

	print_unsigned_integer(5002);
	uart0_put_string("\n\r");
	print_unsigned_integer(542342);
	uart0_put_string("\n\r");
	print_unsigned_integer(782342);
	uart0_put_string("\n\r");
	print_unsigned_integer(5543242);
	uart0_put_string("\n\r");
	print_unsigned_integer(544342);
	uart0_put_string("\n\r");
	print_unsigned_integer(5442342);
	uart0_put_string("\n\r");
	print_signed_integer(-42);
	uart0_put_string("\n\r");
	print_signed_integer(0);
	uart0_put_string("\n\r");
	print_signed_integer(-0);
	uart0_put_string("\n\r");
	print_signed_integer(2);
	uart0_put_string("\n\r");
	print_signed_integer(666);
	uart0_put_string("\n\r");
	print_signed_integer(345303);
	uart0_put_string("\n\r");
	print_signed_integer(-402);
	uart0_put_string("\n\r");

	for(i = 0; i < 10; i++){
		print_unsigned_integer(get_random() % 200);
		uart0_put_string("\n\r");
	}
}

int	main(){
	SystemInit();
	uart0_init();
	uart0_put_string("Hello World!\n\r");
	init_memory_allocation_table();
	run_memory_tests();
	print_some_numbers();
	uart0_put_string("\nProgram Terminated normally\n\r");
	return 0;
}



