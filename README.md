Building
========

Install the dependecies:
 `sudo apt-get install cmake check lubmicrohttpd-dev lua5.2-dev lubcurlpp-dev lubcurl4-openssl-dev`
 
Then to build:
 `mkdir build && cd build && cmake .. && make`
 
Configuration
=============

Create config.json in the same directory as the executable and add the following fields:

```
{
    "token"       : "YOUR TOKEN",
    "api_url"     : "https://api.telegram.org/bot",
    "webhook_url" : "YOUR SERVER URL",
    "log_level"   : "INFO",
    
    "plugins" : [ "plugin1.lua", "plugin2.lua" ]
}
```

Plugins
=======

Example:

```
-- API v0.1 Functions:
-- 
--    send(string message)
--    reply(string message)


function run(messageText, matchStr)
    if (matchStr == "foo") then
        reply("You called foo!");
    elseif (matchStr == "bar") then
        send("You called bar!"); 
    end
end

function getInfo()
    return {
        version = "0.1",
        commandOnly = true,
        description = "This is a test description",
        usage = { foo = "does foo things",
                  bar = "does bar things"},
        commands = { "foo", "bar" },
        matches = { } -- regular expression matches; must turn off commandOnly
    };
end
```
