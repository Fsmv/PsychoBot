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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <array>
#include <string>
#include "json.hpp"
using json = nlohmann::json;

class Config {
public:
    static bool loadConfig(const std::string &filename);
    static bool contains(const std::string &option);
    
    template<typename T>
    static T get(const std::string &option) {
        return Config::config[option].get<T>();
    }

    template<typename T>
    static T get(const std::string &option, const T &default_value) {
        if (contains(option)) {
            return Config::config[option].get<T>();
        } else {
            return default_value;
        }
    }
    
    static const std::array<std::string, 3> REQ_CONF_OPTS;
    static const std::string PB_VERSION;
    
private:
    Config() {}
    static json config;
};

#endif
