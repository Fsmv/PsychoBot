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

#include <string>
#include <iostream>
#include <thread>

#include "webhooks.h"
#include "telegram.h"
#include "logger.h"
#include "config.h"
#include "plugins.h"

static const std::string CONFIG_FILE = "config.json";
static bool running;
static bool output;

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

static void runPlugins() {
    while (running) {
        waitForUpdate();
        
        std::queue<json> updates = popAllUpdates();
        while (!updates.empty()) {
            auto update = updates.front();
            updates.pop();
            //TODO call the right plugin based on the update
        }
    }
}

int main(int argc, char **argv) {
    if(!Config::loadConfig(CONFIG_FILE)) {
        return 1;
    }
    
    loadPlugins();
    running = true;
    std::thread pluginsThread(runPlugins);
    
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
    pluginsThread.join();
    unloadPlugins();
    
    return 0;
}