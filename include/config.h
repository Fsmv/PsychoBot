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
    static Config *loadConfig(const std::string &filename);

    bool contains(const std::string &option) const {
        return config.find(option) != config.end();
    }

    template<typename T>
    T get(const std::string &option) const {
        return config[option].get<T>();
    }

    template<typename T>
    T get(const std::string &option, const T &default_value) const {
        if (contains(option)) {
            return config[option].get<T>();
        } else {
            return default_value;
        }
    }

    void set(const std::string &option, const json::value_type &value) {
        config[option] = value;
    }

    json config;

    static const std::string PB_VERSION;
    static const Config global;

private:
    Config(json &config, const std::string &filename)
        : config(config), filename(filename) {}
    Config(const std::string &filename);

    const std::string &filename;
};

#endif
