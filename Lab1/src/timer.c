

#include <LPC17xx.h>
#include "timer.h"
#include "irq.h"
#define BIT(X) (1<<X)



volatile uint32_t g_timer_count = 0; // increment every 1 ms

uint32_t get_current_time() {
	return g_timer_count;
}

/**
 * @brief: initialize timer. Only timer 0 is supported
 */
uint32_t timer_init(uint8_t n_timer) 
{
    LPC_TIM_TypeDef * pTimer;
	if (n_timer == 0) {
        // Steps 1 & 2: system control configuration.
		//              Under CMSIS, system_LPC17xx.c does these two steps
		 
		// Step 1: Power control configuration, table 46 pg63 in LPC17xx_UM
		// enable TIMER0 power, this is the default setting
		// done in system_LPC17xx.c under CMSIS
		// enclose the code for your refrence
	    //LPC_SC->PCONP |= BIT(1);
	
		
		// Step2: select the clock source, default PCLK=CCLK/4 , where CCLK = 100MHZ.
		// tables 40 and 42	on pg56 and pg57 in LPC17xx_UM
		// Check the PLL0 configuration to see how XTAL=12.0MHZ gets to CCLK=100MHZ
		// in system_LPC17xx.c file
		// enclose the code for your reference
		// LPC_SC->PCLKSEL0 &= ~(BIT(3)|BIT(2));	//PCLK = CCLK/4, default setting in system_LPC17xx.c

		// Step 3: Pin Ctrl Block configuration. Optional, not used in this example
		// See Table 82 on pg110 in LPC17xx_UM for pin settings

	    pTimer = (LPC_TIM_TypeDef *) LPC_TIM0;

	} else {// other timer not supported yet
	    return 1;
	}

    // Step 4: Interrupts configuration

    // Step 4.1: Prescale Register PR setting
    pTimer->PR = 12499;  // CCLK = 100 MHZ, PCLK = CCLK/4 = 25 MHZ
                         // 2*(12499 + 1)*(1/25) * 10^(-6) s = 10^(-3) s = 1 ms
						 // TC (Timer Counter) toggles b/w 0 and 1 every 12500 PCLKs
						 // see MR setting below 

    // Step 4.2: MR setting, see section 21.6.7 on page 496 of LPC17xx_UM
    pTimer->MR0 = 1;

    // Step 4.3: MCR setting, see table 429 on page 496 of LPC17xx_UM
    //   Interrupt on MR0: when MR0 mathches the value in the TC, generate an interrupt
    //   Reset on MR0: Reset TC if MR0 mathches it.
    pTimer->MCR = BIT(0) | BIT(1);

    g_timer_count = 0;

    // Step 4.4: CSMSIS enable timer0 IRQ 
    NVIC_EnableIRQ(TIMER0_IRQn);

    // Step 4.5: Enable the TCR. See table 427 on page 494 of LPC17xx_UM
    pTimer->TCR = 1;

    return 0;
}

/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void TIMER0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_TIMER0_IRQHandler
	PUSH{r4-r11, lr}
	BL c_TIMER0_IRQHandler
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR = BIT(0);  // ack inttrupt, see section  21.6.1 on pg 493 of LPC17XX_UM
    g_timer_count++;
	irq_handler(TIMER_IRQ);
}
