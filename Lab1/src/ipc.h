#ifndef _IPC_H_
#define _IPC_H_

#include <stdint.h>

#include "rtx.h"

typedef struct env {
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
extern void* _receive_message(U32 p_func, int sender) __SVC_0;

#endif
