#include <TimeLib.h>
#include "QNodeItemController.h"

QNFactory<QNodeItemController> *QNodeItemController::f = nullptr;

const String QNodeItemController::LOCAL_BCAST_TOPIC = ":LOCAL_BROADCAST:";
const String QNodeItemController::GLOBAL_BCAST_TOPIC =  ":GLOBAL_BROADCAST:";

QNFactory<QNodeItemController> *QNodeItemController::getFactory() {
  if (f==nullptr) { f = new QNFactory<QNodeItemController>(); }
  return f;
}

void QNodeItemController::onItemConfig( const JsonObject &message) {
    
    String configStr;
    serializeJson( message, configStr ); 
    /*
     * The configuration json is saved to a string and compared with the "new" one, to prevent re-configuration with the exact same settings if the 
     * exact same message is received again.
     */
    
    if ( configStr != currConfigStr ) {
      currConfigStr = configStr;
      logMessage(QNodeController::LOGLEVEL_DEBUG, getItemTag() + F(" Controller: Processing configuration message: "));      
      if (message.containsKey("desc")) {
        String workdesc = message["desc"].as<String>();
        setDescription(workdesc);
        logMessage(QNodeController::LOGLEVEL_DEBUG, "  Description: " + getDescription());
      }
      if (message.containsKey("statetopic")) {
        String workTopic = message["statetopic"].as<String>();
        logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  State topic: ")) +workTopic);
        stateTopic = workTopic;
        if (message.containsKey("stateformat")) {
          if (message["stateformat"].as<String>().equalsIgnoreCase("raw")) {
            statePubFormat[PUB_STATE] = PUB_TEXT;
          }
        }
      }
      if (message.containsKey("eventtopic")) {
      String workTopic = message["eventtopic"].as<String>();
      logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  Event topic: "))+workTopic);
      eventTopic = workTopic;
      }
      if (message.containsKey("commandtopic")) {
        // clear any existing topics added previously
        for(auto topic : cmdTopics ) { removeTopic(topic); }
        cmdTopics.erase(cmdTopics.begin(), cmdTopics.end());
        if (message["commandtopic"].is<JsonArray>()) {
          logMessage( String(F( "  Command topics: "))+"" );
          for (auto ct : message["commandtopic"].as<JsonArray>()) {
            String newTopic = ct.as<String>();
            logMessage(QNodeController::LOGLEVEL_DEBUG, "      "+newTopic);
            cmdTopics.push_back(newTopic);
            addTopic(newTopic);
          }
        }
        else {
          String workTopic = message["commandtopic"].as<String>();
          logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  Command topic: "))+workTopic);        
          //if ((cmdTopic != workTopic) && (cmdTopic != "")) { removeTopic( cmdTopic ); }
          //cmdTopic = workTopic;
          cmdTopics.push_back(workTopic);        
          addTopic(workTopic);
        }
      }
      logMessage(QNodeController::LOGLEVEL_DEBUG, getItemTag() +F( " Controller: Done with base controller configuration.") );
      if (this->onControllerConfig( message )) {
        logMessage(QNodeController::LOGLEVEL_DEBUG, "Starting Item Controller...");
        this->start(); 
        #ifdef QNODE_DEBUG_VERBOSE
        logMessage(QNodeController::LOGLEVEL_DEBUG, "  Item started!");
        #endif
        if ( message.containsKey("init") ) {
          #ifdef QNODE_DEBUG_VERBOSE
          logMessage(QNodeController::LOGLEVEL_DEBUG, getName() + " - Found init command - sending...");
          #endif
          this->onItemCommand( message["init"].as<JsonObject>() );
        }
        else if (message.containsKey( "init_list" )) {
          #ifdef QNODE_DEBUG_VERBOSE
          logMessage(QNodeController::LOGLEVEL_DEBUG, getName() + " - Found init commands - sending...");
          #endif
          for (auto cfg : message["init_list"].as<JsonArray>()) {
            this->onItemCommand(cfg.as<JsonObject>());
          }
        }
      }
      lastConfig = getOwner()->getTime();
      char dateStr[11];
      String msgStr;
      sprintf( dateStr, "%02d/%02d/%d", month(lastConfig), day(lastConfig), year(lastConfig) );
      unsigned long hours = (lastConfig % 86400L) / 3600;
      String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
      unsigned long minutes = (lastConfig % 3600) / 60;
      String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
      unsigned long seconds = lastConfig % 60;
      String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
      String timeStr = hoursStr + ":" + minuteStr + ":" + secondStr;
      lastConfigStr = String(dateStr)+" "+timeStr;
      logMessage(QNodeController::LOGLEVEL_DEBUG, "Setting last configuation to:  " + lastConfigStr);
   }
   else {
    logMessage(QNodeController::LOGLEVEL_DEBUG, "  Configuration has not changed:  Skipping controller configuration.");
   }
 }

void QNodeItemController::directConfig( String stTopic, String evTopic, String cmTopic ) {
  logMessage(QNodeController::LOGLEVEL_DEBUG, getItemTag() + F(" Controller: Processing MQTT configuration message: "));    
  if (stTopic != "") {
    stateTopic = stTopic;
    logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  State topic: "))+stateTopic);
  }
  if (evTopic != "") {
    eventTopic = evTopic;
    logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  Event topic: "))+eventTopic);
  }
  if (cmTopic != "") {
    // cmdTopic = cmTopic;
    cmdTopics.erase(cmdTopics.begin(), cmdTopics.end());
    cmdTopics.push_back(cmTopic);
    addTopic(cmTopic);
    logMessage( QNodeController::LOGLEVEL_DEBUG, String(F("  Command topic: "))+cmTopic);
  }
  logMessage( QNodeController::LOGLEVEL_DEBUG, getItemTag() + F(" Controller: Done with direct configuration.") );
  lastConfig = getOwner()->getTime();
  start();
}

void QNodeItemController::onItemStateDetail(const String &detailName, const String &detailValue ) {
  if (stateTopic != "") {
     String topic = stateTopic;
     this->publishItem( topic, detailName, detailValue, statePubFormat[PUB_STATE_DETAIL] );
  }
}

void QNodeItemController::onItemStateDetail( const String detailName, const JsonObject &detailMessage ) {
  if (stateTopic != "") {
    String topic = stateTopic;
    topic += "/";
    topic += detailName;
    this->publish( topic, detailMessage, true );    
  }
}  

void QNodeItemController::logItemEvent(const String &eventName, const String &addtlAttributes)  {
   if (eventTopic != "") {
     String timeStr = getOwner()->getFormattedTimestamp();
     String attr = "";
     if (!(addtlAttributes.equals(""))) {
        attr = ",";
        attr += addtlAttributes;
     }
     String msg = String("{ \"event\": \"");
     msg += eventName;
     msg += "\", \"time\":\"";
     msg += timeStr;
     msg += "\"";
     msg += attr;
     msg += "}";
     this->publish( eventTopic, msg, false );
   }
 }

 void QNodeItemController::command( const String &cmd, boolean bypass ) {
   if (!bypass && cmdTopics.size() > 0) {
     this->publish( cmdTopics[0], cmd, false );
   }
   else {
  
     DynamicJsonDocument root(JSON_BUFFER_SIZE);     
     auto error = deserializeJson( root, cmd ); 
     if (!error) {
       QNodeItem::onItemCommand(root.to<JsonObject>());
     }
     else {
        this->onMessage( cmdTopics[0], cmd );
     }
   }
 }

 void QNodeItemController::fillItemProperties( JsonObject &props )
 {
    props["last_config"] = lastConfigStr;
    props["name"] = getName();
    props["id"] = getItemID();
    props["desc"] = getDescription();
    props["update_cycles"] = getCycleCount();
 }
