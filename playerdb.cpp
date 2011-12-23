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
  virtual void Event ( bz_EventData * eventData );

  // bz_BaseURLHandler
  bool webBusy;
  std::string webData;
  typedef struct {
    std::string callsign;
    std::string bzid;
    std::string ipaddress;
    std::string build;
  } pdbJoinRecord;

  // This will contain joins that need to be sent off
  std::queue<pdbJoinRecord> joinQueue;

  void nextUpdate();
  
  virtual void URLDone ( const char* URL, void * data, unsigned int size, bool complete );

  // bz_CustomSlashCommandHandler
  virtual bool SlashCommand ( int /*playerID*/, bz_ApiString /*command*/, bz_ApiString /*message*/, bz_APIStringList* /*param*/ ){return false;};
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
  }
  else {
    // TODO: Print error and/or unload plugin
  }
}

void playerdb::Event ( bz_EventData * eventData )
{
  if (eventData->eventType != bz_ePlayerJoinEvent)
    return;

  bz_PlayerJoinPartEventData_V1 *joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

  // Generate a join record
  pdbJoinRecord jr;
  jr.callsign = joinData->record->callsign.c_str();
  jr.bzid = joinData->record->bzID.c_str();
  jr.ipaddress = joinData->record->ipAddress.c_str();
  jr.build = joinData->record->clientVersion.c_str();

  // Add the join to the queue
  joinQueue.push(jr);

  //bz_debugMessagef(0, "PLAYERDB-JOIN: Queued join for '%s'", jr.callsign.c_str());

  webBusy = true;
  nextUpdate();

}

void playerdb::nextUpdate() {

  if (!joinQueue.empty()) {
    // Clear the data
    webData = "";

    // Get the oldest item from the queue
    pdbJoinRecord jr = joinQueue.front();
    
    // Generate our POST data string
    std::string postData = "action=join";
    postData += bz_format("&apikey=%s", bz_urlEncode(APIKey.c_str()));
    postData += bz_format("&callsign=%s", bz_urlEncode(jr.callsign.c_str()));
    postData += bz_format("&bzid=%s", bz_urlEncode(jr.bzid.c_str()));
    postData += bz_format("&ipaddress=%s", bz_urlEncode(jr.ipaddress.c_str()));
    postData += bz_format("&build=%s", bz_urlEncode(jr.build.c_str()));

    // Start the HTTP job
    bz_addURLJob(URL.c_str(), this, postData.c_str());

    //bz_debugMessagef(0, "PLAYERDB-REQUEST: Sent join for '%s'", jr.callsign.c_str());

    // Remove the item from the queue
    joinQueue.pop();
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

    std::vector<std::string> lines = tokenize(webData, std::string("\n"), 0, false);

    std::vector<std::string>::const_iterator itr = lines.begin();
    for (itr = lines.begin(); itr != lines.end(); ++itr) {
      bz_sendTextMessagef(BZ_SERVER, eAdministrators, "Return Data: %s", itr->c_str());
    }
    
    //bz_debugMessagef(0, "PLAYERDB-RESULT: %s", webData.c_str());

    if (!joinQueue.empty()) {
      nextUpdate();
    }
    else {
      webBusy = false;
    }
  }
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

