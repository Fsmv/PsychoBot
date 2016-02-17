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

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <cassert>

static Logger logger("LuaAPI");

static inline void pushVal(lua_State *L, json::const_iterator &elm) {
    switch (elm->type()) {
        case json::value_t::null:
        case json::value_t::discarded:
        default:
            lua_pushnil(L);
            break;
        case json::value_t::object:
        case json::value_t::array:
            logger.error("Called internal function pushVal with invalid argument");
            assert(0);
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
}

static void pushJsonTable(lua_State *L, const json &j) {
    lua_newtable(L);
    int i = 0;
    luaL_checkstack(L, j.size(), "Could not fit table on stack");
    for (auto elm = j.begin(); elm != j.end(); ++elm) {
        lua_pushstring(L, elm.key().c_str());
        if (elm->type() == json::value_t::array ||
            elm->type() == json::value_t::object) {

            pushJsonTable(L, *elm);
        } else {
            pushVal(L, elm);
        }
        lua_settable(L, -3);
    }
}

static json::value_type readValue(lua_State *L, int stackIdx) {
    switch(lua_type(L, stackIdx)) {
        case LUA_TBOOLEAN:
            return static_cast<bool>(lua_toboolean(L, stackIdx));
        case LUA_TNUMBER:
            return lua_tonumber(L, stackIdx);
        case LUA_TSTRING:
            return std::string(lua_tostring(L, stackIdx));
        default:
            luaL_error(L, "Invalid type"); //long jumps, acts as return
            return 0; // not reachable
    }
}

static json readLuaTable(lua_State *L, int stackIdx) {
    json result;
    luaL_checkstack(L, 3, "Could not read table, out of stack space");

    //stack: - 
    lua_pushvalue(L, stackIdx); // copy the table to the top of the stack
    // stack: table
    lua_pushnil(L); // push start key for lua_next
    // stack: nil, table
    while(lua_next(L, -2) != 0) {  // pop key. if next exists, push next key then value
        // stack: value, key, table
        std::string key = std::string(lua_tostring(L, -2)); // read the key
        if (lua_istable(L, -1)) { // is value another table?
            result[key] = readLuaTable(L, -1);
        } else {
            result[key] = readValue(L, -1);
        }

        // stack: value, key, table
        lua_pop(L, 1);
        // stack: key, table
    }
    // stack: table
    lua_pop(L, 1);
    // stack: -

    return result;
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
    std::string message = std::string(luaL_checkstring(L, 1));
    const PluginRunState *currentRun = getRunState(L);
    tg_send(message,
            currentRun->update["message"]["chat"]["id"].get<int>());
    return 0;
}

static int l_reply(lua_State *L) {
    std::string message = std::string(luaL_checkstring(L, 1));
    const PluginRunState *currentRun = getRunState(L);
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
    const PluginRunState *currentRun = getRunState(L);
    std::string confopt = std::string(luaL_checkstring(L, 1));

    if (currentRun->plugin->config == nullptr) {
        logger.warn("Missing config for plugin: " +
                    currentRun->plugin->getName());
        lua_pushnil(L);
        return 1;
    }

    const json &conf = currentRun->plugin->config->config;
    auto elm = conf.find(confopt);
    if (elm != conf.end()) {
        pushVal(L, elm);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int l_setConfig(lua_State *L) {
    if (lua_isuserdata(L, 2) || lua_isthread(L, 2) || lua_isfunction(L, 2)) {
        luaL_argerror(L, 2, "Cannot set config to thread function, or userdata");
        return 0;
    }
    std::string confopt = std::string(luaL_checkstring(L, 1));
    const PluginRunState *currentRun = getRunState(L);
    json &conf = currentRun->plugin->config->config;

    if (lua_istable(L, 2)) {
        conf[confopt] = readLuaTable(L, 2);
    } else {
        conf[confopt] = readValue(L, 2);
    }

    currentRun->plugin->config->setChanged();
    return 0;
}

#define LUA_INJECT(func) \
    lua_pushcfunction(L, l_##func); \
    lua_setglobal(L, #func)

void injectAPIFunctions(lua_State *L) {
    LUA_INJECT(send);
    LUA_INJECT(reply);
    LUA_INJECT(getSender);
    LUA_INJECT(getConfig);
    LUA_INJECT(setConfig);
}
