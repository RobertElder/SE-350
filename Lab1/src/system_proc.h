#ifndef _SYSTEM_PROC_H_
#define _SYSTEM_PROC_H_

#define	MAX_NUMBER_OF_REGISTERABLE_COMMANDS             10
#define	MAX_COMMAND_LENGTH                              500



void register_command(char *, int);
void unregister_all_commands(void);
void keyboard_command_decoder(void * message);
void crt_display(void);

#endif 
