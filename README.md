# Player DB

[![GitHub release](https://img.shields.io/github/release/allejo/playerdb.svg?maxAge=2592000)](https://github.com/allejo/playerdb/releases/latest)
![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.9+-blue.svg)
[![License](https://img.shields.io/github/license/allejo/playerdb.svg)](https://github.com/allejo/playerdb/blob/master/LICENSE.md)

This is a fork of [the playerdb plugin](https://bitbucket.org/blast007/bz_playerdb_plugin) which interfaces with a [web API](https://bitbucket.org/blast007/bz_playerdb_website) to store and query player join information. It is useful for identifying player aliases. Also, most of the actual logic is handled in the web API, so the plugin should not need modifications for future improvements to take effect.

## How it works

The plugin tracks when a player joins the server. It grabs the IP address, callsign, and BZID (if the user is identified) and stores this data in a queue. Also, once bzfs populates the client build version, this will be stored as well. A web request is then initiated to send this data to a central web API. This website will store the data in a database.

The plugin also exposes a /lookup command that can send queries to the web API.  The plugin does not do any special processing on the value passed to it, nor do it does any special processing on the resulting data it receives from the web API. This has been done with the hope that no changes to the plugin will be necessary to add additional types of queries, or to adjust the resulting output. The plugin simply sends all of the input it receives, and displays all of the resulting output.

## Usage

### Loading the plug-in

Make note where your configuration file is stored. It can be shared across servers. The path to the configuration file is passed when loading the plugin, as follows:

```
-loadplugin playerdb,/path/to/playerdb.conf
```

### Custom Slash Commands

| Command | Permission | Description |
| ------- | ---------- | ----------- |
| `/lookup <ip|callsign>` | lookup | Look up related callsigns or IPs a player has used in the past |

> **Tip:** These are the methods that work at the time of writing this documentation. As noted previously, however, additional queries can be added without modifying the plugin. The logic is mostly in the web API.  
> **Tip:** Double quotes around multi-word queries are optional, and the queries are case insensitive.

### Configuration File

A sample configuration file is included as playerdb.example.conf.

| Field | Type | Description |
| ----- | ---- | ----------- |
| `Key` | string | The API key used when communicating with the web API |
| `URL` | string | The API endpoint this plug-in will communicate with |
| `Permission` | string | The permission required for the `/lookup` command |

> **Warning:** An API key is necessary to use the database hosted by blast.  You are, however, free to run your own instance of the website.  
> **Warning:** Do **not** use single or double quotes when defining string values in the configuration file.  
> **Tip:** Permissions are case-insensitive.  
> **Tip:** You may use custom permissions such as 'LOOKUP' or 'Bacon' and the plug-in will still behave correctly.

## License

[GNU LGPL 2.1](https://github.com/allejo/playerdb/blob/master/LICENSE.md) (or any later version).
