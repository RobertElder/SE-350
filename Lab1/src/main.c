#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "utils.h"
#include "rtx.h"






int	main2(){
	SystemInit();
	uart0_init();
	uart0_put_string("Hello World!\n\r");
	init_memory_allocation_table();
	run_memory_tests();
	print_some_numbers();
	uart0_put_string("\nProgram Terminated normally\n\r");
	return 0;
}




