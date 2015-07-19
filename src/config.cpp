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
const std::string Config::VERSION = "0.1.1";
const std::array<std::string, 3> Config::REQ_CONF_OPTS = { "token", "api_url", "webhook_url" };
json Config::config;

bool Config::loadConfig(const std::string &filename) {
    std::ifstream f(filename);
    if(!f.good()) {
        logger.error("Could not open config file: " + filename);
        return false;
    }
    
    std::istream_iterator<char> it(f), eof;
    std::string config_json(it, eof);
    
    try {
        Config::config = json::parse(config_json);
    } catch (std::invalid_argument &e) {
        logger.error("Syntax error in config file: " + std::string(e.what()));
        return false;
    }
    
    if (Config::config.find("log_file") != Config::config.end()) {
        Logger::setLogFile(Config::config["log_file"].get<std::string>().c_str());
    } else {
        Logger::setLogFile("output.log");
    }
    
    for (auto option : REQ_CONF_OPTS) {
        if (Config::config.find(option) == Config::config.end()) {
            logger.error("Missing required config option: " + option);
            return false;
        }
    }
    
    if (Config::config.find("log_level") != Config::config.end()) {
        Logger::setGlobalLogLevel(
            Logger::parseLogLevel(
                Config::config["log_level"].get<std::string>().c_str()));
    }
    
    return true;
}

bool Config::contains(const std::string &option) {
    return Config::config.find(option) != Config::config.end();
}