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

#include "plugin.h"

#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <regex>
#include "config.h"
#include "logger.h"
#include "telegram.h"
#include "luaapi.h"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

static Logger logger("Plugins");
static const std::string pluginsDir = "plugins/";

static bool getfield(lua_State *L, const char *key, const char **value) {
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

static bool getfield(lua_State *L, const char *key, bool *value) {
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

static bool getfield_array(lua_State *L, const char *key, std::vector<std::string> *arr) {
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

static bool getusages(lua_State *L, const std::vector<std::string> &commands,
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

Plugin::Plugin(const std::string &filename) : luaState(luaL_newstate(), lua_close), name(filename) {
    lua_State *L = luaState.get();
    luaL_openlibs(L);
    
    std::vector<std::string> commandStrings, matchStrings;
    
    if (luaL_loadfile(L, (pluginsDir + filename).c_str())) { //load file
        throw std::invalid_argument(lua_tostring(L, -1));
    } 
    
    if (lua_pcall(L, 0, 0, 0)) { //primer call
        throw std::invalid_argument(lua_tostring(L, -1));
    }
    
    lua_getglobal(L, "getInfo"); //find getInfo
    if (!lua_isfunction(L, -1)) {
        throw std::invalid_argument("getInfo not defined");
    }
    
    if (lua_pcall(L, 0, 1, 0)) { //call getInfo
        throw std::invalid_argument("Could not call getInfo"
            + std::string(lua_tostring(L, -1)));
    }
    
    if (!lua_istable(L, -1)) {
        throw std::invalid_argument("getInfo did not return a table");
    }

    const char *v;
    if (!getfield(L, "version", &v)) {
        throw std::invalid_argument("Did not define version");
    }
    if (Config::VERSION != std::string(v)) {
        throw std::invalid_argument("Expected version "
            + std::string(v) + " bot version: " + Config::VERSION);
    }
    
    if (!getfield(L, "description", &v)) {
        throw std::invalid_argument("Did not define description");
    }
    description = std::string(v);
    
    if (!getfield(L, "commandOnly", &commandOnly)) {
        throw std::invalid_argument("Did not define commandOnly");
    }
    
    if (!getfield_array(L, "matches", &matchStrings)) {
        throw std::invalid_argument("Failed to read matches field");
    }
    for (auto match : matchStrings) { // compiled the regexes
        try {
            auto reg = std::regex(match);
            matches[match] = reg;
        } catch (std::regex_error &e) {
            logger.warn("Failed to load regex " + match + " for plugin " + filename);
            logger.warn(e.what());
        }
    }
    
    if (!getfield_array(L, "commands", &commandStrings)) {
        throw std::invalid_argument("Failed to read commands field");
    }
    
    if (!getusages(L, commandStrings, &commands)) {
        throw std::invalid_argument("Failed to read usage strings");
    }
    
    injectAPIFunctions(L);
    
    lua_getglobal(L, "run"); //find getInfo
    if (!lua_isfunction(L, -1)) {
        throw std::invalid_argument("Run function not defined");
    }
    
    logger.info("Loaded plugin " + filename);
}

static void callRun(lua_State *L, const std::string &message, const std::string &match, PluginRunState *state) {
    lua_pushstring(L, message.c_str());
    lua_pushstring(L, match.c_str());
    lua_pushlightuserdata(L, state);
    if(lua_pcall(L, 3, 0, 0)) {
        logger.error("Error in run function of plugin " + state->plugin->getName());
        logger.error(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_getglobal(L, "run");
}

void Plugin::run(const json &update) {
    PluginRunState currentRun;
    std::string message = getMessageText(update);
    if (message == "") {
        return;
    }
    
    // check if we called a command this plugin uses
    for (auto command : commands) {
        if (message.find("/" + command.first) == 0) {
            currentRun.plugin = this;
            currentRun.update = update;
            currentRun.regex = false;
            callRun(luaState.get(), message, command.first, &currentRun);
        }
    }
    
    // check if we called a regex match this plugin uses
    if (!commandOnly) {
        for (auto match : matches) {
            if (std::regex_search(message, match.second)) {
                currentRun.plugin = this;
                currentRun.update = update;
                currentRun.regex = true;
                currentRun.match = match;
                callRun(luaState.get(), message, match.first, &currentRun);
            }
        }
    }
}

bool loadPlugins(std::vector<Plugin> *plugins) {
    if (!Config::contains("plugins")) {
        logger.info("No plugins specified in config");
        return false;
    }
    
    std::vector<std::string> pluginsToLoad = Config::get<std::vector<std::string>>("plugins");
    for (auto plugin : pluginsToLoad) {
        try {
            plugins->emplace_back(plugin);
        } catch (std::invalid_argument &e) {
            logger.error("Failed to load plugin " + plugin);
            logger.error(e.what());
        }
    }
    
    return true;
}