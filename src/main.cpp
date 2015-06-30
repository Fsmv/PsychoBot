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

#include <string>
#include <iostream>

#include "webhooks.h"
#include "telegram.h"
#include "logger.h"
#include "config.h"

static const std::string CONFIG_FILE = "config.json";
static bool running;
static bool output;

void luaHello() {
    lua_State *L = luaL_newstate();
    
    luaL_openlibs(L);
    luaL_dostring(L, "print('hello, '.._VERSION)");
    
    lua_close(L);
}

static void repl() {
    std::cout << "$ " << std::flush;
    std::string command = "";
    char c;
    while((c = getchar()) != '\n') {
        command += c;
        
        output = false;
    }
    
    if (command == "quit") {
        running = false;
    } else if (command != "") {
        std::cout << "Invalid command" << std::endl;
    }
}

int main(int argc, char **argv) {
    if(!Config::loadConfig(CONFIG_FILE)) {
        return 1;
    }
    
    running = true;
    
    if (!setWebhook(Config::get<std::string>("webhook_url"))) {
        return 1;
    }
        
    if(startServer(atoi(getenv("PORT")), getenv("IP"))) {
        return 1;
    }

    output = false;
    while(running) {
        repl();
    }
    
    stopServer();
    
    return 0;
}