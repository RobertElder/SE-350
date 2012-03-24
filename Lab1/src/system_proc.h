#ifndef _SYSTEM_PROC_H_
#define _SYSTEM_PROC_H_

#define	MAX_NUMBER_OF_REGISTERABLE_COMMANDS             10
#define	MAX_COMMAND_LENGTH                              15


#define CRT_START_STACK ((uint32_t*)(START_STACKS + (NUM_USR_PROCESSES + NUM_I_PROCESSES) * STACKS_SIZE + STACKS_SIZE))
#define KCD_START_STACK (CRT_START_STACK + (STACKS_SIZE) / sizeof(uint32_t))
#define CLOCK_START_STACK (KCD_START_STACK + (STACKS_SIZE) / sizeof(uint32_t))

#include "rtx.h"

int get_seconds_from_formatted_time(char *);
int get_int_from_string(char *);
char* get_formatted_time_from_seconds(int);
void register_command(char * s, int process_id);
void unregister_all_commands(void);
void send_error_message(int sender, char* message);

ProcessControlBlock* get_waiting_sys_proc(void);

//-SYS PROCS-
void keyboard_command_decoder(void);
void crt_display(void);
void wall_clock(void);
void priority_process(void);
//----------

//void init_sys_procs(void);

ProcessControlBlock* get_new_sys_proc(void);

#endif 
