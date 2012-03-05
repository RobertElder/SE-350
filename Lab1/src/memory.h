/* @brief: rtx.h User API prototype, this is an example only
 * @author: Yiqing Huang
 * @date: 2012/01/08
  */
#ifndef _MEMORY_H
#define _MEMORY_H

#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include "rtx.h"

extern void* k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void* _request_memory_block(U32 p_func) __SVC_0;
//__SVC_0 can also be put at the end of the function declaration

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void * p_mem_blk) __SVC_0;

void init_memory_allocation_table(void);

extern int numberOfMemoryBlocksCurrentlyAllocated;


#endif
