#include "ipc.h"

int delayed_send(int pid, Envelope * envelope, int delay);

void timeout_i_process();