#ifndef _USR_PROC_H_
#define _USR_PROC_H_



#include "rtx.h"
#include "uart.h"
#include "usr_proc.h"
#include "utils.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0


void nullProc(void);

void proc1(void);

void proc2(void);

void run_memory_tests(void);

void run_priority_tests(void);


#endif 
