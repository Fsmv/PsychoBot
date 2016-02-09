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
#include "plugin.h"

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
    std::vector<Plugin> plugins;
    if (loadPlugins(&plugins)) {
        while (running) {
            waitForUpdate();

            std::queue<json> updates = popAllUpdates();
            while (!updates.empty()) {
                auto update = updates.front();
                updates.pop();

                static int lastUpdateID = 0;
                if (update.find("update_id") != update.end()) {
                    int update_id = update["update_id"].get<int>();
                    if (lastUpdateID >= update_id) {
                        continue; // reject a message we've already seen
                    }
                    lastUpdateID = update_id;
                }

                std::for_each(plugins.begin(), plugins.end(),
                [&update](Plugin &p) {
                    p.run(update);
                });
            }
        }
    }
}

int main(int argc, char **argv) {
    running = true;
    std::thread pluginsThread(runPlugins);

    if (!setWebhook(Config::global.get<std::string>("webhook_url"), 
               Config::global.get<std::string>("webhook_self_signed_cert_file", ""))) {
        return 1;
    }

    if(startServer(Config::global.get<int>("port", 80), Config::global.get<std::string>("bind_address", "0.0.0.0").c_str())) {  
    //if(startServer(atoi(getenv("PORT")), getenv("IP"))) {
        return 1;
    }

    output = false;
    while(running) {
        repl();
    }

    stopServer();
    pluginsThread.join();

    return 0;
}
