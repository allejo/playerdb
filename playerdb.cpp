// playerdb.cpp : Defines the entry point for the DLL application.
//

#include "bzfsAPI.h"
#include "plugin_utils.h"

#include <string>
#include <vector>
#include <queue>

class playerdb : public bz_Plugin, bz_BaseURLHandler, bz_CustomSlashCommandHandler
{
  std::string URL;
  std::string APIKey;

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

    // Query action
    std::string query;
    int playerID;

    // Join action
    std::string callsign;
    std::string bzid;
    std::string ipaddress;
    std::string build;
  } pdbQueueRecord;


  // This will contain requests that need to be sent off
  std::queue<pdbQueueRecord> webQueue;

  // This will contain the item that is currently being processed
  pdbQueueRecord currentItem;

  void doWeb();
  
  virtual void URLDone ( const char* URL, void * data, unsigned int size, bool complete );

  // bz_CustomSlashCommandHandler
  virtual bool SlashCommand ( int /*playerID*/, bz_ApiString /*command*/, bz_ApiString /*message*/, bz_APIStringList* /*param*/ );
};

BZ_PLUGIN(playerdb)

void playerdb::Init ( const char* configFile )
{
  bz_debugMessage(4,"playerdb plugin loaded");

  if (strlen(configFile)) {
    PluginConfig config = PluginConfig(configFile);

    if (!config.errors) {
      URL =    config.item("API", "URL");
      APIKey = config.item("API", "Key");
    }

    Register(bz_ePlayerJoinEvent);
    bz_registerCustomSlashCommand ("lookup",this);
    bz_registerCustomSlashCommand ("ipmap",this);
  }
  else {
    // TODO: Print error and/or unload plugin
    bz_debugMessage(0, "Player DB: Missing configuration file.");
  }
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

  // Generate a join record
  pdbQueueRecord jr;
  jr.action = "join";
  jr.callsign = joinData->record->callsign.c_str();
  jr.bzid = joinData->record->bzID.c_str();
  jr.ipaddress = joinData->record->ipAddress.c_str();
  jr.build = joinData->record->clientVersion.c_str();

  // Add the join to the queue
  webQueue.push(jr);

  doWeb();

}

void playerdb::doWeb() {

  if (!webQueue.empty() && !webBusy) {
    // Mark it as busy
    webBusy = true;

    // Clear the data
    webData = "";

    // This is where the POST data will be stored
    std::string postData = "";

    // Get the oldest item from the queue
    currentItem = webQueue.front();
    
    if (currentItem.action == "join") {
      // Generate our POST data string

      postData = "action=join";
      postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
      postData += bz_format("&callsign=%s", bz_urlEncode(currentItem.callsign.c_str()));
      postData += bz_format("&bzid=%s", bz_urlEncode(currentItem.bzid.c_str()));
      postData += bz_format("&ipaddress=%s", bz_urlEncode(currentItem.ipaddress.c_str()));
      postData += bz_format("&build=%s", bz_urlEncode(currentItem.build.c_str()));
    }
    else if (currentItem.action == "query") {
      postData = "action=query";
      postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
      postData += bz_format("&query=%s", bz_urlEncode(currentItem.query.c_str()));
    }

    // Start the HTTP job
    bz_addURLJob(URL.c_str(), this, postData.c_str());

    // Remove the item from the queue
    webQueue.pop();
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

    if (currentItem.action == "query") {
      std::vector<std::string>::const_iterator itr = lines.begin();
      for (itr = lines.begin(); itr != lines.end(); ++itr) {
        if (itr->length() > 7 && itr->substr(0, 6) == "ERROR:") {
          bz_sendTextMessage(BZ_SERVER, eAdministrators, itr->c_str());
        }
        else {
          bz_sendTextMessagef(BZ_SERVER, currentItem.playerID, itr->c_str());
        }
      }
    }
    
    doWeb();
  }
}

bool playerdb::SlashCommand ( int playerID, bz_ApiString cmd, bz_ApiString message, bz_APIStringList* )
{

  if (strcasecmp (cmd.c_str(), "lookup") != 0 && strcasecmp (cmd.c_str(), "ipmap") != 0) {
        return false;
  }

  if (! bz_hasPerm(playerID,"PLAYERLIST")) {
    bz_sendTextMessage (BZ_SERVER, playerID, "You do not have permission to run the lookup command");
    return true;
  }

  if (message.c_str()[0] == '\0') {
    bz_sendTextMessage (BZ_SERVER, playerID, "Usage : /lookup <callsign>|<ipaddress>");
    return true;
  }

  pdbQueueRecord jr;
  jr.action = "query";
  jr.query = message.c_str();
  jr.playerID = playerID;

  // Add the query to the queue
  webQueue.push(jr);

  doWeb();

  return true;
}

  

  

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

