#include <rtthread.h>
#include "lwip/api.h"
#include "lwip/inet.h"
/* TODO: remove these header files */
#include "ov2640.h"
#include "dcmi.h"

/* -------------------------------------------------------------------------- */
// <<< Use Configuration Wizard in Context Menu >>>
//   <o> NETCAM_FIFO_SIZE (in KBytes) <0x01-0x40:1>
#define NETCAM_FIFO_SIZE        40 * 1024
//   <o> NETCAM_DEFAULT_PORT <0x400-0xffff:1>
#define NETCAM_DEFAULT_PORT     8088
//   <o> NETCAM_THREAD_PRIORITY <0x01-0x20:1>
#define NETCAM_THREAD_PRIORITY  24
// <<< end of configuration section >>>

//#define RT_NETCAM_DBG
#ifdef RT_NETCAM_DBG
#define netcam_dbg(fmt, ...)  rt_kprintf(fmt, ##__VA_ARGS__)
#else
#define netcam_dbg(fmt, ...)
#endif

/* -------------------------------------------------------------------------- */
static struct
{
    rt_uint8_t  *fifo_buff;
    rt_uint32_t  fifo_size;
    
    rt_uint8_t  *frame_buff;
    rt_uint32_t  frame_size;
    rt_uint8_t  *frame_head;
    rt_uint8_t  *frame_tail;
}netcam;

static rt_sem_t sem;

/* -------------------------------------------------------------------------- */
static void _camera_open(void)
{
    /* allocate fifo memory */
    netcam.fifo_size = NETCAM_FIFO_SIZE;
    netcam.fifo_buff = rt_malloc(netcam.fifo_size);
    RT_ASSERT(netcam.fifo_buff);
    netcam.frame_head = netcam.fifo_buff;
    netcam.frame_tail = netcam.fifo_buff;
    
    /* (re)config camera and start transfer */
    OV2640_JPEG_Mode();
    OV2640_OutSize_Set(640, 480);
    dcmi_xfer_config(netcam.fifo_buff, netcam.fifo_size);
    dcmi_xfer_start();
}

/* -------------------------------------------------------------------------- */
static void _camera_close(void)
{
    /* stop transfer */
    dcmi_xfer_stop();
    
    /* free fifo memory */
    rt_free(netcam.fifo_buff);
    netcam.fifo_buff = RT_NULL;
    netcam.fifo_size = 0;
}

/* -------------------------------------------------------------------------- */
static void _camera_suspend(void)
{
    /* suspend transfer */
    dcmi_xfer_suspend();
}

/* -------------------------------------------------------------------------- */
static void _camera_resume(void)
{
    /* resume transfer */
    dcmi_xfer_resume();
}

/* -------------------------------------------------------------------------- */
/* weak function in dmci.c */
void dcmi_interrupt_frame_callback(void)
{
    netcam.frame_tail = netcam.fifo_buff + netcam.fifo_size - dcmi_xfer_remaining();
    
    /* Check whether capture a bad frame */
    if (netcam.frame_head[0] == 0xff && netcam.frame_head[1] == 0xd8)
    {
        _camera_suspend();
        rt_sem_release(sem);
    }
    /* Skip the bad frame and point to next frame */
    else
    {
        if (netcam.frame_tail == netcam.fifo_buff + netcam.fifo_size)
            netcam.frame_head = netcam.fifo_buff;
        else
            netcam.frame_head = netcam.frame_tail;
    }
}

/* -------------------------------------------------------------------------- */
static rt_uint8_t* netcam_pull_frame(void)
{
    /* Get a frame in the same dma transmitting procedure */
    if (netcam.frame_head < netcam.frame_tail)
    {
        netcam.frame_size = netcam.frame_tail - netcam.frame_head;
        netcam.frame_buff = rt_malloc(netcam.frame_size);
        if (netcam.frame_buff != RT_NULL)
            memcpy(netcam.frame_buff, netcam.frame_head, netcam.frame_size);
    }
    /* Get a frame in two different dma transmitting procedures */
    else if (netcam.frame_head > netcam.frame_tail)
    {
        netcam.frame_size = netcam.frame_tail - netcam.frame_head + netcam.fifo_size;
        netcam.frame_buff = rt_malloc(netcam.frame_size);
        if (netcam.frame_buff != RT_NULL)
        {
            rt_uint32_t size0, size1;
            
            size0 = netcam.fifo_buff + netcam.fifo_size - netcam.frame_head;
            size1 = netcam.frame_size - size0;
            memcpy(netcam.frame_buff, netcam.frame_head, size0);
            memcpy(netcam.frame_buff + size0, netcam.fifo_buff, size1);
        }
    }
    
    if (netcam.frame_buff == RT_NULL)
        rt_kprintf("no room for netcam.frame_buff\n");
    
    /* Point to the next frame */
    if (netcam.frame_tail == netcam.fifo_buff + netcam.fifo_size)
        netcam.frame_head = netcam.fifo_buff;
    else
        netcam.frame_head = netcam.frame_tail;
    
    /* resume camera */
    _camera_resume();
    
    return netcam.frame_buff;
}

/* -------------------------------------------------------------------------- */
static void netcam_push_frame(void)
{
    rt_free(netcam.frame_buff);
    netcam.frame_buff = RT_NULL;
    netcam.frame_size = 0;
}

/* -------------------------------------------------------------------------- */
static void netcam_thread_entry(void* parameter)
{
    struct netconn *conn;
    struct netconn *newconn;
    err_t err;
    
    /* Create a new connection identifier. */
    conn = netconn_new(NETCONN_TCP);
    
    /* Bind connection to NETCAM_DEFAULT_PORT. */
    netconn_bind(conn, IP_ADDR_ANY, NETCAM_DEFAULT_PORT);
    LWIP_ERROR("netcam: invalid conn", (conn != NULL), return);
    
    /* Tell connection to go into listening mode. */
    netconn_listen(conn);
    
    while (1)
    {
        /* Grab new connection. */
        err = netconn_accept(conn, &newconn);
        
        /* Process the new connection. */
        if (err == ERR_OK) 
        {
            ip_addr_t ipaddr;
            rt_uint16_t port;
            
            /* get remote ip address and port */
            netconn_getaddr(newconn, &ipaddr, &port, 0);
            netcam_dbg("newconn: %s:%d\n", inet_ntoa(ipaddr), port);
            
            /* open the camera */
            _camera_open();
            
            while (rt_sem_take(sem, 1000) == RT_EOK)
            {
                if (netcam_pull_frame() != RT_NULL)
                {
                    err = netconn_write(newconn,
                                        netcam.frame_buff,
                                        netcam.frame_size,
                                        NETCONN_COPY);
                    netcam_push_frame();
                    
                    if (err != ERR_OK) break;
                }
            }
            /* close the camera */
            _camera_close();
            
            /* Close connection and discard connection identifier. */
            netconn_close(newconn);
            netconn_delete(newconn);
            netcam_dbg("disconn: %s:%d\n", inet_ntoa(ipaddr), port);
        }
    }
}

/* -------------------------------------------------------------------------- */
int netcam_init(void)
{
    rt_thread_t tid;
    
    sem = rt_sem_create("frmrx", 0, RT_IPC_FLAG_FIFO);
    tid = rt_thread_create("netcam", netcam_thread_entry, 
        RT_NULL, 512, NETCAM_THREAD_PRIORITY, 10);
    
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    
    return 0;
}
INIT_APP_EXPORT(netcam_init);
