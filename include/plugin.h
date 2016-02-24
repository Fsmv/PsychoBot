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

#ifndef _PLUGINS_H_
#define _PLUGINS_H_

#include <map>
#include <string>
#include <regex>
#include <memory>

extern "C" {
    #include "lua.h"
}

#include "json.hpp"
using json = nlohmann::json;

#include "config.h"

struct lua_State;

class Plugin {
public:
    /**
     * Loads a plugin from a lua source file
     *
     * @param name the name without extension of the lua source file in the
     * plugins directory
     * @throws invalid_argument if there is any error in loading the plugin
     */
    Plugin(const std::string &name);

    /**
     * Conditionally call the plugin's lua run method if the message matches any
     * of the plugin's commands or regular expressions
     * 
     * @param update update to check
     */
    void run(const json &update);

    std::string getPath() const;
    std::string getDescription() { return description; }
    std::string getName() { return name; }

    std::unique_ptr<Config> config;

private:
    std::unique_ptr<lua_State, decltype(&lua_close)> luaState;
    std::map<std::string, std::string> commands;
    std::map<std::string, std::regex> matches;
    std::string description;
    std::string name;
    bool commandOnly;
    bool alwaysTrigger;
};

// State information for a single call of run
struct PluginRunState {
    // The plugin being run
    Plugin *plugin;
    // the update message that trigger the run call
    json update;
    // if the run call was triggered with a regex match
    bool regex;
    // the string and regex that triggered the run if regex is true
    std::pair<std::string, std::regex> match;
};

bool loadPlugins(std::vector<Plugin> *plugins);

void onUpdate(json message);

#endif
