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

#include <string>
#include <iostream>

#include "webhooks.h"
#include "conf.h"

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
        myRequest.setOpt<Url>(TG_URL + TG_TOKEN + "/setWebhook");
        curlpp::Forms formParts;
        formParts.push_back(new curlpp::FormParts::Content("url", TG_WEBHOOK_URL));
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
    //luaHello();
    telegramHello();
    
    startServer(atoi(getenv("PORT")), getenv("IP"));
    std::cout << "\nPress Enter to stop" << std::endl;
    std::cin.ignore();
    stopServer();
    
    return 0;
}