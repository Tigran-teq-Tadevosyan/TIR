/*
 * system_timer.h
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */

#ifndef SYSTEMTIMER_SYSTEM_TIMER_H_
#define SYSTEMTIMER_SYSTEM_TIMER_H_

#include <stddef.h>

#define SYTEM_TICK_US   10

void systemTimerInit(void);




extern uint32_t system_timer_tick;


#endif /* SYSTEMTIMER_SYSTEM_TIMER_H_ */
