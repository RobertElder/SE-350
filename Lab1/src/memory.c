#include "memory.h"
#include "utils.h" 
#include "process.h"

int maxNumberOfMemoryBlocksEverAllocatedAtOnce = 0;
extern int numberOfMemoryBlocksCurrentlyAllocated = 0;
char allocated_status_value = DEFAULT_ALLOCATED_STATUS_BYTE;

void * get_address_of_memory_block_at_index(int memoryBlockIndex){
	//  taken up by pMaxNumberOfMemoryBlocksEverAllocated plus the offset of the byte for that memory block
	return (void*)(START_OF_ALLOCATABLE_MEMORY + (memoryBlockIndex * MEMORY_BLOCK_SIZE));
}

char * get_address_of_memory_block_allocation_status_at_index(int memoryBlockIndex){
	return (char *)(START_OF_MEMORY_ALLOCATION_TABLE + (sizeof(char) * memoryBlockIndex));
}

void * allocate_memory_block_at_index(int memoryBlockIndex){
	// memoryBlockIndex is the 0-based index addressing the block of memory
	char * pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	// Set the status of this block to allocated (1)
	*pAllocationStatusByte = allocated_status_value;
	return get_address_of_memory_block_at_index(memoryBlockIndex);
}

void request_mem_block_helper() {
	if (pCurrentProcessPCB->currentState == BLOCKED_ON_MEMORY) {
		ProcessControlBlock* runningProcess = getRunningProcess();

		ProcessControlBlock* pNewProcessPCB = (ProcessControlBlock*)dequeue(
			&(blocked_memory_queue[pCurrentProcessPCB->processPriority]))->data;
		assert(pCurrentProcessPCB == pNewProcessPCB, "ERROR: blocked queue and process priorities not in sync.");	

		if (runningProcess->processPriority <= pCurrentProcessPCB->processPriority) {
		 	pCurrentProcessPCB->currentState = RDY;
			enqueue(&(ready_queue[pCurrentProcessPCB->processPriority]),
				get_node_of_process(pCurrentProcessPCB->processId));
			context_switch(pCurrentProcessPCB, runningProcess);
		} else {
			runningProcess->currentState = RDY;
			enqueue(&(ready_queue[runningProcess->processPriority]), 
				get_node_of_process(runningProcess->processId));
			pCurrentProcessPCB->currentState = RUN;	
		}

	}
}

void * k_request_memory_block_debug(char c){
	void * p = NULL;
	allocated_status_value = c;
	p = k_request_memory_block();
	allocated_status_value = DEFAULT_ALLOCATED_STATUS_BYTE;
	return p;
}

void * k_request_memory_block (){
	/*  THIS FUNCTION REQUIRES FURTHER FEATURES IN A FUTURE DELIVERABLE
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
		//  Block and switch to another process.
		block_current_process();
		k_release_processor();
	}

	for(i = 0; i < maxNumberOfMemoryBlocksEverAllocatedAtOnce; i++){
		//  Check the bit at the start of the memory allocation table plus the number of bytes
	 	pIsMemoryInUse = get_address_of_memory_block_allocation_status_at_index(i);
		//	Does this byte indicate that the memory at block i is already in use?
		if (*pIsMemoryInUse == 0) {
			void *p;
			numberOfMemoryBlocksCurrentlyAllocated++;
			p = allocate_memory_block_at_index(i);
			request_mem_block_helper();
			return p;
		}
	}

	assert(maxNumberOfMemoryBlocksEverAllocatedAtOnce < MAX_ALLOWED_MEMORY_BLOCKS, "ERROR: Too many memory blocks allocated");
	// There are no free blocks that we can use, allocate a new one
	rtn = allocate_memory_block_at_index(maxNumberOfMemoryBlocksEverAllocatedAtOnce);
	// There is now one more block allocated
	maxNumberOfMemoryBlocksEverAllocatedAtOnce++;
	numberOfMemoryBlocksCurrentlyAllocated++;
	request_mem_block_helper();
	//return the pointer
	return rtn;
}

int k_release_memory_block (void * MemoryBlock){
	/*  THIS FUNCTION REQUIRES FURTHER FEATURES IN A FUTURE DELIVERABLE
	This primitive returns the memory block to the RTX. If there are processes waiting for a block, the block is given to the
	highest priority process, which is then unblocked. The caller of this primitive never blocks, but could be preempted. Thus,
	it may affect the currently executing process.
	*/

	//  Whatever value we are given it should be inside the range of possible allocated blocks
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

	if(memoryBlockIndex > maxNumberOfMemoryBlocksEverAllocatedAtOnce || memoryBlockIndex < 0){
		assert(0,"Invalid memory address to release, outside range\n");
		return 1;
	}

	// Now everything should be k, lets set this block as un-allocated
	pAllocationStatusByte = get_address_of_memory_block_allocation_status_at_index(memoryBlockIndex);
	assert(*pAllocationStatusByte,"Allocation status byte indicated deallocaton of unallocated memory.");
	// Set the status of this block to unallocated (0)
	*pAllocationStatusByte = 0;

	//  unblock if blocked}
	if(numberOfMemoryBlocksCurrentlyAllocated == MAX_ALLOWED_MEMORY_BLOCKS && has_blocked_processes()){
		ProcessControlBlock* blockedProcessPCB;

		//  Switch to another process.  That process will resume after returning from this function
		numberOfMemoryBlocksCurrentlyAllocated--;
		
		//get highest pririty blocked process; sys_procs blocked on mem get priority
		blockedProcessPCB = getBlockedProcess();
		//context switch from running process to blocked process
		context_switch(pCurrentProcessPCB, blockedProcessPCB);
		
		return 0;
	}

	numberOfMemoryBlocksCurrentlyAllocated--;
	return 0;
}

void init_memory_allocation_table(){
	assert(MAX_ALLOWED_MEMORY_BLOCKS * MEMORY_BLOCK_SIZE < 0x100 * 0x60,"You set the values for memory sizes too big, You might be overwritting someones data.");
}

