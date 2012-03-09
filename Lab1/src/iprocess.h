#include "ipc.h"

int delayed_send(int pid, Envelope * envelope, int delay);

extern int k_delayed_send(int pid, Envelope * envelope, int delay);
#define delayed_send(pid, env, delay) _delayed_send((U32)k_delayed_send, pid, env, delay)
extern int _delayed_send(U32 p_func, int pid, Envelope * env, int delay) __SVC_0;

ProcessControlBlock* get_uart_pcb(void);
ProcessControlBlock* get_timer_pcb(void);

void init_i_processes(void);
void timeout_i_process(void);
