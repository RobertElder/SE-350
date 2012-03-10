#ifndef _ROBERTS_TESTS_H_
#define _ROBERTS_TESTS_H_

int unit_tests_passed(void);

void register_command(char *, int);
void unregister_all_commands(void);

//  Message API functions
int get_sender_PID(void * p_message);
void set_sender_PID(void * p_message, int value);
int get_destination_PID(void * p_message);
void set_destination_PID(void * p_message, int value);
int get_message_type(void * p_message);
void set_message_type(void * p_message, int value);
void * get_message_data(void * p_message);
void set_message_data(void * p_message, void * data_to_copy, int bytes_to_copy);

#endif
