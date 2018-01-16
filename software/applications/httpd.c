// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "mongoose.h"

static const char *s_http_port = "80";
static struct mg_serve_http_opts s_http_server_opts;

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
    switch (ev) {
    case MG_EV_HTTP_REQUEST: {
        struct http_message *hm = (struct http_message *) p;
        
        printf("%p: %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p, 
                                            (int) hm->uri.len, hm->uri.p);
        mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
        break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        char addr[32];

        mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr), 
            MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
        printf("websocket: %s connected\n", addr);
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message *wm = (struct websocket_message *)p;
        
        printf("websocket: %.*s\n", wm->size, wm->data);
        break;
    }
    }
}

void mg_httpd_thread_entry(void *args) {
    struct mg_mgr mgr;
    struct mg_connection *nc;

    mongoose_port_init();
    mg_mgr_init(&mgr, NULL);
    printf("Starting web server on port %s\n", s_http_port);
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    if (nc == NULL) {
        printf("Failed to create listener\n");
        return;
    }

    // Set up HTTP server parameters
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = "/web";  // Serve current directory
    s_http_server_opts.enable_directory_listing = "yes";

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
}

#include <rtthread.h>
int mg_httpd(void)
{
    rt_thread_t tid;
    
    tid = rt_thread_create("mg_httpd", 
        mg_httpd_thread_entry, RT_NULL, 4096, 23, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
    
    return 0;
}
INIT_APP_EXPORT(mg_httpd);
