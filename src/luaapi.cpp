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
#include "config.h"
static Logger logger("LuaAPI");

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}


void pushJsonTable(lua_State *L, const json &j) {
    lua_newtable(L);
    int i = 0;
    //TODO: stack overflow if we get a giant json object
    for (auto elm = j.begin(); elm != j.end(); ++elm) {
        lua_pushstring(L, elm.key().c_str());
        
        switch (elm->type()) {
            case json::value_t::null:
                lua_pushnil(L);
                break;
            case json::value_t::object:
            case json::value_t::array:
                pushJsonTable(L, *elm);
                break;
            case json::value_t::string:
                lua_pushstring(L, elm->get<std::string>().c_str());
                break;
            case json::value_t::boolean:
                lua_pushboolean(L, elm->get<bool>());
                break;
            case json::value_t::number_integer:
            case json::value_t::number_float:
                lua_pushnumber(L, elm->get<float>());
                break;
        }
        
        lua_settable(L, -3);
    }
}

static const PluginRunState *getRunState(lua_State *L) {
    lua_getglobal(L, "TG_RUN_STATE");
    if(lua_islightuserdata(L, -1)) {
        const void *ptr = lua_topointer(L, -1);
        lua_pop(L, 1);
        return reinterpret_cast<const PluginRunState *>(ptr);
    }
    
    return nullptr;
}

static int l_send(lua_State *L) {
    if (!lua_isstring(L, -1)) {
        logger.error("Invalid argument to send");
        return 0;
    }
    
    const PluginRunState *currentRun = getRunState(L);
    
    std::string message = std::string(lua_tostring(L, -1));
    tg_send(message,
            currentRun->update["message"]["chat"]["id"].get<int>());
    return 0;
}

static int l_reply(lua_State *L) {
    if (!lua_isstring(L, -1)) {
        logger.error("Invalid argument to reply");
        return 0;
    }
    const PluginRunState *currentRun = getRunState(L);
    
    std::string message = std::string(lua_tostring(L, -1));
    tg_reply(message,
             currentRun->update["message"]["chat"]["id"].get<int>(),
             currentRun->update["message"]["message_id"].get<int>());
    return 0;
}

static int l_getSender(lua_State *L) {
    const PluginRunState *currentRun = getRunState(L);
    pushJsonTable(L, currentRun->update["message"]["from"]);
    return 1;
}

static int l_getConfig(lua_State *L) {
    if (!lua_isstring(L, -1)) {
        logger.error("Invalid argument to getConfig");
        lua_pushnil(L);
        return 1;
    }
    std::string confopt = std::string(lua_tostring(L, -1));
    if (Config::contains(confopt)) {
        logger.debug(Config::get<std::string>(confopt).c_str());
        lua_pushstring(L, Config::get<std::string>(confopt).c_str());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

void injectAPIFunctions(lua_State *L) {
    lua_pushcfunction(L, l_send);
    lua_setglobal(L, "send");
    lua_pushcfunction(L, l_reply);
    lua_setglobal(L, "reply");
    lua_pushcfunction(L, l_getSender);
    lua_setglobal(L, "getSender");
    lua_pushcfunction(L, l_getConfig);
    lua_setglobal(L, "getConfig");
}
