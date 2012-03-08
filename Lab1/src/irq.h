#ifndef _H_IRQ_
#define _H_IRQ_

typedef enum { UART0_IRQ = 0, TIMER_IRQ } irq_type;

void irq_handler(irq_type);

#endif
