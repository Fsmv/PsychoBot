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

#include "config.h"
#include "logger.h"
#include <fstream>
#include <iterator>
#include <array>
#include <string>

static Logger logger("config");
const std::string Config::PB_VERSION = "0.1.2";
const std::array<std::string, 3> Config::REQ_CONF_OPTS = { "token", "api_url", "webhook_url" };
const Config global("config.json");

Config::Config(const std::string &filename) {
    Config *loaded = loadConfig(filename);
    if (!loaded) {
        throw new std::runtime_error("Could not load global config file");
    }

    this->config = loaded->config;
    this->filename = loaded->filename;

    if (jConfig.find("log_file") != Config::config.end()) {
        Logger::setLogFile(Config::config["log_file"].get<std::string>().c_str());
    } else {
        Logger::setLogFile("output.log");
    }

    if (Config::config.find("log_level") != Config::config.end()) {
        Logger::setGlobalLogLevel(
            Logger::parseLogLevel(
                Config::config["log_level"].get<std::string>().c_str()));
    }

    for (auto option : REQ_CONF_OPTS) {
        if (Config::config.find(option) == Config::config.end()) {
            logger.error("Missing required config option: " + option);
            return nullptr;
        }
    }
}

Config Config::loadConfig(const std::string &filename) {
    std::ifstream f(filename);
    json jConfig;

    if(!f.good()) {
        logger.error("Could not open config file: " + filename);
        return nullptr;
    }

    std::istream_iterator<char> it(f), eof;
    std::string config_json(it, eof);

    try {
        jConfig = json::parse(config_json);
    } catch (std::invalid_argument &e) {
        logger.error("Syntax error in config file: " + std::string(e.what()));
        return nullptr;
    }

    return new Config(jConfig, filename);
}

bool Config::contains(const std::string &option) {
    return config.find(option) != Config::config.end();
}
