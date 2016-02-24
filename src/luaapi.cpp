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
#include <algorithm>

static Logger logger("LuaAPI");

static inline void pushVal(lua_State *L, json::const_reference elm) {
    switch (elm.type()) {
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
            lua_pushstring(L, elm.get<std::string>().c_str());
            break;
        case json::value_t::boolean:
            lua_pushboolean(L, elm.get<bool>());
            break;
        case json::value_t::number_integer:
        case json::value_t::number_float:
            lua_pushnumber(L, elm.get<float>());
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
            pushVal(L, *elm);
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

static void l_sendMessage(lua_State *L, bool reply) {
    std::string message = std::string(luaL_checkstring(L, 1));
    const PluginRunState *currentRun = getRunState(L);

    bool markdown = true, disable_preview = false;
    switch (lua_gettop(L)) { // read optional arguments; switch on number of args
    default: // if more than 3 arguments
    case 3:  // if 3 args
        disable_preview = lua_toboolean(L, 3);
    case 2:  // if 2 args
        markdown = lua_toboolean(L, 2);
    case 1:
    case 0:
        ;
    }

    int reply_message = -1;
    if (reply) {
        reply_message = currentRun->update["message"]["message_id"].get<int>();
    }

    tg_sendMessage(message,
             currentRun->update["message"]["chat"]["id"].get<int>(),
             reply_message, markdown, disable_preview);
}

static int l_send(lua_State *L) {
    l_sendMessage(L, false);
    return 0;
}

static int l_reply(lua_State *L) {
    l_sendMessage(L, true);
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

    const auto &conf = *currentRun->plugin->config.get();
    auto elm = conf.find(confopt);
    if (elm != conf.end()) {
        pushVal(L, *elm);
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
    auto &conf = *currentRun->plugin->config.get();

    if (lua_istable(L, 2)) {
        conf[confopt] = readLuaTable(L, 2);
    } else {
        conf[confopt] = readValue(L, 2);
    }

    currentRun->plugin->config->setChanged();
    return 0;
}

static std::string getMsgFile(const json &message, std::string *file_id) {
    auto contains = [message](const std::string &s) {
        return message.find(s) != message.end();
    };

    if (contains("audio")) {
        if (file_id) {
            *file_id = message["audio"]["file_id"].get<std::string>();
        }
        return "AUDIO";
    } else if (contains("document")) {
        if (file_id) {
            *file_id = message["document"]["file_id"].get<std::string>();
        }
        return "FILE";
    } else if (contains("photo")) {
        if (file_id) {
            auto photo = std::max_element(message["photo"].begin(),
                                          message["photo"].end(),
                [](const json &lhs, const json &rhs) {
                    return lhs["width"].get<int>() * lhs["height"].get<int>() <
                           rhs["width"].get<int>() * rhs["height"].get<int>();
                });

            *file_id = (*photo)["file_id"].get<std::string>();
        }
        return "PHOTO";
    } else if (contains("sticker")) {
        if (file_id) {
            *file_id = message["sticker"]["file_id"].get<std::string>();
        }
        return "STICKER";
    } else if (contains("video")) {
        if (file_id) {
            *file_id = message["video"]["file_id"].get<std::string>();
        }
        return "VIDEO";
    } else if (contains("contact")) {
        return "CONTACT";
    } else if (contains("location")) {
        return "LOCATION";
    } else if (contains("text")) {
        return "TEXT";
    } else {
        return "UNKNOWN";
    }
}

static int l_messageType(lua_State *L) {
    const PluginRunState *currentRun = getRunState(L);
    const auto &msg = currentRun->update["message"];
    lua_pushstring(L, getMsgFile(msg, nullptr).c_str());
    return 1;
}

static int l_downloadFile(lua_State *L) {
    const PluginRunState *currentRun = getRunState(L);
    const auto &msg = currentRun->update["message"];
    std::string file_id = "";
    std::string type = getMsgFile(msg, &file_id);

    bool success = false;
    std::string filename = currentRun->plugin->getPath() + file_id;
    if (file_id != "") {
        success = tg_downloadFile(file_id, filename);
    }

    if (success) {
        lua_pushstring(L, filename.c_str());
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, type.c_str());
    return 2;
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
    LUA_INJECT(messageType);
    LUA_INJECT(downloadFile);
}
