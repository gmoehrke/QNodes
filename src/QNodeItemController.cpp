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
      logMessage(getItemTag() + F(" Controller: Processing MQTT configuration message: ") + configStr);
      if (message.containsKey("desc")) {
        String workdesc = message["desc"].as<String>();
        setDescription(workdesc);
        logMessage("  Description: " + getDescription());
      }
      if (message.containsKey("statetopic")) {
        String workTopic = message["statetopic"].as<String>();
        logMessage( String(F("  State topic: ")) +workTopic);
        stateTopic = workTopic;
      }
      if (message.containsKey("eventtopic")) {
      String workTopic = message["eventtopic"].as<String>();
      logMessage( String(F("  Event topic: "))+workTopic);
      eventTopic = workTopic;
      }
      if (message.containsKey("commandtopic")) {
        String workTopic = message["commandtopic"].as<String>();
        logMessage( String(F("  Command topic: "))+workTopic);
        if ((cmdTopic != workTopic) && (cmdTopic != "")) { removeTopic( cmdTopic ); }
        cmdTopic = workTopic;
        addTopic(workTopic);
      }
     logMessage( getItemTag() +F( " Controller: Done with base controller configuration.") );
      if (onControllerConfig( message )) {
        start(); 
        if ( message.containsKey("init") ) {
          logMessage(QNodeController::LOGLEVEL_DEBUG, getName() + " - Found init command - sending...");
          //JsonObject ini = msg["init"];
          this->onItemCommand( message["init"].as<JsonObject>() );
        }
        else if (message.containsKey( "init_list" )) {
          logMessage(QNodeController::LOGLEVEL_DEBUG, getName() + " - Found init commands - sending...");
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
   }
   else {
    logMessage("  Configuration has not changed:  Skipping controller configuration.");
   }
 }

void QNodeItemController::directConfig( String stTopic, String evTopic, String cmTopic ) {
  logMessage(getItemTag() + F(" Controller: Processing MQTT configuration message: "));    
  if (stTopic != "") {
    stateTopic = stTopic;
    logMessage( String(F("  State topic: "))+stateTopic);
  }
  if (evTopic != "") {
    eventTopic = evTopic;
    logMessage( String(F("  Event topic: "))+eventTopic);
  }
  if (cmTopic != "") {
    cmdTopic = cmTopic;
    logMessage( String(F("  Command topic: "))+cmdTopic);
  }
  logMessage( getItemTag() + F(" Controller: Done with direct configuration.") );
  lastConfig = getOwner()->getTime();
  start();
}

void QNodeItemController::onItemStateDetail(const String &detailName, const String &detailValue ) {
  if (stateTopic != "") {
     String topic = stateTopic;
     publishItem( topic, detailName, detailValue, statePubFormat[PUB_STATE_DETAIL] );
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

void QNodeItemController::onItemEvent(const String &eventName, const String &addtlAttributes)  {
   if (eventTopic != "") {
     char dateStr[11];
     sprintf( dateStr, "%02d/%02d/%d", month(), day(), year() );
     String timeStr = String(dateStr)+" "+getOwner()->getNTPClient()->getFormattedTime();
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
   if (!bypass && cmdTopic != "") {
     this->publish( cmdTopic, cmd, false );
   }
   else {
  
     DynamicJsonDocument root(JSON_BUFFER_SIZE);     
     auto error = deserializeJson( root, cmd ); 
     if (!error) {
       QNodeItem::onMessage(cmdTopic, root.to<JsonObject>());
     }
     else {
        this->onMessage( cmdTopic, cmd );
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
