#include "ipc.h"

int delayed_send(int pid, Envelope * envelope, int delay);

ProcessControlBlock* get_uart_pcb(void);
ProcessControlBlock* get_timer_pcb(void);

void init_i_processes(void);
void timeTEMP(void);
