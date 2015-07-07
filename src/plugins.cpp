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

#include "plugins.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include "config.h"
#include "logger.h"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

static Logger logger("Plugins");
static const std::string pluginsDir = "plugins/";

struct Plugin {
    lua_State *L;
    std::map<std::string, std::string> commands;
    std::vector<std::string> matches;
    std::string description;
    bool commandOnly;
};

static std::unordered_map<std::string, struct Plugin> plugins;

bool getfield(lua_State *L, const char *key, const char **value) {
    lua_getfield(L, -1, key);
    if (lua_isnil(L, -1)) {
        logger.error("Missing key " + std::string(key));
        return false;
    }
    if (!lua_isstring(L, -1)) {
        logger.error("Invalid type for key " + std::string(key) + " expected string");
        return false;
    }
    *value = lua_tostring(L, -1);
    lua_pop(L, 1);
    return true;
}

bool getfield(lua_State *L, const char *key, bool *value) {
    lua_getfield(L, -1, key);
    if (lua_isnil(L, -1)) {
        logger.error("Missing key " + std::string(key));
        return false;
    }
    if (!lua_isboolean(L, -1)) {
        logger.error("Invalid type for key " + std::string(key) + " expected boolean");
        return false;
    }
    *value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return true;
}

bool getfield_array(lua_State *L, const char *key, std::vector<std::string> *arr) {
    lua_getfield(L, -1, key);
    if (!lua_istable(L, -1)) {
        logger.error("Invalid type for key " + std::string(key) + " expected array");
        return false;
    }
    
    size_t n = lua_rawlen(L, -1);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, -1, i);
        if (!lua_isstring(L, -1)) {
            logger.error("Invalid type in string array index=" + std::to_string(i));
            return false;
        }
        const char *v;
        v = lua_tostring(L, -1);
        lua_pop(L, 1);
        arr->push_back(v);
    }
    
    lua_pop(L, 1);
    return true;
}

bool getusages(lua_State *L, const std::vector<std::string> &commands,
               std::map<std::string, std::string> *commandUsages) {
    // We can hardcode since this method is specific to usages
    lua_getfield(L, -1, "usage");
    if (!lua_istable(L, -1)) {
        logger.error("Invalid type for key usage expected array");
        return false;
    }
    
    for (std::string command : commands) {
        const char *usage;
        if(!getfield(L, command.c_str(), &usage)) {
            logger.error("in usages");
            return false;
        }
        (*commandUsages)[command] = std::string(usage);
    }
    
    lua_pop(L, 1);
    return true;
}

void loadPlugins() {
    if (!Config::contains("plugins")) {
        logger.info("No plugins specified in config");
        return;
    }
    
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    
    std::vector<std::string> pluginsToLoad = Config::get<std::vector<std::string>>("plugins");
    for (auto plugin : pluginsToLoad) {
        std::vector<std::string> commands;
        struct Plugin p;
        
        if (luaL_loadfile(L, (pluginsDir + plugin).c_str())) { //load file
            logger.error(lua_tostring(L, -1));
            goto LOAD_ERROR;
        } 
        
        if (lua_pcall(L, 0, 0, 0)) { //primer call
            logger.error(lua_tostring(L, -1));
            goto LOAD_ERROR;
        }
        
        lua_getglobal(L, "getInfo"); //find getInfo
        if (!lua_isfunction(L, -1)) {
            logger.error("getInfo not defined");
            goto LOAD_ERROR;
        }
        
        if (lua_pcall(L, 0, 1, 0)) { //call getInfo
            logger.error("Could not call getInfo");
            logger.error(lua_tostring(L, -1));
            goto LOAD_ERROR;
        }
        
        if (!lua_istable(L, -1)) {
            logger.error("getInfo did not return a table");
            goto LOAD_ERROR;
        }
        
        p.L = L;

        const char *v;
        if (!getfield(L, "version", &v)) {
            goto LOAD_ERROR;
        }
        if (Config::VERSION != std::string(v)) {
            logger.error("Plugin " + plugin + " expects version "
                + std::string(v) + " bot version: " + Config::VERSION);
        }
        
        if (!getfield(L, "version", &v)) {
            goto LOAD_ERROR;
        }
        p.description = std::string(v);
        
        if (!getfield(L, "commandOnly", &p.commandOnly)) {
            goto LOAD_ERROR;
        }
        
        if (!getfield_array(L, "matches", &p.matches)) {
            logger.error("Failed to read matches");
            goto LOAD_ERROR;
        }
        
        if (!getfield_array(L, "commands", &commands)) {
            logger.error("Failed to read commands");
            goto LOAD_ERROR;
        }
        
        if (!getusages(L, commands, &p.commands)) {
            goto LOAD_ERROR;
        }
        
        lua_getglobal(L, "run"); //find getInfo
        if (!lua_isfunction(L, -1)) {
            logger.error("run not defined");
            goto LOAD_ERROR;
        }
        
        logger.info("Loaded plugin " + plugin);
        plugins[plugin] = p;
        continue;

LOAD_ERROR:
        logger.error("Failed to load plugin: " + plugin);
        lua_close(L);
        continue;
    }
}

void unloadPlugins() {
    for (auto plugin : plugins) {
        lua_close(plugin.second.L);
    }
}