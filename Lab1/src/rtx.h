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
#define START_OF_MEMORY_ALLOCATION_TABLE START_STACKS + NUM_PROCESSES * STACKS_SIZE
#define START_OF_ALLOCATABLE_MEMORY START_OF_MEMORY_ALLOCATION_TABLE + 0xA
#define MEMORY_BLOCK_SIZE 0x40
#define MAX_ALLOWED_MEMORY_BLOCKS 0x1E

// ----------------------------------------------------------------

#define NUM_PRIORITIES 5

// process states
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_RECEIVE} proc_state_t;

typedef struct list_node {
	 struct list_node* next;
	 void* data;
} ListNode;

typedef struct linked_list {
	ListNode* head;
	ListNode* tail;
} LinkedList;

typedef struct pcb {
  
  struct pcb* next;

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

typedef struct queue_head {
	ProcessControlBlock* head;
	ProcessControlBlock* tail;
} QueueHead;

extern QueueHead ready_queue[NUM_PRIORITIES];
extern QueueHead blocked_memory_queue[NUM_PRIORITIES];
extern QueueHead blocked_receive_queue[NUM_PRIORITIES];

void enqueue(QueueHead*, ProcessControlBlock*);
void enqueue_ll(LinkedList*, ListNode*);
ProcessControlBlock* dequeue(QueueHead*);
ListNode* dequeue_ll(LinkedList*);
void remove_proc(QueueHead*, ProcessControlBlock*);	

#endif
