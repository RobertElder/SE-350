#include "rtx.h"
#include "timer.h"
#include "ipc.h"

QueueHead delayed_messages;

typedef struct delayedMessage {
	int pid;
	Envelope * envelope;
	int expiry_time;
} DelayedMessage;

extern volatile uint32_t g_timer_count;

uint32_t get_current_time() {
	return g_timer_count;
}

int delayed_send(int pid, Envelope * envelope, int delay) {
	DelayedMessage * m;
	m->pid = pid;
	m->envelope = envelope;
	m->expiry_time = get_current_time() + delay;

	//add m to timeoutProcess message queue

	return 0;
}

void timeout_i_process() {
	Envelope * env;// = receive_message();

	while(env != NULL) {
	 	//enqueue(&delayed_messages, env);
		//env = receive_message();
	}

	if(delayed_messages.head != NULL) {
//		while (head_of_timeout_list.expiry_time <= cur_time)
//		{
//			env = timeout_list.dequeue();
//			target_pid = env.destination_pid;
//			send( target_pid, env ); //forward msg to destination
//		}						
	}

}
