#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"

extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;
unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;

#define START_OF_MEMORY_ALLOCATION_TABLE free_mem
#define START_OF_ALLOCATABLE_MEMORY START_OF_MEMORY_ALLOCATION_TABLE + 0x00002000
#define MEMORY_BLOCK_SIZE 0x100

#define MAX_ALLOWED_MEMORY_BLOCKS 0x100

int * pMaxNumberOfMemoryBlocksEverAllocated;

int RANDOM_NUMBER_GENERATOR_m_w = 7;    /* must not be zero */
int RANDOM_NUMBER_GENERATOR_m_z = 123;    /* must not be zero */


void assert(int value, unsigned char * message){
	if(value == 0){
		uart0_put_string("\nTHERE WAS AN ASSERTION FAILURE!!!\n");
		uart0_put_string(message);
		uart0_put_string("\nAbnormal program termination.\n");
		// I can't figure out any other way to get it to stop executing.
		while (1) {}
	}
}
 
unsigned int get_random(){
	/* This is a shitty way to generate random numbers but it works for what we need */
    RANDOM_NUMBER_GENERATOR_m_z = 36969 * (RANDOM_NUMBER_GENERATOR_m_z & 65535) + (RANDOM_NUMBER_GENERATOR_m_z >> 16);
    RANDOM_NUMBER_GENERATOR_m_w = 18000 * (RANDOM_NUMBER_GENERATOR_m_w & 65535) + (RANDOM_NUMBER_GENERATOR_m_w >> 16);
    return (RANDOM_NUMBER_GENERATOR_m_z << 16) + RANDOM_NUMBER_GENERATOR_m_w;  /* 32-bit result */
}

unsigned int pow(unsigned int base, unsigned int exponent){
	// Big surprise we have to write our own pow function
	if(exponent == 0)
		return 1;
	else
		return base * pow(base,exponent - 1);
}

void printDigit(unsigned int i){
	unsigned char digits[] = "0123456789";
	assert(i < 10, "This should not happen");
	//Put a null after the digit we want so that we only print that digit
	digits[i+1] = 0;
	uart0_put_string(&(digits[i]));
}

void print_unsigned_integer(unsigned int i){

	int hasSeenADigitYet = 0;
	// Max int is 4,294,967,295
	int currentExponent = 9;

	if(i == 0){
		printDigit(i);
		return;
	}

	while(currentExponent >= 0){
		// What value does a digit have for this exponent? 
		int currentDigitValue = pow(10,currentExponent); 
		//  What is the value for this digit?
		int currentDigit = i / currentDigitValue;
		//  We have accounted for this quantity in our representation, remove it
		i -= currentDigit * currentDigitValue;

		//  Don't print leading 0's but do print interior zeros
		if(hasSeenADigitYet || currentDigit){
			//  Always print 0's now
			hasSeenADigitYet = 1;
			//  Determines if P = NP
			printDigit(currentDigit);
		}
		// Next value 
		currentExponent--;
	}
}

void print_signed_integer(signed int i){
	if(i < 0){
		uart0_put_string("-");
		print_unsigned_integer((unsigned int)(-i));
	}else{
		print_unsigned_integer((unsigned int)i);		
	}
}



void * get_address_of_memory_block_at_index(int memoryBlockIndex){
	//  taken up by pMaxNumberOfMemoryBlocksEverAllocated plus the offset of the byte for that memory block
	return (void*)(START_OF_ALLOCATABLE_MEMORY + (memoryBlockIndex * MEMORY_BLOCK_SIZE));
}

char * get_address_of_memory_block_allocation_status_at_index(int memoryBlockIndex){
	return (char *)(START_OF_MEMORY_ALLOCATION_TABLE + sizeof(int) + (sizeof(char) * memoryBlockIndex));
}

void * allocate_memory_block_at_index(int memoryBlockIndex){
	// memoryBlockIndex is the 0-based index addressing the block of memory
	char * pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	// Set the status of this block to allocated (1)
	*pAllocationStatusByte = 1;
	return get_address_of_memory_block_at_index(memoryBlockIndex);
}

void * request_memory_block (){
	// This variable will point to a byte that will be 
	// 0 if no block is allocated at that index or 
	// 1 if there is a block allocated there
	char * pIsMemoryInUse = (char *)0;
	void * rtn = (void *)0;

	int i = 0;
	for(i = 0; i < *pMaxNumberOfMemoryBlocksEverAllocated; i++){
		//  Check the bit at the start of the memory allocation table plus the number of bytes
	 	pIsMemoryInUse = get_address_of_memory_block_allocation_status_at_index(i);
		//	Does this byte indicate that the memory at block i is already in use?
		if(*pIsMemoryInUse == 0)	{
			return allocate_memory_block_at_index(i);
		}
	}

	assert(*pMaxNumberOfMemoryBlocksEverAllocated < MAX_ALLOWED_MEMORY_BLOCKS,"Too many memory blocks allocated");
	// There are no free blocks that we can use, allocate a new one
	rtn = allocate_memory_block_at_index(*pMaxNumberOfMemoryBlocksEverAllocated);
	// There is now one more block allocated
	(*pMaxNumberOfMemoryBlocksEverAllocated)++;
	//return the pointer
	return rtn;
}

int release_memory_block (void * MemoryBlock){
	// Whatever value we are given it should be inside the range of possible allocated blocks
	//  I don't know why you can't just subtract the constant, but for some reason, when you do it will add 0x4000 to the result: WTF?
	int startOfAllocatableMemory = START_OF_ALLOCATABLE_MEMORY;
	int memoryBlockOffset = ((unsigned int)MemoryBlock) - startOfAllocatableMemory;
	int memoryBlockIndex = 0;
	char * pAllocationStatusByte = (char *)0;

	// Make sure it is a valid pointer
	if(memoryBlockOffset % MEMORY_BLOCK_SIZE != 0){
		assert(0,"Invalid memory address to release, not divisible by memory block size\n");
		return 1;
	}

	memoryBlockIndex = memoryBlockOffset / MEMORY_BLOCK_SIZE;

	if(memoryBlockIndex > *pMaxNumberOfMemoryBlocksEverAllocated || memoryBlockIndex < 0){
		assert(0,"Invalid memory address to release, outside range\n");
		return 1;
	}

	// Now everything should be k, lets set this block as un-allocated
	pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	// Set the status of this block to unallocated (0)
	*pAllocationStatusByte = 0;

	return 0;
}

void init_memory_allocation_table(){
	pMaxNumberOfMemoryBlocksEverAllocated	= (int *)START_OF_MEMORY_ALLOCATION_TABLE;
	*pMaxNumberOfMemoryBlocksEverAllocated = 0;
}



void run_memory_tests(){

	int testCases = 100000;
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



