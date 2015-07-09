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
 
#include "luaapi.h"
#include "telegram.h"
#include "plugin.h"
#include "logger.h"
static Logger logger("LuaAPI");

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

static int l_send(lua_State *L) {
    if (!lua_islightuserdata(L, -2) || !lua_isstring(L, -1)) {
        logger.error("Invalid argument to send");
        return 0;
    }
    const PluginRunState *currentRun = reinterpret_cast<const PluginRunState*>(lua_topointer(L, -2));
    
    std::string message = std::string(lua_tostring(L, -1));
    tg_send(message,
            currentRun->update["message"]["chat"]["id"].get<int>());
    return 0;
}

static int l_reply(lua_State *L) {
    if (!lua_islightuserdata(L, -2) || !lua_isstring(L, -1)) {
        logger.error("Invalid argument to reply");
        return 0;
    }
    const PluginRunState *currentRun = reinterpret_cast<const PluginRunState*>(lua_topointer(L, -2));
    
    std::string message = std::string(lua_tostring(L, -1));
    tg_reply(message,
             currentRun->update["message"]["chat"]["id"].get<int>(),
             currentRun->update["message"]["message_id"].get<int>());
    return 0;
}

void injectAPIFunctions(lua_State *L) {
    lua_pushcfunction(L, l_send);
    lua_setglobal(L, "send");
    lua_pushcfunction(L, l_reply);
    lua_setglobal(L, "reply");
}