// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include <rtthread.h>
#include "mongoose.h"

#define HTTPD_ROOT_PATH     "/web"
#define HTTPD_PORT_STR      "80"

static struct mg_serve_http_opts httpd_opts;

#include "dc_motor.h"

rt_dc_motor_t motor_l, motor_r;
rt_timer_t timer = RT_NULL;

static void timeout_handler(void *parameter)
{
    rt_kprintf("brake!\n");
    dc_motor_brake(motor_l);
    dc_motor_brake(motor_r);
}

static void control_handler(void *data, int size)
{
#define CAR_FORWARD "U"
#define CAR_REVERSE "D"
#define CAR_TURN_LF "L"
#define CAR_TURN_RT "R"

    if (!strncmp(data, CAR_FORWARD, size))
    {
        dc_motor_forward(motor_l, 75);
        dc_motor_reverse(motor_r, 75);
        rt_timer_start(timer);
    }
    else if (!strncmp(data, CAR_REVERSE, size))
    {
        dc_motor_reverse(motor_l, 75);
        dc_motor_forward(motor_r, 75);
        rt_timer_start(timer);
    }
    else if (!strncmp(data, CAR_TURN_LF, size))
    {
        dc_motor_reverse(motor_l, 60);
        dc_motor_reverse(motor_r, 60);
        rt_timer_start(timer);
    }
    else if (!strncmp(data, CAR_TURN_RT, size))
    {
        dc_motor_forward(motor_l, 60);
        dc_motor_forward(motor_r, 60);
        rt_timer_start(timer);
    }
}

static void ev_handler(struct mg_connection *nc, int ev, void *p)
{
    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *) p;

        printf("%p: %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p,
               (int) hm->uri.len, hm->uri.p);
        mg_serve_http(nc, (struct http_message *) p, httpd_opts);
//        nc->flags |= MG_F_SEND_AND_CLOSE;
        break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
    {
        char addr[32];

        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                            MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("websocket: %s connected\n", addr);
        break;
    }
    case MG_EV_WEBSOCKET_FRAME:
    {
        struct websocket_message *wm = (struct websocket_message *)p;

        printf("websocket: %.*s\n", wm->size, wm->data);
        control_handler(wm->data, wm->size);
        break;
    }
    case MG_EV_CLOSE:
    {
        printf("websocket: closed\n");
        break;
    }
    }
}

void mg_httpd_thread_entry(void *args)
{
    struct mg_mgr mgr;
    struct mg_connection *nc;

    mongoose_port_init();
    mg_mgr_init(&mgr, NULL);
    nc = mg_bind(&mgr, HTTPD_PORT_STR, ev_handler);
    if (nc == NULL)
    {
        printf("Failed to create listener on port:%s\n", HTTPD_PORT_STR);
        return;
    }

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);
    httpd_opts.document_root = HTTPD_ROOT_PATH;
    httpd_opts.enable_directory_listing = "yes";

    for (;;)
    {
        mg_mgr_poll(&mgr, 200);
    }
//    mg_mgr_free(&mgr);
}

int mg_httpd_init(void)
{
    {
        motor_l = (rt_dc_motor_t)rt_device_find("dcmotor1");
        motor_r = (rt_dc_motor_t)rt_device_find("dcmotor2");
        rt_device_open((rt_device_t)motor_l, RT_DEVICE_OFLAG_RDWR);
        rt_device_open((rt_device_t)motor_r, RT_DEVICE_OFLAG_RDWR);
    }

    {
        const rt_tick_t timeout = rt_tick_from_millisecond(100);

        timer = rt_timer_create("ctrl_tim", timeout_handler,
                                RT_NULL, 0, RT_TIMER_FLAG_ONE_SHOT);
        rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, (void *)&timeout);
    }

    {
        rt_thread_t tid;

        tid = rt_thread_create("mg_httpd",
                               mg_httpd_thread_entry, RT_NULL, 4096, 23, 30);
        if (tid != RT_NULL)
            rt_thread_startup(tid);
    }

    return 0;
}
INIT_APP_EXPORT(mg_httpd_init);
