Building
========

Install the dependecies:
runtime: `libmicrohttpd, lua, curlpp, curl, sqlite3`
building: `cmake, check, pkgconfig`

Lots of plugins will also require libraries like `lua-socket`

Rasbian/Debian/Ubuntu:
 `sudo apt-get install cmake check libmicrohttpd-dev lua5.2-dev libcurlpp-dev libcurl4-openssl-dev pkg-config`

Arch Linux:
`sudo pacman -S cmake check libmicrohttpd lua curl pkg-config; yaourt -S libcurlpp`

OS/X:
`brew install cmake lua libmicrohttpd curlpp sqlite3 pkg-config`

Then to build:
release:
 `mkdir release && cd release && cmake .. && make`

debug:
`mkdir debug && cd debug && cmake -DCMAKE_BUILD_TYPE=Debug .. && make`

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
