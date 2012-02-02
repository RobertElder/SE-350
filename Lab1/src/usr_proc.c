#include "rtx.h"
#include "uart_polling.h"

#ifdef DEBUG_0
#include <stdio.h>
#endif // DEBUG_0

void proc1(void)
{

    volatile int i =0;
	volatile int ret_val = 10;
	
    while ( 1) {
        if (i != 0 && i % 5 == 0 ) {
            ret_val = release_processor();
#ifdef DEBUG_0
		    printf("\n\rproc1: ret_val=%d. ", ret_val);
#else
		  	uart0_put_string("\n\r");
#endif // DEBUG_0
        }
        uart0_put_char('A' + i%26);
        i++;
    }

}

void proc2(void)
{
    volatile int i =0;		  
	volatile int ret_val = 20;
    while ( 1) {
        if (i!=0 &&i%5 == 0 ) {
            ret_val = release_processor();
#ifdef DEBUG_0
	    	printf("\n\rproc2: ret_val=%d. ", ret_val);
#else
			uart0_put_string("\n\r");
#endif // DEBUG_0
        }
        uart0_put_char('a' + i%26);
        i++;
    }
}
