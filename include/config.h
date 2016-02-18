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
#include <memory>
#include "json.hpp"

class Config : public nlohmann::json {
public:
    static Config *loadConfig(const std::string &filename);
    virtual ~Config() { if (changed) { save(); } }

    inline bool contains(const std::string &option) const {
        return find(option) != end();
    }

    template<typename T>
    T get(const std::string &option) const {
        return (*this)[option].get<T>();
    }

    template<typename T>
    T get(const std::string &option, const T &default_value) const {
        if (contains(option)) {
            return (*this)[option].get<T>();
        } else {
            return default_value;
        }
    }

    inline void set(const std::string &option, nlohmann::json::const_reference value) {
        (*this)[option] = value;
    }

    void save();
    void setChanged() { changed = true; }

    static void loadGlobalConfig();
    static const Config *global() { return m_global.get(); }


    static const std::string PB_VERSION;

private:
    Config(const nlohmann::json &config, const std::string &filename);
    Config(const std::string &filename);

    static std::unique_ptr<Config> m_global;

    std::string filename;
    bool changed;
};

#endif
