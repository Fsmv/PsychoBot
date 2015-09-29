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

#ifndef _TELEGRAM_H_
#define _TELEGRAM_H_

#include <string>
#include "json.hpp"
using json = nlohmann::json;


/**
 * Set the webhook url
 * 
 * @param url address to set, if "" turn off the webhook
 * @return true if success, false otherwise
 */
bool setWebhook(std::string url, std::string certFile = "");

void tg_send(std::string message, int chat_id);
void tg_reply(std::string message, int chat_id, int message_id);

/**
 * Returns the message text from a given Update object (in the telegram api)
 * 
 * @param update the Update object from telegram to parse
 * @return The update text. Returns "" if there was no text.
 */
std::string getMessageText(const json &update);

#endif
