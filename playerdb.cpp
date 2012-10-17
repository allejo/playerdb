/* playerdb - BZFlag plugin for storing/querying player data
 * Copyright (C) 2012  Scott Wichser
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "bzfsAPI.h"
#include "plugin_utils.h"

#include <string>
#include <vector>
#include <queue>

class playerdb : public bz_Plugin, bz_BaseURLHandler, bz_CustomSlashCommandHandler
{
  std::string URL;
  std::string APIKey;
  bool LookupEnabled;
  std::string LookupPermission;

  // bz_Plugin
  virtual const char* Name (){return "Player DB";}
  virtual void Init ( const char* config);
  virtual void Cleanup ();
  virtual void Event ( bz_EventData * eventData );

  // bz_BaseURLHandler
  bool webBusy;
  std::string webData;

  typedef struct {
    std::string action;
    std::string query;
    int playerID;
  } pdbQueryRecord;


  // This will contain requests that need to be sent off
  std::queue<pdbQueryRecord> queryQueue;

  // This will contain the item that is currently being processed
  pdbQueryRecord currentQuery;

  void nextQuery();
  
  virtual void URLDone ( const char* URL, void * data, unsigned int size, bool complete );
  virtual void URLTimeout ( const char* URL, int errorCode );
  virtual void URLError ( const char* URL, int errorCode, const char * errorString );

  // bz_CustomSlashCommandHandler
  virtual bool SlashCommand ( int /*playerID*/, bz_ApiString /*command*/, bz_ApiString /*message*/, bz_APIStringList* /*param*/ );

};

class joinhandler : public bz_BaseURLHandler
{
  virtual void URLDone ( const char* /*URL*/, void * /*data*/, unsigned int /*size*/, bool /*complete*/ ) {};
};

joinhandler * joinHandler;

BZ_PLUGIN(playerdb)

void playerdb::Init ( const char* configFile )
{
  bz_debugMessage(4,"playerdb plugin loaded");

  if (strlen(configFile)) {
    PluginConfig config = PluginConfig(configFile);

    if (!config.errors) {
      URL =    config.item("API", "URL");
      APIKey = config.item("API", "Key");

      std::string LookupEnabledString = config.item("Lookup", "Enabled");
      LookupEnabled = (LookupEnabledString == "yes" || LookupEnabledString == "true" || LookupEnabledString == "1");

      LookupPermission = config.item("Lookup", "Permission");
    }

    Register(bz_ePlayerJoinEvent);
    bz_registerCustomSlashCommand ("lookup",this);
    bz_registerCustomSlashCommand ("ipmap",this);
  }
  else {
    // TODO: Print error and/or unload plugin
    bz_debugMessage(0, "Player DB: Missing configuration file.");
  }

  joinHandler = new joinhandler;

  MaxWaitTime = 0.1f;
}

void playerdb::Cleanup ( void )
{
  bz_removeCustomSlashCommand ("lookup");
  bz_removeCustomSlashCommand ("ipmap");
  Flush();
}

void playerdb::Event ( bz_EventData * eventData )
{
  if (eventData->eventType != bz_ePlayerJoinEvent)
    return;

  bz_PlayerJoinPartEventData_V1 *joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

  // Assemble the POST data
  std::string postData = "action=join";
  postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
  postData += bz_format("&callsign=%s", bz_urlEncode(joinData->record->callsign.c_str()));
  postData += bz_format("&bzid=%s", bz_urlEncode(joinData->record->bzID.c_str()));
  postData += bz_format("&ipaddress=%s", bz_urlEncode(joinData->record->ipAddress.c_str()));
  postData += bz_format("&build=%s", bz_urlEncode(joinData->record->clientVersion.c_str()));

  // Start the HTTP job
  bz_addURLJob(URL.c_str(), joinHandler, postData.c_str());
}

void playerdb::nextQuery() {

  if (!queryQueue.empty() && !webBusy) {
    // Mark it as busy
    webBusy = true;

    // Clear the data
    webData = "";

    // This is where the POST data will be stored
    std::string postData = "action=query";

    // Get the oldest item from the queue
    currentQuery = queryQueue.front();
    
    postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
    postData += bz_format("&query=%s", bz_urlEncode(currentQuery.query.c_str()));

    // Start the HTTP job
    bz_addURLJob(URL.c_str(), this, postData.c_str());

    // Remove the item from the queue
    queryQueue.pop();
  }

}

void playerdb::URLDone( const char* /*URL*/, void * data, unsigned int size, bool complete )
{
  if (!webBusy)
    return;

  if (data && size > 0)
  {
    char *p = (char*)malloc(size+1);
    memcpy(p,data,size);
    p[size] = 0;
    webData += p;
    free(p);
  }

  if (complete) {
    webBusy = false;

    std::vector<std::string> lines = tokenize(webData, std::string("\n"), 0, false);

    std::vector<std::string>::const_iterator itr = lines.begin();
    for (itr = lines.begin(); itr != lines.end(); ++itr) {
      if (itr->length() > 7 && itr->substr(0, 6) == "ERROR:") {
        bz_sendTextMessage(BZ_SERVER, eAdministrators, itr->c_str());
      }
      else {
        bz_sendTextMessagef(BZ_SERVER, currentQuery.playerID, itr->c_str());
      }
    }
    
    nextQuery();
  }

}

void playerdb::URLTimeout( const char* /*URL*/, int /* errorCode*/ )
{
  bz_sendTextMessage(BZ_SERVER, currentQuery.playerID, "A timeout occured during the lookup.");
  nextQuery();
}

void playerdb::URLError( const char* /*URL*/, int /*errorCode*/, const char * /*errorString*/ )
{
  bz_sendTextMessage(BZ_SERVER, currentQuery.playerID, "An error occured during the lookup.");
  nextQuery();
}

bool playerdb::SlashCommand ( int playerID, bz_ApiString cmd, bz_ApiString message, bz_APIStringList* )
{

  if (strcasecmp (cmd.c_str(), "lookup") != 0 && strcasecmp (cmd.c_str(), "ipmap") != 0) {
    return false;
  }

  if (!LookupEnabled) {
    bz_sendTextMessage (BZ_SERVER, playerID, "The lookup function is not enabled.");
    return true;
  }

  if (! bz_hasPerm(playerID,LookupPermission.c_str())) {
    bz_sendTextMessage (BZ_SERVER, playerID, "You do not have permission to run the lookup command");
    return true;
  }

  if (message.c_str()[0] == '\0') {
    bz_sendTextMessage (BZ_SERVER, playerID, "Usage : /lookup <callsign>|<ipaddress>");
    return true;
  }

  pdbQueryRecord jr;
  jr.query = message.c_str();
  jr.playerID = playerID;

  // Add the query to the queue
  queryQueue.push(jr);

  nextQuery();

  return true;
}

  

  

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

