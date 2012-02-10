========================================================================
    DYNAMIC LINK LIBRARY : playerdb Project Overview
========================================================================

The playerdb plugin interfaces with a web API to store and query player
join information. It is useful for identifying player aliases. Also,
most of the actual logic is handled in the web API, so the plugin should
not need modifications for future improvements to take effect.

=========================
 License
=========================

The playerdb plugin is licensed under the GNU LGPL 2.1 (or any later
version). Please see the provided COPYING file for the full license.

The playerdb website is licensed under the GNU AGPL 3 (or any later
version).  Please see the COPYING file in the bz_playerdb_website repo
linked to at the end of this readme.

=========================
 Project History
=========================

Version 1.0 - February 10, 2012
* Initial versioned release

=========================
 How it works
=========================

The plugin tracks when a player joins the server. It grabs the IP
address, callsign, and BZID (if the user is identified) and stores this
data in a queue. Also, once bzfs populates the client build version,
this will be stored as well.  A web request is then initiated to send
this data to a central web API.  This website will store the data in a
database.

The plugin also exposes a /lookup command that can send queries to the
web API.  The plugin does not do any special processing on the value
passed to it, nor do it does any special processing on the resulting
data it receives from the web API.  This has been done with the hope
that no changes to the plugin will be necessary to add additional types
of queries, or to adjust the resulting output.  The plugin simply sends
all of the input it receives, and displays all of the resulting output.

=========================
 Configuration
=========================

The plugin requires a simple configuration file. A sample configuration
file is included as playerdb.example.conf.  An API key is necessary to
use the database hosted by blast.  You are, however, free to run your
own instance of the website.

The configuration file also allows you to enable or disable the lookup
functionality. With the lookup functionality disabled, the plugin would
still send player join data to the web API, but it would not allow the
use of the /lookup command.

The file also allows you to configure which permission allows for the
use of the /lookup command. You may wish to use a custom permission, or
you can use a built-in permission such as PLAYERLIST.

=========================
 Usage
=========================

Make note where your configuration file is stored. It can be shared
across servers. The path to the configuration file is passed when
loading the plugin, as follows:
  -loadplugin playerdb,/path/to/playerdb.conf

Once loaded, the plugin with send data to the API when players join. In
order to run a lookup, use the /lookup command as follows:
  /lookup SomePlayerName
  /lookup Some Player Name
  /lookup "Some Player Name"
  /lookup 127.0.0.1

These are the methods that work at the time of writing this
documentation.  As noted previously, however, additional queries can be
added without modifying the plugin.  The logic is mostly in the web API.

Double quotes around multi-word queries are optional, and the queries
are case insensitive.

=========================
 Source Code Repository
=========================

Plugin source can be found here:
https://bitbucket.org/blast007/bz_playerdb_plugin

If you want to host your own copy of the website/database, you can get
the code from here:
https://bitbucket.org/blast007/bz_playerdb_website

