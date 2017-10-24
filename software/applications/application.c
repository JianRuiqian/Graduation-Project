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

#include <board.h>
#include <rtthread.h>
#include <msh.h>

#ifdef RT_USING_DFS
#include <dfs_fs.h>
#ifdef RT_USING_DFS_MNTTABLE
const struct dfs_mount_tbl mount_table[] = {
    {"flash0", "/", "elm", 0, 0},
    {0}
};
#endif
#endif

void rt_init_thread_entry(void* parameter)
{
    /* Platform Initialization */
    extern void rt_platform_init(void);
    rt_platform_init();
    
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_init();
#endif  /* RT_USING_COMPONENTS_INIT */
    
    /* execute script for wifi */
    {
        char cmd[] = "/mrvl/init.sh";

        rt_kprintf("Marvell WiFi exec init.sh...\n");
        msh_exec(cmd, sizeof(cmd));
    }
}

int rt_application_init()
{
    rt_thread_t tid;

    tid = rt_thread_create("init",
        rt_init_thread_entry, RT_NULL,
        2048, RT_THREAD_PRIORITY_MAX/3, 20);

    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}

/*@}*/
