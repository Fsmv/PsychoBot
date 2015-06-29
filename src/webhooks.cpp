/**
 *  Copyright (C) 2015  Andrew Kallmeyer <fsmv@sapium.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "webhooks.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <iostream>

static struct MHD_Daemon *server;

// 1 MB
#define MAX_MESSAGE_LEN (8*1024*1024)
#define POST_BUFFER_SIZE 65536

#define GET             0
#define POST            1

struct connection_info {
    char *message;
    size_t message_len;
    bool valid;
};

static int send_page (struct MHD_Connection *connection, const char *page) {
    int ret;
    struct MHD_Response *response;
  
    response = MHD_create_response_from_buffer (strlen (page), (void *) page,
				                              MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }

    ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    MHD_destroy_response (response);

    return ret;
}

static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe) {

    struct connection_info *con_info = (struct connection_info*)*con_cls;
    
    if (!con_info) return;
    if (con_info->message) {
        puts(con_info->message);
        delete[] con_info->message;
    }
    
    delete con_info;
    *con_cls = nullptr;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls) {
                          
    if (!*con_cls) { // new request
        if (strcmp(method, "POST") == 0) {
            // create the persistent connection info object
            struct connection_info *con_info = new struct connection_info;
            con_info->message = nullptr;
            con_info->message_len = 0;
            con_info->valid = true;
            
            *con_cls = (void*)con_info;
            return MHD_YES;
        }
        
        return MHD_NO;
    }
    
    if (strcmp(method, "POST") == 0) {
        struct connection_info *con_info = (struct connection_info*)*con_cls;
        
        // if there is some data
        if (*upload_data_size != 0) {
            // process all of the data with the postprocessor
            char *newmsg = new char[con_info->message_len + *upload_data_size + 1];
            
            if (con_info->message) {
                strncpy(newmsg, con_info->message, con_info->message_len);
                delete[] con_info->message;
            }
            
            strncpy(newmsg + con_info->message_len, upload_data, *upload_data_size);
            con_info->message_len += *upload_data_size;
            newmsg[con_info->message_len] = '\0';
            con_info->message = newmsg;
            
            *upload_data_size = 0;
            return MHD_YES;
        } else {
            return send_page(connection, " ");
        }
    }
    
    return MHD_NO;
}

int startServer(uint16_t port, const char *ip)  {
    struct sockaddr_in ipaddr;
    ipaddr.sin_family = AF_INET;
    ipaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &ipaddr.sin_addr);
    
    server = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG,
                              0, NULL, NULL,
                              &answer_to_connection, NULL,
                              MHD_OPTION_SOCK_ADDR, &ipaddr,
                              MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
                              MHD_OPTION_END);
                              
    if (!server) {
        return 1;
    }
    
    return 0;
}

void stopServer() {
    if(server) {
        MHD_stop_daemon(server);
    }
}