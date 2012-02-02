/* @brief: rtx.c kernel API implementations, this is a skeleton only
 * @author: Yiqing Huang
 * @date: 2012/01/08
 */

#include "uart_polling.h"
#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

// To better strcture your code, you may want to split these functions
// into different files. For example, memory related kernel APIs in one file
// and process related API(s) in another file.
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit;  // symbol defined in the scatter file
                                                 // refer to RVCT Linker User Guide


void* k_request_memory_block(void) {
     #ifdef DEBUG_0
	 unsigned int free_mem = (unsigned int) &Image$$RW_IRAM1$$ZI$$Limit;
	 printf("free mem starts at 0x%x\n", free_mem);
	 #endif // DEBUG_0
	 return (void *)0;
}

int k_release_memory_block(void* p_mem_blk) {
     return 0;
}
