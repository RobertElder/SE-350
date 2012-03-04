/**
 * @brief Timer example code. Prints 0-9 every second. Loops around when 9 is reached.
 * @author Y. Huang
 * @date 02/15/2012
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "timer.h"
#include "uart_polling.h"  

extern volatile uint32_t g_timer_count;

int main () {

    volatile uint8_t sec = 0;

	SystemInit();
	__disable_irq();
    timer_init(0); // initialize timer 0
    uart0_init();  // polling is used. In your final project, polling is not allowed
	               // check the UART_irq example code to see how to set up UART interrupts.
	__enable_irq();

    while (1) {
        if (g_timer_count == 1000) { // g_timer_count gets updated every 1ms
		    uart0_put_char('0'+ sec);
			sec = (++sec)%10;
            g_timer_count = 0; // reset the counter
		}     
	}
}
