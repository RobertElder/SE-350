
#ifndef _TIMER_H_
#define _TIMER_H_

#define CLOCK_MULTIPLIER 20

extern uint32_t timer_init ( uint8_t n_timer );  // initialize timer n_timer 
uint32_t get_current_time(void);

#endif // ! _TIMER_H_
