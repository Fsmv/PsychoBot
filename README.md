Dependencies
============

 `sudo apt-get install cmake check lubmicrohttpd-dev lua5.2-dev lubcurlpp-dev lubcurl4-openssl-dev`
 
Configuration
=============

Create config.json in the same directory as the executable and add the following fields:

```
{
    "token"       : "YOUR TOKEN",
    "api_url"     : "https://api.telegram.org/bot",
    "webhook_url" : "YOUR SERVER URL"
}
```