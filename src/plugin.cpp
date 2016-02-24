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
        logger.debug("Missing key " + std::string(key));
        lua_pop(L, 1);
        return false;
    }
    if (!lua_isstring(L, -1)) {
        logger.debug("Invalid type for key " + std::string(key) + " expected string");
        lua_pop(L, 1);
        return false;
    }

    *value = lua_tostring(L, -1);
    lua_pop(L, 1);
    return true;
}

static bool getfield(lua_State *L, const char *key, bool *value) {
    lua_getfield(L, -1, key);
    if (lua_isnil(L, -1)) {
        logger.debug("Missing key " + std::string(key));
        lua_pop(L, 1);
        return false;
    }
    if (!lua_isboolean(L, -1)) {
        logger.debug("Invalid type for key " + std::string(key) + " expected boolean");
        lua_pop(L, 1);
        return false;
    }
    *value = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return true;
}

static bool getfield_array(lua_State *L, const char *key, std::vector<std::string> *arr) {
    lua_getfield(L, -1, key);
    if (!lua_istable(L, -1)) {
        logger.debug("Invalid type for key " + std::string(key) + " expected array");
        return false;
    }

    size_t n = lua_rawlen(L, -1);
    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(L, -1, i);
        if (!lua_isstring(L, -1)) {
            logger.debug("Invalid type in string array index=" + std::to_string(i));
            lua_pop(L, 1);
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
        logger.error("Usage list must be an array");
        return false;
    }

    for (std::string command : commands) {
        const char *usage;
        if(!getfield(L, command.c_str(), &usage)) {
            logger.error("Did not define usage for command: " + command);
            lua_pop(L, 1);
            return false;
        }
        (*commandUsages)[command] = std::string(usage);
    }

    lua_pop(L, 1);
    return true;
}

static bool checkVersion(std::string v) {
    static const std::regex versionCheck("\\d+\\.\\d+\\.\\d+");
    if (!std::regex_match(v, versionCheck)) {
        logger.debug("Version string did not match the correct format");
        return false;
    }

    int bot_maj, bot_min, bot_patch;
    std::stringstream(Config::PB_VERSION) >> bot_maj >> bot_min >> bot_patch;
    int v_maj, v_min, v_patch;
    std::stringstream(Config::PB_VERSION) >> v_maj >> v_min >> v_patch;

    // TODO: better/more useful version checking
    if (bot_maj != v_maj) {
        return bot_maj < v_maj;
    }
    if (bot_min != v_min) {
        return bot_min < v_min;
    }
    if (bot_patch != v_patch) {
        return bot_patch < v_patch;
    }

    return true;
}

Plugin::Plugin(const std::string &name) : config(nullptr), luaState(luaL_newstate(), lua_close), name(name) {
    config.reset(Config::loadConfig(pluginsDir + name + ".json"));
    if (!config) {
        throw std::invalid_argument("malformed config file");
    }

    lua_State *L = luaState.get();
    if (L == nullptr) {
        throw std::invalid_argument("Could not create lua state");
    }

    luaL_openlibs(L); // load the standard lua libraries

    if (luaL_loadfile(L, (pluginsDir + name + ".lua").c_str())) { //load file
        throw std::invalid_argument(lua_tostring(L, -1));
    } 

    if (lua_pcall(L, 0, 0, 0)) { // evaluate the globals in the file
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

    { // check plugin version
        const char *v;
        if (!getfield(L, "version", &v)) {
            throw std::invalid_argument("Did not define version");
        }
        if (!checkVersion(std::string(v))) {
            throw std::invalid_argument("Plugin version "
                + std::string(v) + " not compatible with bot version: " + Config::PB_VERSION);
        }
    }

    { // get plugin description
        const char *v;
        if (!getfield(L, "description", &v)) {
            throw std::invalid_argument("Did not define description");
        }
        description = std::string(v);
    }

    { // load the commandOnly field
        if (!getfield(L, "commandOnly", &this->commandOnly)) {
            commandOnly = false;
        }
    }

    { // load the alwaysTrigger field
        if (!getfield(L, "alwaysTrigger", &this->alwaysTrigger)) {
            alwaysTrigger = false;
        }
    }

    { // load the regular expressions
        std::vector<std::string> matchStrings;
        if (getfield_array(L, "matches", &matchStrings)) {
            for (auto match : matchStrings) { // compile the regexes
                try {
                    auto reg = std::regex(match);
                    matches[match] = reg;
                } catch (std::regex_error &e) {
                    logger.error(e.what());
                    throw std::invalid_argument("Failed to load regex " + match + " for plugin " + name);
                }
            }
        }
    }

    { // load commands and usages
        std::vector<std::string> commandStrings;
        if (getfield_array(L, "commands", &commandStrings)) {
            if (!getusages(L, commandStrings, &commands)) {
                throw std::invalid_argument("Failed to read usage strings");
            }
        }
    }

    // check if plugin has a way to be triggered
    if ((commandOnly && commands.size() == 0) ||
        (commands.size() == 0 && matches.size() == 0)) {
        throw std::invalid_argument("Plugin will never be run");
    }

    injectAPIFunctions(L);

    lua_getglobal(L, "run"); // find the run function
    if (!lua_isfunction(L, -1)) {
        throw std::invalid_argument("Run function not defined");
    }

    logger.info("Loaded plugin " + name);
}

static void callRun(lua_State *L, const std::string &message, const std::string &match, PluginRunState *state) {
    lua_pushlightuserdata(L, state);
    lua_setglobal(L, "TG_RUN_STATE");

    lua_pushstring(L, message.c_str());
    lua_pushstring(L, match.c_str());
    if(lua_pcall(L, 2, 0, 0)) {
        logger.error("Error in run function of plugin " + state->plugin->getName());
        logger.error(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_getglobal(L, "run");
}

void Plugin::run(const json &update) {
    PluginRunState currentRun;
    std::string message = getMessageText(update);

    if (alwaysTrigger) {
        currentRun.plugin = this;
        currentRun.update = update;
        currentRun.regex = false;
        callRun(luaState.get(), message, "ANY", &currentRun);
    }


    if (message == "") {
        return;
    }

    // check if we called a command this plugin uses
    for (auto command : commands) {
        if (message.find("/" + command.first) == 0 &&
                (message.length() == command.first.length() + 1 ||
                 message[command.first.length() + 1] == ' ')) {

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

std::string Plugin::getPath() const {
    return pluginsDir + name + "/";
}

bool loadPlugins(std::vector<Plugin> *plugins) {
    if (!Config::global()->contains("plugins")) {
        logger.info("No plugins specified in config");
        return false;
    }

    std::vector<std::string> pluginsToLoad = Config::global()->get<std::vector<std::string>>("plugins");
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
