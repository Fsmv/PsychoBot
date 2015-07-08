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

#ifndef _LUAAPI_H_
#define _LUAAPI_H_

#include <utility>
#include <string>
#include <regex>
#include "json.hpp"
using json = nlohmann::json;

// Declaration so we can use the pointer
struct lua_State;
struct Plugin;

struct runningPlugin {
    Plugin *plugin;
    json update;
    bool regex;
    std::pair<const std::string, std::regex> *match;
};

/**
 * Adds the lua api functions to the globals of the provided lua_State
 * 
 * @param state to add the api functions to
 */
void injectAPIFunctions(lua_State *L);

#endif