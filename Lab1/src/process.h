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

  typedef unsigned int U32;

#define __SVC_0  __svc_indirect(0)

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
//extern int __SVC_0 _release_processor(U32 p_func);
int __SVC_0 _release_processor(U32 p_func);

extern int k_set_process_priority(int, int);
#define set_process_priority(process_ID, priority) _set_process_priority((U32)k_set_process_priority, process_ID, priority)
int __SVC_0 _set_process_priority(U32 p_func, int process_ID, int priority);

extern int k_get_process_priority(int);
#define get_process_priority(process_ID) _get_process_priority((U32)k_set_process_priority, process_ID)
int __SVC_0 _get_process_priority(U32 p_func, int process_ID);

extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file
                                                 // refer to RVCT Linker User Guide
extern unsigned int free_mem;

//---------------------------------------------------------------
//
//             MEMORY MAP
//       tread lightly, here be dragons	
//
//----------------------------------------------------------------
							

// user process stack size 512 = 0x80 *4 bytes	 (128 4-byte words)
#define START_STACKS free_mem
#define STACKS_SIZE 0x080
#define NUM_PROCESSES 5

// dynamic heap for user processes
#define START_OF_MEMORY_ALLOCATION_TABLE START_STACKS + NUM_PROCESSES * STACKS_SIZE
#define START_OF_ALLOCATABLE_MEMORY START_OF_MEMORY_ALLOCATION_TABLE + 0xA
#define MEMORY_BLOCK_SIZE 0x10
#define MAX_ALLOWED_MEMORY_BLOCKS 0x1

// ---------------------------------------------------------------


						  



// --------------------------------------------------------
// Data structures
// --------------------------------------------------------

#define NUM_PRIORITIES 4

// process states
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY} proc_state_t;




typedef struct pcb {
  
  struct pcb* next;
  
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
  uint32_t* start_sp;

  // Where process begins
  uint32_t* process;
        
  //need another variable for i-process (indicates if this is an i-process or not)  
} ProcessEntry;

extern void init_processe_table(void);

//extern ProcessControlBlock pcb_array[NUM_PROCESSES];

typedef struct queue_head {
	ProcessControlBlock* head;
	ProcessControlBlock* tail;
} QueueHead;


// NOTE: The example code uses compile time memory for pcb storage.
//       If the system supports dynamica process creation/deletion,
//       then pcb data structure should use dynamically allocated memory

extern ProcessControlBlock * pCurrentProcessPCB;  // always point to the current process

extern QueueHead ready_queue[NUM_PRIORITIES];
extern QueueHead blocked_queue[NUM_PRIORITIES];

// -----------------------------------------------------
// Public routines
// -----------------------------------------------------

extern ProcessControlBlock * get_process_pointer_from_id(int);

void enqueue(QueueHead*, ProcessControlBlock*);
ProcessControlBlock* dequeue(QueueHead* );										
extern void process_init(void);	    // initialize all procs in the system
ProcessControlBlock* scheduler(ProcessControlBlock* pOldPCB, ProcessControlBlock* pNewPCB);				// pick the pid of the next to run process
int k_release_processor(void);		// kernel release_process API
int has_blocked_processes(void); // check if there are blocked processes
void block_current_process(void); // Put current process in a blocking state, as well as enqueue in the blocked queue
void context_switch(ProcessControlBlock*, ProcessControlBlock*); // Switch contexts from the passed-in PCB to the pCurrentPCB
ProcessControlBlock* getBlockedProcess(void); //Gets the highest priority blocked process
ProcessControlBlock* getRunningProcess(void); //Gets a running process from all processes array

// ------------------------------------------------------
// External routines
// ------------------------------------------------------
extern void p1(void);
extern void p2(void);
extern void p3(void);
extern void p4(void);
extern void pp1(void);
extern void pp2(void);
extern void proc1(void);			// user process 1
extern void proc2(void);			// user process 2
extern void run_block_memory_test(void);			// user process 1
extern void memory_request_process(void);			// user process 2
extern void nullProc(void);				// null process
extern void run_priority_tests(void);
extern void run_memory_tests(void);
extern void __rte(void);			// pop exception stack frame

#endif // ! _PROCESS_H_
