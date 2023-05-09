/*
 * system_timer.c
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */




#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>

/* Board Header file */
#include "ti_drivers_config.h"
#include "system_timer.h"

uint32_t system_timer_tick = 0;

/* Callback used for toggling the LED. */
void timerCallback(Timer_Handle myHandle, int_fast16_t status);


void systemTimerInit(void)
{
    Timer_Handle timer0;
    Timer_Params params;

    Timer_init();

    /*
     * Setting up the timer in continuous callback mode that calls the callback
     * function every 1,000 microseconds, or 1 ms.
     */
    Timer_Params_init(&params);
    params.period        = SYTEM_TICK_US;
    params.periodUnits   = Timer_PERIOD_US;
    params.timerMode     = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timerCallback;


    timer0 = Timer_open(SYNC_TIMER, &params);

    if (timer0 == NULL)
    {
        /* Failed to initialized timer */
        while (1) {}
    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR)
    {
        /* Failed to start timer */
        while (1) {}
    }
}

/*
 * This callback is called every 1,000,000 microseconds, or 1 second. Because
 * the LED is toggled each time this function is called, the LED will blink at
 * a rate of once every 2 seconds.
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{
    system_timer_tick++;
    if(system_timer_tick % (1000 / SYTEM_TICK_US) == 0)
    {
      //GPIO_toggle(CONFIG_GPIO_GLED);
      if(system_timer_tick % (2000 / SYTEM_TICK_US) == 0)
      {
          GPIO_write(CONFIG_GPIO_GLED, 0);
      }
      else
          GPIO_write(CONFIG_GPIO_GLED, 1);
    }
}
