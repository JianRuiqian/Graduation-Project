/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      first implementation
 */

#include <rthw.h>
#include <rtthread.h>

#include "board.h"
#include "drv_usart.h"

#ifdef RT_USING_PIN
#include "drv_gpio.h"
#endif

/**
 * @addtogroup STM32
 */

/*@{*/

/*******************************************************************************
* Function Name  : NVIC_Configuration
* Description    : Configures Vector Table base location.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else  /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void  SysTick_Configuration(void)
{
    RCC_ClocksTypeDef  rcc_clocks;
    rt_uint32_t         cnts;

    RCC_GetClocksFreq(&rcc_clocks);

    cnts = (rt_uint32_t)rcc_clocks.HCLK_Frequency / RT_TICK_PER_SECOND;
    cnts = cnts / 8;

    SysTick_Config(cnts);
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

void rt_hw_us_delay(rt_uint32_t us)
{
#define USECOND_PER_TICK (1000000 / RT_TICK_PER_SECOND)

    rt_uint32_t delta = 0;
    rt_uint32_t pre_tick = SysTick->VAL;
    rt_uint32_t cur_tick;

    RT_ASSERT(us < USECOND_PER_TICK);

    us = us * (SysTick->LOAD / USECOND_PER_TICK);
    for (;;)
    {
        cur_tick = SysTick->VAL;
        if (cur_tick < pre_tick)
            delta += pre_tick - cur_tick;
        else
            delta += SysTick->LOAD + pre_tick - cur_tick;
        pre_tick = cur_tick;

        if (delta >= us)
            return;
    }
}

/**
 * This function will initial STM32 board.
 */
void rt_hw_board_init()
{
    /* NVIC Configuration */
    NVIC_Configuration();

    /* Configure the SysTick */
    SysTick_Configuration();

    rt_system_heap_init((void *)STM32_SRAM_BEGIN, (void *)STM32_SRAM_END);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#else
    stm32_hw_usart_init();
#ifdef RT_USING_PIN
    stm32_hw_pin_init();
#endif
#endif

#ifdef RT_USING_CONSOLE
    rt_console_set_device(CONSOLE_DEVICE);
#endif
}

void rt_hw_cpu_reset(void)
{
    rt_kprintf("System reset!\n");
    NVIC_SystemReset();
}

#ifdef RT_USING_FINSH
#include <finsh.h>

FINSH_FUNCTION_EXPORT_ALIAS(rt_hw_cpu_reset, reset, software reset);
MSH_CMD_EXPORT_ALIAS(rt_hw_cpu_reset, reset, software reset);
#endif /* RT_USING_FINSH */

/*@}*/
