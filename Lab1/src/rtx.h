/* @brief: rtx.h User API prototype, this is an example only
 * @author: Yiqing Huang
 * @date: 2012/01/08
  */
#ifndef _RTX_H
#define _RTX_H

#include <stdint.h>

typedef unsigned int U32;		
#define NULL 0
#define __SVC_0  __svc_indirect(0)
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
#define STACKS_SIZE 0x200
#define NUM_PROCESSES 7

// dynamic heap for user processes
// there are 7 user processes plus 2 iprocesses 
//TODO: might need to consider system processes here as well 
#define START_OF_MEMORY_ALLOCATION_TABLE START_STACKS + (NUM_PROCESSES + 2) * STACKS_SIZE
#define START_OF_ALLOCATABLE_MEMORY START_OF_MEMORY_ALLOCATION_TABLE + 0xA
#define MEMORY_BLOCK_SIZE 0x40
#define MAX_ALLOWED_MEMORY_BLOCKS 0x1E

// ----------------------------------------------------------------

#define NUM_PRIORITIES 5

// process states
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_RECEIVE, INTERRUPTED} proc_state_t;

typedef struct list_node {
	 struct list_node* next;
	 void* data;
} ListNode;

typedef struct linked_list {
	ListNode* head;
	ListNode* tail;
} LinkedList;

typedef struct pcb {
  LinkedList waitingMessages;

  // stack pointer of the process
  uint32_t* processStackPointer;

  // process id      
  uint32_t processId;

  // state of the process 		
  proc_state_t currentState;

  //Priority of the process  
  uint32_t processPriority;      

} ProcessControlBlock;


extern LinkedList ready_queue[NUM_PRIORITIES];
extern LinkedList blocked_memory_queue[NUM_PRIORITIES];
extern LinkedList blocked_receive_queue[NUM_PRIORITIES];

void enqueue(LinkedList*, ListNode*);
ListNode* dequeue(LinkedList*);
ListNode* remove_node(LinkedList*, void*);	

#endif
