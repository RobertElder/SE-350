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

void assert(int value, unsigned char * message){
	if(value == 0){
		uart0_put_string("\nTHERE WAS AN ASSERTION FAILURE!!!\n\n");
		uart0_put_string(message);
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
	int memoryBlockOffset = ((int)MemoryBlock) - START_OF_ALLOCATABLE_MEMORY;
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
	// This test assumes that they were allocated in consecutive order

	int i;


	for(i=0; i < 10; i++){
		int * pInt = request_memory_block();
		assert((int)pInt, "Request memory returned null");
		*pInt = i;
	}

	for(i=0; i < 10; i++){
		int * pInt = get_address_of_memory_block_at_index(i);
		assert((*pInt) == i, "the first memory test failed.");
	}
}

int	main(){
	SystemInit();
	uart0_init();
	uart0_put_string("Hello World!\n\r");
	init_memory_allocation_table();
	run_memory_tests();
	uart0_put_string("\nProgram Terminated normally\n\r");
	return 0;
}



