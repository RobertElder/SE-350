
#ifndef _MEMORY_H
#define _MEMORY_H

#include <LPC17xx.h>
#include <system_LPC17xx.h>

#include "rtx.h"

extern void* k_request_memory_block(void);
extern void* k_request_memory_block_debug(char);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
#define request_memory_block_debug(char) _request_memory_block_debug((U32)k_request_memory_block_debug, char)
extern void* _request_memory_block(U32 p_func) __SVC_0;
extern void* _request_memory_block_debug(U32 p_func, char) __SVC_0;
//__SVC_0 can also be put at the end of the function declaration

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void * p_mem_blk) __SVC_0;

void init_memory_allocation_table(void);

extern int numberOfMemoryBlocksCurrentlyAllocated;


#endif
