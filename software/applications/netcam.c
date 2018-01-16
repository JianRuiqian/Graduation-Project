#include <rtthread.h>
#include <lwip/api.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>
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

#define MJPEG_BOUNDARY "boundarydonotcross"

/* -------------------------------------------------------------------------- */
static struct
{
    struct {
        rt_uint8_t *buff;
        rt_uint32_t size;
    }fifo;
    
    struct {
        rt_uint8_t *buff;
        rt_uint32_t size;
        rt_uint8_t *head;
        rt_uint8_t *tail;
    }frame;
    
    rt_sem_t frame_ready;
}netcam;

static char g_send_buf[1024];

/* -------------------------------------------------------------------------- */
static void _camera_open(void)
{
    /* allocate fifo memory */
    netcam.fifo.size = NETCAM_FIFO_SIZE;
    netcam.fifo.buff = rt_malloc(netcam.fifo.size);
    RT_ASSERT(netcam.fifo.buff);
    netcam.frame.head = netcam.fifo.buff;
    netcam.frame.tail = netcam.fifo.buff;
    
    /* (re)config camera and start transfer */
    OV2640_JPEG_Mode();
    OV2640_OutSize_Set(640, 480);
    dcmi_xfer_config(netcam.fifo.buff, netcam.fifo.size);
    dcmi_xfer_start();
}

/* -------------------------------------------------------------------------- */
static void _camera_close(void)
{
    /* stop transfer */
    dcmi_xfer_stop();
    
    /* free fifo memory */
    rt_free(netcam.fifo.buff);
    netcam.fifo.buff = RT_NULL;
    netcam.fifo.size = 0;
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
    netcam.frame.tail = netcam.fifo.buff + netcam.fifo.size - dcmi_xfer_remaining();
    
    /* Check whether capture a bad frame */
    if (netcam.frame.head[0] == 0xff && netcam.frame.head[1] == 0xd8)
    {
        _camera_suspend();
        rt_sem_release(netcam.frame_ready);
    }
    /* Skip the bad frame and point to next frame */
    else
    {
        if (netcam.frame.tail == netcam.fifo.buff + netcam.fifo.size)
            netcam.frame.head = netcam.fifo.buff;
        else
            netcam.frame.head = netcam.frame.tail;
    }
}

/* -------------------------------------------------------------------------- */
static rt_uint8_t* netcam_pull_frame(void)
{
    /* Get a frame in the same dma transmitting procedure */
    if (netcam.frame.head < netcam.frame.tail)
    {
        netcam.frame.size = netcam.frame.tail - netcam.frame.head;
        netcam.frame.buff = rt_malloc(netcam.frame.size);
        if (netcam.frame.buff != RT_NULL)
            memcpy(netcam.frame.buff, netcam.frame.head, netcam.frame.size);
    }
    /* Get a frame in two different dma transmitting procedures */
    else if (netcam.frame.head > netcam.frame.tail)
    {
        netcam.frame.size = netcam.frame.tail - netcam.frame.head + netcam.fifo.size;
        netcam.frame.buff = rt_malloc(netcam.frame.size);
        if (netcam.frame.buff != RT_NULL)
        {
            rt_uint32_t size0, size1;
            
            size0 = netcam.fifo.buff + netcam.fifo.size - netcam.frame.head;
            size1 = netcam.frame.size - size0;
            memcpy(netcam.frame.buff, netcam.frame.head, size0);
            memcpy(netcam.frame.buff + size0, netcam.fifo.buff, size1);
        }
    }
    
    if (netcam.frame.buff == RT_NULL)
        rt_kprintf("no room for netcam.frame.buff\n");
    
    /* Point to the next frame */
    if (netcam.frame.tail == netcam.fifo.buff + netcam.fifo.size)
        netcam.frame.head = netcam.fifo.buff;
    else
        netcam.frame.head = netcam.frame.tail;
    
    /* resume camera */
    _camera_resume();
    
    return netcam.frame.buff;
}

/* -------------------------------------------------------------------------- */
static void netcam_push_frame(void)
{
    rt_free(netcam.frame.buff);
    netcam.frame.buff = RT_NULL;
    netcam.frame.size = 0;
}

#if 1
/* -------------------------------------------------------------------------- */
static int send_first_response(int client)
{
    g_send_buf[0] = 0;

    rt_snprintf(g_send_buf, 1024,
             "HTTP/1.0 200 OK\r\n"
             "Connection: close\r\n"
             "Server: MJPG-Streamer/0.2\r\n"
             "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0,"
             " post-check=0, max-age=0\r\n"
             "Pragma: no-cache\r\n"
             "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
             "Content-Type: multipart/x-mixed-replace;boundary="
             MJPEG_BOUNDARY "\r\n"
             "\r\n"
             "--" MJPEG_BOUNDARY "\r\n");
    if (send(client, g_send_buf, strlen(g_send_buf), 0) < 0)
    {
        closesocket(client);
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static int mjpeg_send_stream(int client, void *data, int size)
{
    g_send_buf[0] = 0;

    snprintf(g_send_buf, 1024,
             "Content-Type: image/jpeg\r\n"
             "Content-Length: %d\r\n"
             "\r\n", size);
    if (send(client, g_send_buf, strlen(g_send_buf), 0) < 0)
    {
        closesocket(client);
        return -1;
    }

    if (send(client, data, size, 0) < 0)
    {
        closesocket(client);
        return -1;
    }

    g_send_buf[0] = 0;
    snprintf(g_send_buf, 1024, "\r\n--" MJPEG_BOUNDARY "\r\n");
    if (send(client, g_send_buf, strlen(g_send_buf), 0) < 0)
    {
        closesocket(client);
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
static void netcam_thread_entry(void* parameter)
{
    int on;
    int srv_sock = -1;
    struct sockaddr_in addr;
    socklen_t sock_len = sizeof(struct sockaddr_in);

    srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (srv_sock < 0)
    {
        printf("netcam: create server socket failed due to (%s)\n",
              strerror(errno));
        goto exit;
    }

    memset(&addr, 0, sock_len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(NETCAM_DEFAULT_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* ignore "socket already in use" errors */
    on = 1;
    setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(srv_sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    if (bind(srv_sock, (struct sockaddr *)&addr, sock_len) != 0)
    {
        printf("netcam: bind() failed due to (%s)\n",
              strerror(errno));
        goto exit;
    }

    if (listen(srv_sock , RT_LWIP_TCP_PCB_NUM) != 0)
    {
        printf("netcam: listen() failed due to (%s)\n",
              strerror(errno));
        goto exit;
    }

    while (1)
    {
        struct sockaddr_in client_addr;
        int client = accept(srv_sock, (struct sockaddr *)&client_addr, &sock_len);
        if (client < 0)
            continue;
        
        if (recv(client, g_send_buf, sizeof(g_send_buf)-1, 0) &&
            strncmp(g_send_buf, "GET /?action=stream", 19) == 0)
        {
            if (send_first_response(client) < 0)
            {
                client = -1;
                continue;
            }

            printf("netcam: client connected\n");
            /* open the camera */
            _camera_open();

            while (rt_sem_take(netcam.frame_ready, 1000) == RT_EOK)
            {
                int err;
                
                if (netcam_pull_frame() != RT_NULL)
                {
                    err = mjpeg_send_stream(client, (void *)netcam.frame.buff, netcam.frame.size);
                    netcam_push_frame();
                    
                    if (err < 0)
                    {
                        client = -1;
                        break;
                    }
                }
            }

            printf("netcam: client disconnected\n");
            /* close the camera */
            _camera_close();
        }
        else closesocket(client);
    }

    exit:
    if (srv_sock >= 0) closesocket(srv_sock);
}
#else
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
                                        netcam.frame.buff,
                                        netcam.frame.size,
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
#endif

/* -------------------------------------------------------------------------- */
int netcam_init(void)
{
    rt_thread_t tid;
    
    netcam.frame_ready = rt_sem_create("frmrdy", 0, RT_IPC_FLAG_FIFO);
    tid = rt_thread_create("netcam", netcam_thread_entry, 
        RT_NULL, 512, NETCAM_THREAD_PRIORITY, 10);
    
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    
    return 0;
}
INIT_APP_EXPORT(netcam_init);
