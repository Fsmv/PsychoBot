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

#include "json.hpp"
using json = nlohmann::json;

const json &getConfigOption(const std::string &option);

static std::string callMethod(std::string method, std::map<std::string, std::string> arguments) {
    using namespace curlpp::options;
    std::stringstream result;
    
    try {
        curlpp::Cleanup cleanup;
        curlpp::Easy request;
        request.setOpt<Url>(getConfigOption("api_url").get<std::string>()
                            + getConfigOption("token").get<std::string>()
                            + method);
        
        if (arguments.size() != 0) {
            curlpp::Forms formParts;
            for (auto pair : arguments) {
                formParts.push_back(
                    new curlpp::FormParts::Content(pair.first, pair.second));
            }
            request.setOpt<HttpPost>(formParts);
        }
        
        request.setOpt<WriteStream>(&result);
        request.perform();
    }
    
    catch(curlpp::RuntimeError &e) {
        std::cerr << "Failed to call method: " << e.what() << std::endl;
    }
    catch(curlpp::LogicError &e) {
        std::cerr << "Failed to call method: " << e.what() << std::endl;
    }
    
    return result.str();
}

bool setWebhook(std::string url) {
    callMethod("setWebhook", {{"url", url}});
}