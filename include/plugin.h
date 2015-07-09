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

struct lua_State;

class Plugin {
public:
    /**
     * Loads a plugin from a lua source file
     * 
     * @param filename the full path to the lua source file
     * @throws invalid_argument if there is any error in loading the plugin
     */
    Plugin(const std::string &filename);
    
    /**
     * Conditionally call the plugin's lua run method if the message matches any
     * of the plugin's commands or regular expressions
     * 
     * @param update update to check
     */
    void run(const json &update);
    
    std::string getDescription() { return description; }
    std::string getName() { return name; }

private:
    std::unique_ptr<lua_State, decltype(&lua_close)>luaState;
    std::map<std::string, std::string> commands;
    std::map<std::string, std::regex> matches;
    std::string description;
    std::string name;
    bool commandOnly;
};

struct PluginRunState {
    Plugin *plugin;
    json update;
    bool regex;
    std::pair<std::string, std::regex> match;
};

bool loadPlugins(std::vector<Plugin> *plugins);

void onUpdate(json message);

#endif