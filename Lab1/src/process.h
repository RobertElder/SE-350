/**
 * @brief: process.h  process management hearder file
 * @author Irene Huang
 * @author Thomas Reidemeister
 * @date 2011/01/18
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef _PROCESS_H_
#define _PROCESS_H_

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200   // user proc stack size 2048 = 0x200 *4 bytes
#else
#define USR_SZ_STACK 0x080   // user process stack size 512 = 0x80 *4 bytes
#endif // DEBUG_0

#define NULL 0
#define INITIAL_xPSR 0x01000000  // user process initial xPSR value

#include <stdint.h>

#define NUM_PROCESSES 5

extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file
                                                 // refer to RVCT Linker User Guide
extern unsigned int free_mem;

typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY} proc_state_t;  // process states, note we only assume three states in this example

typedef struct pcb {
  
  //Note you may want to add your own member variables
  //in order to finish P1 and the entire project 
  //struct ProcessControlBlock *mp_next;  // next ProcessControlBlock, not used in this example, RTX project most likely will need it, keep here for reference
  
  uint32_t * processStackPointer;      // stack pointer of the process
  uint32_t processId;		// process id
  proc_state_t currentState; // state of the process  
  uint32_t processPriority; //Priority of the process     

} ProcessControlBlock;
 
typedef struct processEntry {
  uint32_t pid;				// process id
  uint32_t priority; 		//Priority of the process   
  uint32_t stack_size;  
  uint32_t start_sp;      // stack pointer to the start of the process stack?
  //need another variable for i-process (indicates if this is an i-process or not)  
} init_t;

extern void* k_init_processes_to_create(void);


extern ProcessControlBlock pcb_array[NUM_PROCESSES];


// NOTE: The example code uses compile time memory for pcb storage.
//       If the system supports dynamica process creation/deletion,
//       then pcb data structure should use dynamically allocated memory

extern ProcessControlBlock * pCurrentProcessPCB;  // always point to the current process
extern int isMemBlockJustReleased;

extern ProcessControlBlock * get_process_pointer_from_id(int);
extern int set_process_priority (int, int);
extern int get_process_priority (int);
										
extern void process_init(void);	    // initialize all procs in the system
int scheduler(void);				// pick the pid of the next to run process
int k_release_process(void);		// kernel release_process API
int has_blocked_processes(void); // check if there are blocked processes

extern void proc1(void);			// user process 1
extern void proc2(void);			// user process 2
extern void nullProc(void);				// null process
extern void run_priority_tests(void);
extern void run_memory_tests(void);
extern void __rte(void);			// pop exception stack frame

#endif // ! _PROCESS_H_
