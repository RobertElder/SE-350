/**
 * @brief: process.h  process management hearder file
 * @author Irene Huang
 * @author Thomas Reidemeister
 * @date 2011/01/18
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef _PROCESS_H_
#define _PROCESS_H_

#define NULL 0
#define INITIAL_xPSR 0x01000000  // user process initial xPSR value

#include <stdint.h>



extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file
                                                 // refer to RVCT Linker User Guide
extern unsigned int free_mem;

//---------------------------------------------------------------
//
//             MEMORY MAP
//       tread lightly, here be dragons	
//
//----------------------------------------------------------------
							
// dynamic memory allocation space for the kernel
#define START_KERNEL_HEAP free_mem
#define KERNAL_HEAP_BLOCK_SIZE 0x10
#define KERNEL_HEAP_SIZE 0x200

// user process stack size 512 = 0x80 *4 bytes	 (128 4-byte words)
#define START_STACKS START_KERNEL_HEAP + KERNEL_HEAP_SIZE
#define STACKS_SIZE 0x080
#define NUM_PROCESSES 5

// dynamic heap for user processes
#define START_OF_MEMORY_ALLOCATION_TABLE START_STACKS + NUM_PROCESSES * STACKS_SIZE
#define START_OF_ALLOCATABLE_MEMORY START_OF_MEMORY_ALLOCATION_TABLE + 0xA
#define MEMORY_BLOCK_SIZE 0x10
#define MAX_ALLOWED_MEMORY_BLOCKS 0x1E

// ---------------------------------------------------------------


						  



// --------------------------------------------------------
// Data structures
// --------------------------------------------------------

// process states
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY} proc_state_t;

typedef struct pcb {
  
  //Note you may want to add your own member variables
  //in order to finish P1 and the entire project 
  //struct ProcessControlBlock *mp_next;  // next ProcessControlBlock, not used in this example, RTX project most likely will need it, keep here for reference
  
  // stack pointer of the process
  uint32_t* processStackPointer;

  // process id      
  uint32_t processId;

  // state of the process 		
  proc_state_t currentState;

  //Priority of the process  
  uint32_t processPriority;      

} ProcessControlBlock;
 
typedef struct processEntry {
// process id
  uint32_t pid;	
  
  //Priority of the process 			
  uint32_t priority;
  
  // Size of the process stack 		  
  uint32_t stack_size;
  
  // stack pointer to the start(top) of the process stack  
  uint32_t start_sp;
        
  //need another variable for i-process (indicates if this is an i-process or not)  
} init_t;

extern void* k_init_processes_to_create(void);

extern ProcessControlBlock pcb_array[NUM_PROCESSES];


// NOTE: The example code uses compile time memory for pcb storage.
//       If the system supports dynamica process creation/deletion,
//       then pcb data structure should use dynamically allocated memory

extern ProcessControlBlock * pCurrentProcessPCB;  // always point to the current process
extern int isMemBlockJustReleased;

// -----------------------------------------------------
// Public routines
// -----------------------------------------------------

extern ProcessControlBlock * get_process_pointer_from_id(int);
extern int set_process_priority (int, int);
extern int get_process_priority (int);
										
extern void process_init(void);	    // initialize all procs in the system
int scheduler(void);				// pick the pid of the next to run process
int k_release_process(void);		// kernel release_process API
int has_blocked_processes(void); // check if there are blocked processes

// ------------------------------------------------------
// External routines
// ------------------------------------------------------

extern void proc1(void);			// user process 1
extern void proc2(void);			// user process 2
extern void run_block_memory_test(void);			// user process 1
extern void memory_request_process(void);			// user process 2
extern void nullProc(void);				// null process
extern void run_priority_tests(void);
extern void run_memory_tests(void);
extern void __rte(void);			// pop exception stack frame

#endif // ! _PROCESS_H_
