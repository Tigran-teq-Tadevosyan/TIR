/*
 * system_timer.h
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */

#ifndef SYSTEMTIMER_SYSTEM_TIMER_H_
#define SYSTEMTIMER_SYSTEM_TIMER_H_

#include <stddef.h>
#include <stdint.h>

#include <ti_radio_config.h>

#define SYTEM_TIMESLOT_DURATION_US  (2500)
#define SYTEM_TICK_US   (10)
#define SYTEM_TIMESLOT_DURATION  (SYTEM_TIMESLOT_DURATION_US / SYTEM_TICK_US)


#define TX_START_DURATION_US  (400)
#define TX_START_DURATION  (TX_START_DURATION_US / SYTEM_TICK_US)

#define RAT_TO_SYSTICK(tick)  (RF_convertRatTicksToUs(tick) / SYTEM_TICK_US)



void systemTimerInit(void);

void releaseCounters(uint16_t passedTime);
void fixCounters(void);



extern uint32_t system_timer_tick;
extern uint32_t self_timer_tick;


#endif /* SYSTEMTIMER_SYSTEM_TIMER_H_ */
