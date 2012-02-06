/* @brief: rtx.c kernel API implementations, this is a skeleton only
 * @author: Yiqing Huang
 * @date: 2012/01/08
 */


#include "rtx.h"


#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

// To better strcture your code, you may want to split these functions
// into different files. For example, memory related kernel APIs in one file
// and process related API(s) in another file.
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file
                                                 // refer to RVCT Linker User Guide


extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;
unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;



int maxNumberOfMemoryBlocksEverAllocatedAtOnce = 0;
extern int numberOfMemoryBlocksCurrentlyAllocated = 0;


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

void * k_request_memory_block (){
	/*  TODO: THIS FUNCTION REQUIRES FURTHER FEATURES IN A FUTURE DELIVERABLE
	The primitive returns a pointer to a memory block to the calling process. If no memory block is available, the calling process
	is blocked until a memory block becomes available. If several processes are waiting for a memory block and a block
	becomes available, the highest priority waiting process will get it.
	*/

	// This variable will point to a byte that will be 
	// 0 if no block is allocated at that index or 
	// 1 if there is a block allocated there
	char * pIsMemoryInUse = (char *)0;
	void * rtn = (void *)0;
	int i = 0;

	//  If there are no more memory blocks, block the calling process (which is the current process)
	if(numberOfMemoryBlocksCurrentlyAllocated == MAX_ALLOWED_MEMORY_BLOCKS){
		//  Block the process
		pCurrentProcessPCB->currentState = BLOCKED_ON_MEMORY;
		pCurrentProcessPCB->processStackPointer = (uint32_t *) __get_MSP();
		//  Switch to another process.  That process will resume after returning from this function
		k_release_processor();
		//assert(0,"not implemented");
	}

	for(i = 0; i < maxNumberOfMemoryBlocksEverAllocatedAtOnce; i++){
		//  Check the bit at the start of the memory allocation table plus the number of bytes
	 	pIsMemoryInUse = get_address_of_memory_block_allocation_status_at_index(i);
		//	Does this byte indicate that the memory at block i is already in use?
		if(*pIsMemoryInUse == 0)	{
			numberOfMemoryBlocksCurrentlyAllocated++;
			return allocate_memory_block_at_index(i);
		}
	}

	assert(maxNumberOfMemoryBlocksEverAllocatedAtOnce < MAX_ALLOWED_MEMORY_BLOCKS,"Too many memory blocks allocated");
	// There are no free blocks that we can use, allocate a new one
	rtn = allocate_memory_block_at_index(maxNumberOfMemoryBlocksEverAllocatedAtOnce);
	// There is now one more block allocated
	maxNumberOfMemoryBlocksEverAllocatedAtOnce++;
	numberOfMemoryBlocksCurrentlyAllocated++;
	//return the pointer
	return rtn;
}

int k_release_memory_block (void * MemoryBlock){
	/*  TODO: THIS FUNCTION REQUIRES FURTHER FEATURES IN A FUTURE DELIVERABLE
	This primitive returns the memory block to the RTX. If there are processes waiting for a block, the block is given to the
	highest priority process, which is then unblocked. The caller of this primitive never blocks, but could be preempted. Thus,
	it may affect the currently executing process.
	*/

	//  Whatever value we are given it should be inside the range of possible allocated blocks
	//  I don't know why you can't just subtract the constant, but for some reason, when you do it will add 0x4000 to the result: WTF?
	int startOfAllocatableMemory = START_OF_ALLOCATABLE_MEMORY;
	int memoryBlockOffset = ((unsigned int)MemoryBlock) - startOfAllocatableMemory;
	int memoryBlockIndex = 0;
			int j;
	char * pAllocationStatusByte = (char *)0;

	// Make sure it is a valid pointer
	if(memoryBlockOffset % MEMORY_BLOCK_SIZE != 0){
		assert(0,"Invalid memory address to release, not divisible by memory block size\n");
		return 1;
	}

	memoryBlockIndex = memoryBlockOffset / MEMORY_BLOCK_SIZE;

	if(memoryBlockIndex > maxNumberOfMemoryBlocksEverAllocatedAtOnce || memoryBlockIndex < 0){
		assert(0,"Invalid memory address to release, outside range\n");
		return 1;
	}

	// Now everything should be k, lets set this block as un-allocated
	pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	// Set the status of this block to unallocated (0)
	*pAllocationStatusByte = 0;

	//  unblock if blocked
	if(numberOfMemoryBlocksCurrentlyAllocated == MAX_ALLOWED_MEMORY_BLOCKS){
		uart0_put_string("unblocking all processes\r\n");

		// Unblock everything
		for(j = 0; j < NUM_PROCESSES; j++){
			if(process_array[j].currentState == BLOCKED_ON_MEMORY)
				process_array[j].currentState = RDY;
		}
		//  Switch to another process.  That process will resume after returning from this function
		numberOfMemoryBlocksCurrentlyAllocated--;
		k_release_processor();
		return 0;
	}


	numberOfMemoryBlocksCurrentlyAllocated--;

	return 0;
}

void init_memory_allocation_table(){
	assert(MAX_ALLOWED_MEMORY_BLOCKS * MEMORY_BLOCK_SIZE < 0x100 * 0x60,"You set the values for memory sizes too big, You might be overwritting someones data.");
}
