#ifndef _IPC_H_
#define _IPC_H_

#include <stdint.h>
#include "rtx.h"

#define COMMAND_REGISTRATION 1
#define COMMAND_INPUT 2
#define KEYBOARD_INPUT 3
#define OUTPUT_STRING 4
#define DELAYED_SEND 5
#define START_CLOCK	6
#define STOP_CLOCK 7
#define CLOCK_TICK 8
#define PAUSE_CLOCK 9
#define UNPAUSE_CLOCK 10

typedef struct env {
	ListNode dummyVar; // DO NOT USE ME.
	uint32_t sender_pid;
	uint32_t receiver_pid;
	uint32_t message_type;
	uint32_t message_data; 
} Envelope;

extern int k_send_message(int target_pid, void* envelope);
#define send_message(pid, env) _send_message((U32)k_send_message, pid, env)
extern int _send_message(U32 p_func, int pid, void* env) __SVC_0;
																												
extern void* k_receive_message(int* sender_ID);
#define receive_message(sender) _receive_message((U32)k_receive_message, sender)
extern void* _receive_message(U32 p_func, int* sender) __SVC_0;

//  Message API functions
int get_sender_PID(void * p_message);
void set_sender_PID(void * p_message, int value);
int get_destination_PID(void * p_message);
void set_destination_PID(void * p_message, int value);
int get_message_type(void * p_message);
void set_message_type(void * p_message, int value);
void * get_message_data(void * p_message);
void set_message_bytes(void * p_message, void * data_to_copy, int bytes_to_copy);
void set_message_words(void * p_message, void * data_to_copy, int words_to_copy);


#endif
