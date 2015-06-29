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

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "json.hpp"
using json = nlohmann::json;

#include <string>
#include <iostream>
#include <fstream>
#include <iterator>

#include "webhooks.h"

static const std::string CONFIG_FILE = "config.json";
static json config;

bool loadConfig() {
    std::ifstream f(CONFIG_FILE);
    if(!f.good()) {
        return false;
    }
    
    std::istream_iterator<char> it(f), eof;
    std::string config_json(it, eof);
    config = json::parse(config_json);
    
    if (config.find("token") == config.end() ||
        config.find("api_url") == config.end() ||
        config.find("webhook_url") == config.end()) {
        return false;        
    }
    
    return true;
}

void luaHello() {
    lua_State *L = luaL_newstate();
    
    luaL_openlibs(L);
    luaL_dostring(L, "print('hello, '.._VERSION)");
    
    lua_close(L);
}

void telegramHello() {
    using namespace curlpp::options;
    
    try {
        curlpp::Cleanup myCleanup;
        curlpp::Easy myRequest;
        myRequest.setOpt<Url>(config["api_url"].get<std::string>() + config["token"].get<std::string>() + "/setWebhook");
        curlpp::Forms formParts;
        formParts.push_back(new curlpp::FormParts::Content("url", config["webhook_url"].get<std::string>()));
        myRequest.setOpt<HttpPost>(formParts);
        myRequest.perform();
    }
    
    catch(curlpp::RuntimeError &e) {
        std::cout << e.what() << std::endl;
    }
    
    catch(curlpp::LogicError &e) {
        std::cout  << e.what() << std::endl;
    }
}

int main(int argc, char **argv) {
    if(!loadConfig()) {
        std::cerr << "Could not load config" << std::endl;
        return 1;
    }
    
    //luaHello();
    telegramHello();
    
    startServer(atoi(getenv("PORT")), getenv("IP"));
    std::cout << "\nPress Enter to stop" << std::endl;
    std::cin.ignore();
    stopServer();
    
    return 0;
}