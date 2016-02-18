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

static const std::array<std::string, 3> REQ_CONF_OPTS = { "token", "api_url", "webhook_url" };
static Logger logger("config");
static const std::string GLOBAL_FILE = "config.json";

const std::string Config::PB_VERSION = "0.1.2";
std::unique_ptr<Config> Config::m_global = nullptr;
using json = nlohmann::json;

/**
 * Initialize the global config file
 */
void Config::loadGlobalConfig() {
    m_global.reset(loadConfig(GLOBAL_FILE));
    if (!m_global) {
        logger.error("Could not load global config file");
        m_global.reset(nullptr);
    }

    Logger::setLogFile(m_global->get<std::string>("log_file", "output.log").c_str());

    auto idx = m_global->find("log_level");
    if (idx != m_global->end()) {
        Logger::setGlobalLogLevel(Logger::parseLogLevel(idx->get<std::string>().c_str()));
    }

    for (auto option : REQ_CONF_OPTS) {
        idx = m_global->find("option");
        if (idx != m_global->end()) {
            logger.error("Global config missing required option: " + option);
            m_global.reset(nullptr);
        }
    }
}

Config::Config(const json &config, const std::string &filename) 
    : json(config), filename(filename), changed(false) {
}

Config *Config::loadConfig(const std::string &filename) {
    std::ifstream f(filename);
    json jConfig;

    if(!f.good()) {
        logger.debug("Could not open config file: " + filename);
        return new Config(jConfig, filename);
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

void Config::save() {
    std::ofstream f(filename);
    if (f.good()) {
        f << std::setw(4) << static_cast<json>(*this);
        changed = false;
    } else {
        logger.error("Could not write config file: " + filename);
    }
}
