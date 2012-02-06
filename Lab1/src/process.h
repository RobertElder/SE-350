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

extern ProcessControlBlock process_array[NUM_PROCESSES];

// TODO the example code uses compile time memory for stack allocation
// This makes the image size quite large. 
// The project requires you to use dynamically allocated memory for
// stack operation. The focus of the example code is for context switching,
// so we use statically allocated stack to simplify the code.

/* Variable declarations */
extern uint32_t stack0[USR_SZ_STACK];      // stack for nullProc
extern uint32_t stack1[USR_SZ_STACK];      // stack for proc1
extern uint32_t stack2[USR_SZ_STACK];	    // stack for proc2
extern uint32_t stack3[USR_SZ_STACK];      // stack for run_priority_tests
extern uint32_t stack4[USR_SZ_STACK];      // stack for run_memory_tests

// NOTE: The example code uses compile time memory for pcb storage.
//       If the system supports dynamica process creation/deletion,
//       then pcb data structure should use dynamically allocated memory

extern ProcessControlBlock * pCurrentProcessPCB;  // always point to the current process

extern ProcessControlBlock * get_process_pointer_from_id(int);
extern int set_process_priority (int, int);
extern int get_process_priority (int);
										
extern void process_init(void);	    // initialize all procs in the system
int scheduler(void);				// pick the pid of the next to run process
int k_release_process(void);		// kernel release_process API

extern void memory_request_process(void);			//
extern void run_block_memory_test(void);			//
extern void nullProc(void);				// null process
extern void run_priority_tests(void);
extern void run_memory_tests(void);
extern void __rte(void);			// pop exception stack frame

#endif // ! _PROCESS_H_
