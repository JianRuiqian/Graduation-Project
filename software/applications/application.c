/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 * 2014-04-27     Bernard      make code cleanup.
 */

#include <rtthread.h>

void rt_init_thread_entry(void *parameter)
{
#if (defined RT_USING_SDIO) && (RTTHREAD_VERSION < 30000)
    extern void rt_mmcsd_core_init(void);
    rt_mmcsd_core_init();
#endif  /* RT_USING_SDIO */

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_init();
#endif  /* RT_USING_COMPONENTS_INIT */
}

int rt_application_init(void)
{
    rt_thread_t tid;

    tid = rt_thread_create("init",
                           rt_init_thread_entry, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX / 3, 20);

    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}

/*@}*/
