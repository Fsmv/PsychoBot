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

#ifndef _WEBHOOKS_H_
#define _WEBHOOKS_H_

#include <cstdint>
#include "json.hpp"
using json = nlohmann::json;

/**
 * Start the webhooks server and bind to the specified port and ip address
 * 
 * @param port port to bind to
 * @param ip IPv4 address string
 * @return 0 on success 1 on error
 */
int startServer(uint16_t port, const char *ip);

/**
 * Stop the webhooks server 
 */
void stopServer();

/**
 * Gets the top event from the queue without removing it
 * @return the json update data
 */
json peekUpdate();

/**
 * Gets the top event from the queue and removes it
 * @return the json update data
 */
json popUpdate();

/**
 * @return the number of updates in the queue 
 */
size_t getNumUpdates();

#endif