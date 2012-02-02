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
#define USR_SZ_STACK 0x080   // user proc stack size 512 = 0x80 *4 bytes
#endif // DEBUG_0

#define NULL 0
#define INITIAL_xPSR 0x01000000  // user process initial xPSR value

#include <stdint.h>

typedef enum {NEW = 0, RDY, RUN} proc_state_t;  // process states, note we only assume three states in this example

typedef struct pcb {
  
  //Note you may want to add your own member variables
  //in order to finish P1 and the entire project 
  //struct pcb *mp_next;  // next pcb, not used in this example, RTX project most likely will need it, keep here for reference
  
  uint32_t *mp_sp;      // stack pointer of the process
  uint32_t m_pid;		// process id
  proc_state_t m_state; // state of the process       
} pcb_t;


// NOTE the example code uses compile time memory for stack allocation
// This makes the image size quite large. 
// The project requires you to use dynamically allocated memory for
// stack operation. The focus of the example code is for context switching,
// so we use statically allocated stack to simply the code.
uint32_t stack1[USR_SZ_STACK];      // stack for proc1
uint32_t stack2[USR_SZ_STACK];	    // stack for proc2

// NOTE: The example code uses compile time memory for pcb storage.
//       If the system supports dynamica process creation/deletion,
//       then pcb data structure should use dynamically allocated memory
pcb_t pcb1;
pcb_t pcb2;

pcb_t  *gp_current_process = NULL;  // always point to the current process


extern void process_init(void);	    // initialize all procs in the system
int scheduler(void);				// pick the pid of the next to run process
int k_release_process(void);		// kernel release_process API

extern void proc1(void);			// user process 1
extern void proc2(void);			// user process 2
extern void __rte(void);			// pop exception stack frame

#endif // ! _PROCESS_H_
