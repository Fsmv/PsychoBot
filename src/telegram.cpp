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
 
#include <map>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include "logger.h"
static Logger logger("tg api");

#include "config.h"
#include "json.hpp"
using json = nlohmann::json;

static std::string callMethod(const std::string &method,
        const std::map<std::string, std::string> &arguments,
        const std::map<std::string, std::string> &files = {}) {

    using namespace curlpp::options;
    std::stringstream result;

    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;

        request.setOpt<Url>(Config::global()->get<std::string>("api_url")
                            + "bot" + Config::global()->get<std::string>("token")
                            + "/" + method);

        if (arguments.size() != 0 || files.size() != 0) {
            curlpp::Forms formParts;

            for (auto pair : arguments) {
                formParts.push_back(
                    new curlpp::FormParts::Content(pair.first, pair.second));
            }

            for (auto pair : files) {
                formParts.push_back(
                    new curlpp::FormParts::File(pair.first, pair.second));
            }

            request.setOpt<HttpPost>(formParts);
        }

        request.setOpt<WriteStream>(&result);
        request.perform();
    }

    catch(curlpp::RuntimeError &e) {
        logger.error("Failed to call method: " + std::string(e.what()));
    }
    catch(curlpp::LogicError &e) {
        logger.error("Failed to call method: " + std::string(e.what()));
    }

    return result.str();
}

bool tg_downloadFile(const std::string &file_id, const std::string &directory) {
    // Get the file path to download
    json response = json::parse(callMethod("getFile", {{"file_id", file_id}}));
    auto path_it = response.find("file_path");
    if (path_it == response.end()) {
        logger.error("Telegram did not provide a download url");
        return false;
    }
    std::string path = *path_it;

    // get the file stream to write to
    std::ofstream file(directory + file_id);
    if (!file.good()) {
        logger.error("Could not write downloaded file. ID: " + file_id);
        return false;
    }

    // download the file
    using namespace curlpp::options;
    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;

        request.setOpt<Url>(Config::global()->get<std::string>("api_url")
                            + "file/bot"
                            + Config::global()->get<std::string>("token")
                            + "/" + path);

        request.setOpt<WriteStream>(&file);
        request.perform();
    }

    catch(curlpp::RuntimeError &e) {
        logger.error("Failed to download file: " + std::string(e.what()));
        return false;
    }
    catch(curlpp::LogicError &e) {
        logger.error("Failed to download file: " + std::string(e.what()));
        return false;
    }

    return true;
}

bool setWebhook(const std::string &url, std::string certFile) {
    json data;
    if (certFile == "") {
        data = json::parse(callMethod("setWebhook", {{"url", url}}));
    } else {
        std::string s = callMethod("setWebhook", {{"url", url}},
                           {{"certificate", certFile}});
        std::cout << s << std::endl;
        data = json::parse(s);
    }

    bool result = data["ok"].get<bool>();
    if (result) {
        logger.debug(data["description"].get<std::string>());
    } else {
        logger.error(data["description"].get<std::string>());
    }

    return result;
}

void tg_sendMessage(const std::string &message, int chat_id,
                    int message_id, bool markdown,
                    bool disable_link_preview) {
    std::map<std::string, std::string> arguments = 
        {{"text", message},
         {"chat_id", std::to_string(chat_id)}};

    if (message_id != -1) {
        arguments["reply_to_message_id"] = std::to_string(message_id);
    }

    if (markdown) {
        arguments["parse_mode"] = "Markdown";
    }

    if (disable_link_preview) {
        arguments["disable_web_page_preview"] = "true";
    }

    callMethod("sendMessage", arguments);
}

std::string getMessageText(const json &update) {
    if (update.find("message") != update.end() &&
        update["message"].find("text") != update["message"].end()) {

        return update["message"]["text"].get<std::string>();
    }

    return "";
}
