/* PlayerDB - BZFlag plugin for storing/querying player data
 * Copyright (C) 2012 Scott Wichser
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

#include <memory>
#include <queue>

class PlayerDB : public bz_Plugin, public bz_BaseURLHandler, public bz_CustomSlashCommandHandler
{
public:
    virtual const char* Name () { return "Player DB 1.1.1"; }
    virtual void Init ( const char* config);
    virtual void Cleanup ();
    virtual void Event ( bz_EventData * eventData );

    virtual bool SlashCommand (int playerID, bz_ApiString, bz_ApiString, bz_APIStringList*);

    virtual void URLDone ( const char* URL, const void * data, unsigned int size, bool complete );
    virtual void URLTimeout ( const char* URL, int errorCode );
    virtual void URLError ( const char* URL, int errorCode, const char * errorString );

    virtual void nextQuery();

    bool webBusy;

    struct WebQuery
    {
        std::string
            query,
            bzID;

        int playerID;

        void sendMessage (std::string message)
        {
            std::unique_ptr<bz_BasePlayerRecord> pr(bz_getPlayerByIndex(playerID));

            if (pr->verified && pr->bzID == bzID)
            {
                bz_sendTextMessage(BZ_SERVER, playerID, message.c_str());
            }
        }
    };

    std::string URL;
    std::string APIKey;
    std::string LookupPermission;

    std::queue<WebQuery> queryQueue;

    WebQuery currentQuery;
};

BZ_PLUGIN(PlayerDB)

void PlayerDB::Init ( const char* configFile )
{
    std::string conf = configFile;

    if (!conf.empty())
    {
        PluginConfig config = PluginConfig(conf.c_str());

        if (!config.errors)
        {
            URL    = config.item("API", "URL");
            APIKey = config.item("API", "Key");
            LookupPermission = config.item("Lookup", "Permission");

            bz_debugMessage(2, "DEBUG :: Player DB :: Configuration file loaded successfully");
            bz_debugMessagef(3, "DEBUG :: Player DB :: API Endpoint: %s", URL.c_str());
            bz_debugMessagef(3, "DEBUG :: Player DB :: API Key: %s", APIKey.c_str());
            bz_debugMessagef(3, "DEBUG :: Player DB :: Lookup Perm: %s", LookupPermission.c_str());
        }
        else
        {
            bz_debugMessage(0, "ERROR :: Player DB :: Configuration file contains error(s)");
        }
    }
    else
    {
        // TODO: Print error and/or unload plugin
        bz_debugMessage(0, "ERROR :: Player DB :: Missing configuration file.");
    }

    Register(bz_ePlayerJoinEvent);

    bz_registerCustomSlashCommand("lookup", this);

    MaxWaitTime = 0.1f;
    webBusy = false;
}

void PlayerDB::Cleanup ( void )
{
    Flush();

    bz_removeCustomSlashCommand("lookup");
}

void PlayerDB::Event ( bz_EventData * eventData )
{
    switch (eventData->eventType)
    {
        case bz_ePlayerJoinEvent:
        {
            bz_PlayerJoinPartEventData_V1 *joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

            std::string postData = "action=join";
            postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
            postData += bz_format("&callsign=%s", bz_urlEncode(joinData->record->callsign.c_str()));
            postData += bz_format("&bzid=%s", bz_urlEncode(joinData->record->bzID.c_str()));
            postData += bz_format("&ipaddress=%s", bz_urlEncode(joinData->record->ipAddress.c_str()));
            postData += bz_format("&build=%s", bz_urlEncode(joinData->record->clientVersion.c_str()));

            bz_addURLJob(URL.c_str(), NULL, postData.c_str());
        }
        break;

        default:
            break;
    }
}

void PlayerDB::nextQuery()
{
    if (!queryQueue.empty() && !webBusy)
    {
        // Mark it as busy
        webBusy = true;

        // Get the oldest item from the queue
        currentQuery = queryQueue.front();

        // Start the HTTP job
        bz_addURLJob(URL.c_str(), this, currentQuery.query.c_str());

        // Remove the item from the queue
        queryQueue.pop();
    }

}

void PlayerDB::URLDone( const char* /*URL*/, const void * data, unsigned int size, bool complete )
{
    std::string webData = (const char*)data;

    if (complete)
    {
        webBusy = false;

        std::vector<std::string> lines = tokenize(webData, "\n", 0, false);

        for (auto line : lines)
        {
            if (line.substr(0, 6) == "ERROR:")
                bz_sendTextMessage(BZ_SERVER, eAdministrators, line.c_str());
            else
                currentQuery.sendMessage(line);
        }

        nextQuery();
    }
}

void PlayerDB::URLTimeout( const char* /*URL*/, int /* errorCode*/ )
{
    currentQuery.sendMessage("A timeout occured during the lookup.");
    webBusy = false;
    nextQuery();
}

void PlayerDB::URLError( const char* /*URL*/, int /*errorCode*/, const char * /*errorString*/ )
{
    currentQuery.sendMessage("An error occured during the lookup.");
    webBusy = false;
    nextQuery();
}

bool PlayerDB::SlashCommand ( int playerID, bz_ApiString cmd, bz_ApiString message, bz_APIStringList* params)
{
    if (cmd == "lookup")
    {
        if (!bz_hasPerm(playerID, LookupPermission.c_str()))
        {
            bz_sendTextMessage(BZ_SERVER, playerID, "You do not have permission to run the lookup command");
            return true;
        }

        if (params->size() == 0)
        {
            bz_sendTextMessage (BZ_SERVER, playerID, "Usage : /lookup <callsign>|<ipaddress>");
            return true;
        }

        std::string postData = "action=query";
        postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
        postData += bz_format("&query=%s", bz_urlEncode(message.c_str()));

        WebQuery jr;
        jr.query = postData;
        jr.playerID = playerID;
        jr.bzID = bz_getPlayerByIndex(playerID)->bzID;

        // Add the query to the queue
        queryQueue.push(jr);

        nextQuery();

        return true;
    }

    return false;
}
