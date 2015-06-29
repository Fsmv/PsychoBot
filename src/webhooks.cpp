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

#include <iostream>
#include <queue>
#include <mutex>

#include <microhttpd.h>
#include "logger.h"

static Logger logger("Webhooks");
static std::mutex updatesMutex;
static std::queue<json> updates;
static struct MHD_Daemon *server;

json peekUpdate() {
    updatesMutex.lock();
    return updates.front();
    updatesMutex.unlock();
}

json popUpdate() {
    updatesMutex.lock();
    auto front = updates.front();
    updates.pop();
    return front;
    updatesMutex.unlock();
}

size_t getNumUpdates() {
    updatesMutex.lock();
    return updates.size();
    updatesMutex.unlock();
}

#define VERIFY_IP
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

std::string getIP(struct MHD_Connection *connection) {
    char address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, (void *)
        &((struct sockaddr_in *)MHD_get_connection_info(connection,
            MHD_CONNECTION_INFO_CLIENT_ADDRESS)->client_addr)->sin_addr,
        address, INET_ADDRSTRLEN);
    return std::string(address);
}

static void
request_completed (void *cls, struct MHD_Connection *connection,
                   void **con_cls, enum MHD_RequestTerminationCode toe) {

    struct connection_info *con_info = (struct connection_info*)*con_cls;
    
    if (!con_info) return;
    if (con_info->message) {
        updatesMutex.lock();
        try {
            auto update = json::parse(con_info->message);
            updates.push(update);
            logger.debug("Update: " + update.dump());
        } catch (std::invalid_argument &e) {
            logger.error("Invalid data received from " + getIP(connection)
                       + "\nMessage:\n" + std::string(con_info->message));
        }
        updatesMutex.unlock();
        
        delete[] con_info->message;
    }
    
    delete con_info;
    *con_cls = nullptr;
}

// Warning: not real security
bool checkIP(struct MHD_Connection *connection) {
#ifdef VERIFY_IP
    std::string ip = getIP(connection);
    return ip.find("10.240.") == 0;
#else
    return true;
#endif
}

static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls) {
                          
    if (!*con_cls) { // new request
        if (strcmp(method, "POST") == 0) {
            if(!checkIP(connection)) {
                logger.debug("Rejected packet from " + getIP(connection));
                return MHD_NO;
            }
            
            // create the persistent connection info object
            struct connection_info *con_info = new struct connection_info;
            con_info->message = nullptr;
            con_info->message_len = 0;
            con_info->valid = true;
            
            *con_cls = (void*)con_info;
            return MHD_YES;
        }
        
        logger.debug("Rejected non-POST message");
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
        logger.error("Failed to start webhooks server on " + std::string(ip) + ":" + std::to_string(port));
        return 1;
    }
    
    logger.info("Started webhooks server on " + std::string(ip) + ":" + std::to_string(port));
    return 0;
}

void stopServer() {
    if(server) {
        MHD_stop_daemon(server);
        logger.info("Stopped webhooks server");
    }
}