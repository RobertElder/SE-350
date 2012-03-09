#ifndef _SYSTEM_PROC_H_
#define _SYSTEM_PROC_H_

#define	MAX_NUMBER_OF_REGISTERABLE_COMMANDS             10
#define	MAX_COMMAND_LENGTH                              500

int number_of_registered_commands = 0;
char registered_commands[MAX_NUMBER_OF_REGISTERABLE_COMMANDS][MAX_COMMAND_LENGTH];
int registered_processes[MAX_NUMBER_OF_REGISTERABLE_COMMANDS];

void register_command(char *, int);
void keyboard_command_decoder(void * message);
void crt_display(void);

#endif 
