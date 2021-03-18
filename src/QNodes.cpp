#include "QNodes.h"
#include "QNodeItemController.h"
#include "TimeLib.h"
#include <Limits.h>
#include <string.h>

void QNodeObject::publish( const String &topic, const String &msg, bool retain ) { if (owner) { owner->mqtt_publish( topic, msg, retain); } }
void QNodeObject::publish( const String &topic, const JsonObject &msg, bool retain ) { if (owner) { owner->mqtt_publish( topic, msg, retain); } }

void QNodeObject::logMessage( uint8_t level, String &msg ) { if (owner) { owner->logMessage( level, msg ); } }
void QNodeObject::logMessage( String &msg ) { if (owner) {owner->logMessage(msg); } }
void QNodeObject::logMessage( const char *msg ) { if (owner) { owner->logMessage( msg ); } }
void QNodeObject::logMessage( uint8_t level, const char *msg ) { if (owner) {owner->logMessage( level, msg ); } }

void QNodeObserver::addTopic(const String &newTopic ) { 
  if (std::find(topics.begin(), topics.end(), newTopic) == topics.end() ) {
    topics.push_back( newTopic ); 
    if (getOwner()) { 
      getOwner()->mqttSubscribeTopic(newTopic);       
    }
  }
}

void QNodeObserver::removeTopic(const String &topic ) { 
  if (std::find(topics.begin(), topics.end(), topic) != topics.end() ) {
    topics.erase(std::remove(topics.begin(), topics.end(), topic), topics.end()); 
    if (getOwner()) { 
        getOwner()->mqttUnsubscribeTopic( topic, false );  
   } 
 }
}

void QNodeActor::setInactive( boolean newValue ) { 
  if (newValue != inactive) {
    inactive = newValue;
    if ( !inactive && !unThrottled ) { updateTimer.start(); }
    else { updateTimer.stop(); }
  }
}

void QNodeActor::setUnthrottled( boolean newValue ) {
  if (newValue != unThrottled) {
    unThrottled = newValue;
    if (updateTimer.isStarted() && !inactive) { updateTimer.start(); }
  }
}    

void QNodeActor::actorUpdate() {
  if (!inactive) { 
    if ((updateTimer.isUp() || unThrottled)) {
      update();
      if (cycleCount == ULONG_MAX) { 
        cycleRollover++; 
        cycleCount = 0;
      }
      else {
        cycleCount++;
      }
      if (updateTimer.isStarted()) { updateTimer.step(); }
    }
  }
  yield();
}

unsigned long long QNodeActor::msSinceStarted() {
  return (updateTimer.getRollovers() * ULONG_MAX) + updateTimer.timeSinceStarted();
}

unsigned long long QNodeActor::getNumCycles() { 
   return (cycleRollover * ULONG_MAX) + cycleCount;
}

unsigned long QNodeItem::itemConstructionCounter = 0;

void QNodeItem::actorUpdate() {
  if (started) { 
      QNodeActor::actorUpdate(); 
//      onItemUpdate(); 
  }
}


void QNodeItem::start() { 
  if (!started) {
    setInactive( false );
    started = true; 
    logItemEvent(getName() + " Started update handler.");
  }
} 
   
void QNodeItem::stop() { 
  if (started) { 
    logItemEvent(getName() + " Stopped update handler");
    started = false; 
    setInactive( true );
  }
}


void QNodeItem::publishItem( const String topic, const String value ) { publish( topic, value, true ); }
void QNodeItem::publishItem( const String topic, JsonObject &content ) { publish( topic, content, true ); }
void QNodeItem::publishItem( const String topic, const String &attrName, const String &attrValue, PublishFormat format ) {
  switch (format) {
    case PUB_TEXT : { this->publish( topic + QNodeController::slash + attrName, attrValue, true ); break; }
    case PUB_JSON : { String js = "{\"";
                      js += attrName;
                      js += "\":\"";
                      js += attrValue;
                      js += "\"}";
                      this->publish( topic,   js, true );
                      break;
                    }
  }
}

const char* QNodeController::LOG_TO_SERIAL = "Serial";

QNodeController::QNodeController( const char *init_ssid, const char *password, const char *mqtt_server, int mqtt_port, const char *mqtt_user, const char *mqtt_password, const String &rootHostTopic ) : QNodeItem("QNODE_INIT") {
    setName(String(F("QNode Main Controller")));
    ssid = String(init_ssid);
    netPassword = String(password);
    mqttServerName = String(mqtt_server);
    mqttPort = mqtt_port;
    mqttUserName = String(mqtt_user);
    mqttPassword = String(mqtt_password);
    mqttHostRoot = rootHostTopic;
    ntpStarted = false;
    timeSet = false;
    logMessage( "Initializing...");
    attachItem(this);
    connect();    
    String topic=getHostConfigTopic();
    addTopic( topic ); 
    setUnthrottled( true );
    start();    
    pulseTimer.start();
 }

String QNodeController::getHostRootTopic() {
  String result = mqttHostRoot;
  result += slash;
  result += currHostName;
  return result;
}

String QNodeController::getHostConfigBaseTopic() {
  String result = getHostRootTopic();
  result += slash;
  result += configTopic;
  return result;
}

String QNodeController::getHostConfigTopic() {
  String result = getHostConfigBaseTopic();
  return result;
}

String QNodeController::getHostLogTopic() {
  String result = getHostRootTopic();
  result += slash;
  result += logTopic;
  return result;
}

String QNodeController::getHostStateTopic() {
  String result = getHostRootTopic();
  result += slash;
  result += stateTopic;
  return result;
}


bool QNodeController::startWifi() {
  // Connect to WiFi network
  logMessage("Establishing WiFi connection to "+ssid+" ("+netPassword+")" );
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid.c_str(), netPassword.c_str());
  while (!wifiConnected()) {
    delay(500);
    logMessage("...waiting for connect...");
  }
  currHostName = String(WiFi.hostname());
  currIPAddr = WiFi.localIP().toString();
  currMACAddr = WiFi.macAddress();
  logMessage("WiFi connected");
  logMessage("IP address: " + WiFi.localIP().toString());
  logMessage("Host name: "+String(currHostName) );  
  return(WiFi.status() == WL_CONNECTED);
}

bool QNodeController::wifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

void QNodeController::endWifi() {
  logMessage("Wifi disconnecting...");
  endNtp();
  endMqtt();
  WiFi.disconnect();
}

void QNodeController::subUnsubAllTopics( bool sub ) { 
    logMessage("Refreshing subscribed topics." );
    for (auto i : items ) {
         for( auto j : i->getTopicList() ) {
            if (sub) {
              mqttSubscribeTopic( j );
            }
            else {
              mqttUnsubscribeTopic( j, true );
            }
          }
       }
}

bool QNodeController::startMqtt() {
    bool result = true;
    String st;
    if (wifiClient==nullptr) {  wifiClient = new WiFiClient(); }
    if (mqttClient==nullptr) {  mqttClient = new PubSubClient(mqttServerName.c_str(), DEFAULT_MQTT_PORT, std::bind(&QNodeController::mqttCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), *wifiClient); }
    if (!wifiConnected()) {
      if (!startWifi()) {
        return false;
      }
    }
    if (mqttConnected()) {
      mqttClient->setBufferSize(MQTT_BUFFER_SIZE);
      return true; 
    }
    st = F("Starting MQTT service.");
    logMessage(st);
    // Clear the list of actively subscribed topics - will re-sub after connection is established 
    subdTopics.clear();
    #ifdef QNODE_DEBUG_VERBOSE
    logMessage(LOGLEVEL_DEBUG, "Attempting MQTT connection to: "+mqttServerName+" ("+String(mqttPort)+") as "+currHostName+" ["+mqttUserName+"/*password*]" );
    #endif
    if (mqttClient->connect(currHostName.c_str(), mqttUserName.c_str(), mqttPassword.c_str())) {
      st = F("MQTT connection established.");
      logMessage(LOGLEVEL_INFO, st);
      // Subscribe to any topics we're listening to...
      logMessage( "MQTT Connected - Max Packet Size is " + String(MQTT_MAX_PACKET_SIZE));
      subUnsubAllTopics(true); 
    }    
    else {    
      #ifdef QNODE_DEBUG_VERBOSE
      logMessage(LOGLEVEL_DEBUG, "MQTT Error on connect("+currHostName+", "+mqttServerName+", "+mqttUserName+", "+mqttPassword+") ");
      #endif
      st = F("MQTT Connection unsuccessful, error code: ");
      logMessage(st+String(mqttClient->state()));
      result = false;
    }
    return result;
}

bool QNodeController::mqttConnected() {
  bool result = true;
  if (mqttClient) {
    result = (mqttClient->state()==MQTT_CONNECTED);
  }
  else { result = false; }
  return result;
}

void QNodeController::endMqtt() {
    logMessage("Stopping MQTT service.");
    if (mqttClient) { 
      mqttClient->disconnect();
    }  
}

bool QNodeController::startNtp() {
  bool result = true;  
  if (wifiConnected() && !ntpStarted) {
    String st = F("Starting NTP time update service.");
    logMessage(st);
    if (ntpClient==nullptr) {
      ntpUdp = new WiFiUDP();
      ntpClient = new NTPClient(*ntpUdp, TIME_ZONE_OFFSET);
    }  
    //ntpClient->begin();
    ntpStarted = true;
    logMessage("NTP Time started.");
    ntpTimer.start();
    updateTime();
    bootTime = ntpClient->getEpochTime()-(GET_TIME_MILLIS_ABS/1000);
  }
  return result;  
}

bool QNodeController::ntpConnected() {
  return (ntpStarted);
}

void QNodeController::endNtp() {
  if (ntpConnected()) {
    String st = F("Stopping NTP time update service.");
    logMessage(st);    
    ntpStarted = false;
    ntpTimer.stop();
    st = F("NTP time update service stopped.");
    logMessage(st);    
  }
}

void QNodeController::connect() {
  
  if (startWifi()) {
      startNtp();
      startMqtt();
      if (mqttConnected()) { 
        sendStateJson();
      }
    }
}

void QNodeController::logMessage( uint8_t level, String &msg ) {
    String msgStr;
    if (logLevel >= level) {
      if (timeSet) {
          char dateStr[11];
          sprintf( dateStr, "%02d/%02d/%d", month(), day(), year() );
          msgStr = "["+String(dateStr)+" "+ntpClient->getFormattedTime()+"] ";
      }
      else {
        msgStr = "[ boot "+String(GET_TIME_MILLIS_ABS)+" ms] ";
      }
      msgStr.concat( msg );
      if ( (strcmp(logTopic.c_str(), LOG_TO_SERIAL)==0)  || (!mqttConnected()) )
        Serial.println( msgStr );
      else {
        String topic = getHostLogTopic();;
        publish( topic, msgStr, false );  
      }
    }
  }

void QNodeController::attachItem( QNodeItem *item ) { 
  item->setOwner(this);
  items.push_back(item);   
  logMessage( LOGLEVEL_DEBUG, "Attaching item: " + item->getItemTag());
  for (auto t : item->getTopicList() ) { 
      mqttSubscribeTopic( t );
   }
  item->onItemAttach( this ); 
}

void QNodeController::detachItem( QNodeItem *item ) { 
  if (std::find(items.begin(), items.end(), item) != items.end() ) {
    items.erase(std::remove(items.begin(), items.end(), item), items.end());
    item->onItemDetach( this );   
    item->setOwner(nullptr);
    for (auto t : item->getTopicList() ) {   
      mqttUnsubscribeTopic( t, false );
    }
  } 
}

boolean QNodeController::topicInUse(const String &topic ) {
  boolean result = false;
  for (auto i : items) {
    if (std::find(i->getTopicList().begin(), i->getTopicList().end(), topic) != i->getTopicList().end() )
      result = true;
      break;
  }
  return result;
}

void QNodeController::mqttSubscribeTopic(const String &topic ) {    
  if (mqttConnected()) {
    if (std::find( subdTopics.begin(), subdTopics.end(), topic) == subdTopics.end()) {
      if (mqttClient->subscribe( topic.c_str() )) {
        subdTopics.push_back( topic );
        logMessage(String(F("MQTT:  Subscribed to topic: ")) + topic );
      }
      else {
        logMessage("MQTT:  Error subscribing to topic: " + topic );
      }
    }
  }
  else {
    subdTopics.clear();
  }
}

void QNodeController::mqttUnsubscribeTopic(const String &topic, boolean force ) {    
  if (mqttConnected()) {
    if (std::find( subdTopics.begin(), subdTopics.end(), topic) != subdTopics.end() ) {
      if (!topicInUse(topic) || force) {
        mqttClient->unsubscribe( topic.c_str() );
        subdTopics.erase(std::remove(subdTopics.begin(), subdTopics.end(), topic), subdTopics.end()); 
        logMessage(QNodeController::LOGLEVEL_INFO,String(F("MQTT:  Unsubscribed to topic: ")) + topic );      
      }
    }
  }
  else {
    subdTopics.clear();
  }
}

void QNodeController::mqtt_publish(const String &topic, const String &msg, bool retain ) {
  if (mqttConnected()) {
    String t = topic;
    String m = msg;
    onMQTTSend( t, m );

    mqttClient->beginPublish(t.c_str(), msg.length(), retain );
    for (uint16_t i=0; i<msg.length(); i++) {
      mqttClient->write(msg.c_str()[i]);
    }
    mqttClient->endPublish();
    pubMsg++;
  }
  else {
    logMessage(QNodeController::LOGLEVEL_INFO, String(F("MQTT connection unavailable.  Unable to publish to: "))+topic+" Message: "+msg );
  }
}

void QNodeController::mqtt_publish( const String &topic, const JsonObject &msg, bool retain ) {
  String jsonStr = "";
  serializeJson( msg, jsonStr ); 
  
  #ifdef QNODE_DEBUG_VERBOSE
  logMessage( LOGLEVEL_DEBUG, "JSON serialized:  "+jsonStr );
  #endif 
  
  if (mqttConnected()) {
    publish( topic, jsonStr, retain );
  }
  else {
    logMessage( String(F("MQTT connection unavailable.  Unable to publish JSON to: "))+topic+" Message: "+jsonStr );
  }
}

void QNodeController::dispatchMessage( String topic, String message ) {
    
      DynamicJsonDocument doc(JSON_BUFFER_SIZE);
      JsonObject root = doc.to<JsonObject>(); 
      auto error = deserializeJson( doc, message );
      #ifdef QNODE_DEBUG_VERBOSE
      if (error) {
        this->logMessage(LOGLEVEL_DEBUG, "Not a valid JSON message.");
        this->logMessage(LOGLEVEL_DEBUG, message );
        this->logMessage(LOGLEVEL_DEBUG, error.c_str() );
      }
      else {
        this->logMessage( LOGLEVEL_DEBUG, "Sucessfully parsed incoming JSON message.");
        this->logMessage( LOGLEVEL_DEBUG, "JSON Buffer Size:  " + String(measureJson(doc)) );
      }
      #endif 
      
      if ( topic.startsWith(getHostConfigBaseTopic())) {        
        for (auto i : items) 
        {
          if (error) { 
            (i)->onConfig(topic, message) ; }
          else { 
            (i)->onConfig(topic, root ); 
          }
        }       
      }  // Not a config message....
      else {
        for (auto i : items)
        {
          if (error) {
            (i)->onMessage( topic, message );
          }
          else {
            (i)->onMessage( topic, root );
          }
        }
      }
}

void QNodeController::mqttCallback( char* topic, byte* payload, unsigned int length ) {
    #ifdef QNODE_DEBUG_VERBOSE
    logMessage(LOGLEVEL_DEBUG, "** mqttCallback:  callback executed - topic:  " + String(topic));
    #endif
    String stTopic = String(topic);
    char message[length + 1];
    for (unsigned int i = 0; i < length; i++) {
      message[i] = (char)payload[i];
    }
    message[length] = '\0';
    String stMessage = String(message);
    if (stMessage!="") {
      onMQTTReceive( stTopic, stMessage );
      
      #ifdef QNODE_DEBUG_VERBOSE
      this->logMessage(LOGLEVEL_DEBUG, "******************************");
      this->logMessage(LOGLEVEL_DEBUG, "Message received on ["+stTopic+"]: "+stMessage);    
      this->logMessage(LOGLEVEL_DEBUG, "Free Heap Size:  " + String(ESP.getFreeHeap()) );
      this->logMessage(LOGLEVEL_DEBUG, "******************************");
      #endif
      
      dispatchMessage( stTopic, stMessage );
    }
}

void QNodeController::onConfig( const String &topic, const JsonObject &msg ) {
  if (msg.containsKey("hostname")) {
    logMessage( LOGLEVEL_INFO, "Request new host name:  "+msg["hostname"].as<String>()+" - Wifi willl reset..." );
    configNewHostName = msg["hostname"].as<String>();
  } 

  if (msg.containsKey("description")) {
    setDescription(msg["description"].as<String>());
  }

  if (msg.containsKey("items")) {
    logMessage("Creating Items:  ");
    for (auto ctx : msg["items"].as<JsonArray>()) {
      if (ctx.as<JsonObject>().containsKey("tag")) {      
        boolean found = false;
        for (auto i : items) {
          if (i->getItemTag()==ctx["tag"]) { 
            if (!(ctx["id"])) { found = true; break; }
            else if (ctx["id"]==getItemID()) { found = true; break; }
          }
        }
        if (!found) {
          logMessage("  Item: " + ctx["tag"].as<String>() );
          QNodeItemController *qni = QNodeItemController::getFactory()->create(ctx["tag"].as<String>());
          if (ctx.as<JsonObject>().containsKey("id")) {
             qni->setConfigSubtopic(ctx["id"].as<String>());
          }
          pendingItems.push_back( qni );         
          logMessage("  Item: " + ctx["tag"].as<String>() + " created." );
        }
    }
  }
 }
}

void QNodeController::onMessage( const String &topic, const String &message )  {
      recdTextMsg++;
      #ifdef QNODE_DEBUG_VERBOSE
      logMessage(LOGLEVEL_DEBUG, "Text based message received ["+topic+"]: "+ message);        
      #endif
}

void QNodeController::onMessage( const String &topic, const JsonObject &msg ) {
  recdJsonMsg++;
}

int QNodeController::dstOffset (unsigned long unixTime)
{
  //Receives unix epoch time and returns seconds of offset for local DST
  //Valid thru 2099 for US only, Calculations from "http://www.webexhibits.org/daylightsaving/i.html"
  //Code idea from jm_wsb @ "http://forum.arduino.cc/index.php/topic,40286.0.html"
  //Get epoch times @ "http://www.epochconverter.com/" for testing
  //DST update wont be reflected until the next time sync
  time_t t = unixTime;
  int beginDSTDay = (14 - (1 + year(t) * 5 / 4) % 7);  
  int beginDSTMonth=3;
  int endDSTDay = (7 - (1 + year(t) * 5 / 4) % 7);
  int endDSTMonth=11;
  #ifdef QNODE_DEBUG_VERBOSE
  logMessage(LOGLEVEL_DEBUG,  "NTP Client - Calculating DST Start Date: " + String( beginDSTMonth ) + "/" + String( beginDSTDay ) ); 
  #endif 
  if (((month(t) > beginDSTMonth) && (month(t) < endDSTMonth))
    || ((month(t) == beginDSTMonth) && (day(t) > beginDSTDay))
    || ((month(t) == beginDSTMonth) && (day(t) == beginDSTDay) && (hour(t) >= 2))
    || ((month(t) == endDSTMonth) && (day(t) < endDSTDay))
    || ((month(t) == endDSTMonth) && (day(t) == endDSTDay) && (hour(t) < 1)))
    { 
      #ifdef QNODE_DEBUG_VERBOSE
      logMessage( LOGLEVEL_DEBUG, "NTP Client - DST in effect, Adding 3600 to current time." ); 
      #endif      
      return (3600);  //Add back in one hours worth of seconds - DST in effect
    }
  else
    { 
      #ifdef QNODE_DEBUG_VERBOSE 
      logMessage( LOGLEVEL_DEBUG, "NTP Client - Not in DST, Adding 0 to current time" );
      #endif      
      return (0);  //NonDST
    }
}

void QNodeController::updateTime() {
  if (ntpConnected() && (ntpTimer.isUp() || !timeSet)) {
     ntpClient->begin();
     ntpClient->forceUpdate();
     ntpClient->end();
     ntpClient->setTimeOffset( TIME_ZONE_OFFSET + dstOffset(ntpClient->getEpochTime()) );
     setTime(ntpClient->getEpochTime());
     if (nodeStarted==0) {
       nodeStarted = ntpClient->getEpochTime()-(GET_TIME_MILLIS_ABS/1000);
     }
     timeSet = true;
     logMessage( LOGLEVEL_INFO, "NTP Time updated.");
     ntpTimer.step();     
  }
}

void QNodeController::setConfigItems() {
  if (!currItemsSet) {
    currChipID = String(ESP.getChipId(), HEX);
    currChipID.toUpperCase(); 
    currMACAddr = WiFi.macAddress();
    if (timeSet) {
          char dateStr[11];
          sprintf( dateStr, "%02d/%02d/%d", month(nodeStarted), day(nodeStarted), year(nodeStarted) );
          unsigned long hours = (nodeStarted % 86400L) / 3600;
          String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);
          unsigned long minutes = (nodeStarted % 3600) / 60;
          String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);
          unsigned long seconds = nodeStarted % 60;
          String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);
          String timeStr = hoursStr + ":" + minuteStr + ":" + secondStr;
          currBootTimeStr = String(dateStr)+" "+timeStr;
          currItemsSet = true;
    }
    else { currItemsSet = false; }            
  }
}

void QNodeController::fillItemProperties( JsonObject &props )
 {
    props["name"] = getName();
    props["id"] = getItemID();
    props["update_cycles"] = getCycleCount();
 }

void QNodeController::sendStateJson() {  
  markLongOpStart();
  String baseTopic = getHostStateTopic();
  String msgStr = "";
  DynamicJsonDocument doc(JSON_BUFFER_SIZE);
  JsonObject root = doc.to<JsonObject>();
  setConfigItems();
  if (timeSet) {
          char dateStr[11];
          sprintf( dateStr, "%02d/%02d/%d", month(), day(), year() );
          if (ntpClient != nullptr) { msgStr = String(dateStr)+" "+ntpClient->getFormattedTime(); }
          publishItem( baseTopic, "current_time", msgStr, PUB_TEXT );
  } 
  root["node_id"] = currHostName;
  root["description"] = getDescription();
  root["firmware"] = getSketchVersion();
  publishItem( baseTopic, "firmware", getSketchVersion(), PUB_TEXT );
  publishItem( baseTopic, "node_id", currHostName, PUB_TEXT );
  publishItem( baseTopic, "description", getDescription(), PUB_TEXT );
  publishItem( baseTopic, "chip_id", currChipID, PUB_TEXT );
  publishItem( baseTopic, "ip_address", currIPAddr, PUB_TEXT );
  publishItem( baseTopic, "mac_address", currMACAddr, PUB_TEXT );
  publishItem( baseTopic, "free_memory", String(ESP.getFreeHeap()), PUB_TEXT );
  publishItem( baseTopic, "wifi_signal", String(WiFi.RSSI()), PUB_TEXT );
  publishItem( baseTopic, "wifi_reconnects", String(wifiReconnect), PUB_TEXT );
  publishItem( baseTopic, "mqtt_reconnects", String(mqttReconnect), PUB_TEXT );
  publishItem( baseTopic, "mqtt_last_disconnect_state", lastDisconnectReason, PUB_TEXT );
  publishItem( baseTopic, "boot_time", currBootTimeStr, PUB_TEXT );  
  publishItem( baseTopic, "received_text", String(recdTextMsg), PUB_TEXT );
  publishItem( baseTopic, "received_json", String(recdJsonMsg), PUB_TEXT );
  publishItem( baseTopic, "published", String(pubMsg), PUB_TEXT );  
  JsonArray jsitems = root.createNestedArray("items");
  for (auto i : items) {
    publishItem( baseTopic + QNodeController::slash + String(i->getItemID()), "name", i->getName(), PUB_TEXT );
    publishItem( baseTopic + QNodeController::slash + String(i->getItemID()), "update_cycles", String(i->getCycleCount()), PUB_TEXT );
    JsonObject jsitem = jsitems.createNestedObject();
    i->fillItemProperties( jsitem );    
    JsonArray jstopics = jsitem.createNestedArray("subscribed_topics");
    for(auto j : i->getTopicList()) {
      jstopics.add(j);
     }
  }
  publish( baseTopic, root, true );
  markLongOpEnd();
}

void QNodeController::publishState() {
    sendStateJson();
}

void QNodeController::update() {
   if (!wifiConnected()) {
     endMqtt();
     logMessage("WiFi connection lost - reconnecting.");
     wifiReconnect++;
     connect();   
   }
   if (wifiConnected()) {
    if (!mqttConnected()) {
      logMessage(LOGLEVEL_INFO, "MQTT connection lost - attempting to reconnect.");     
      logMessage(LOGLEVEL_INFO, "MQTT Status -> " + String(mqttClient->state()) );
      if (mqttClient->state()==MQTT_CONNECTION_TIMEOUT) { lastDisconnectReason = "MQTT Connection Timeout"; }
      else if (mqttClient->state()==MQTT_CONNECTION_LOST) { lastDisconnectReason = "MQTT Connection Lost"; }
      else if (mqttClient->state()==MQTT_CONNECT_FAILED) { lastDisconnectReason = "MQTT Connection Failed"; }
      else { lastDisconnectReason = String(mqttClient->state()); }
      mqttReconnect += 1;
      startMqtt();
    }
    else {
     mqttClient->loop();     
     if (ntpConnected()) {
       updateTime();
     }
    }
    if (pulseTimer.isUp()) {
      publishState();
      pulseTimer.step();
    }
  }
}

QNodeItem *QNodeController::findVectorItem(std::vector<QNodeItem *> list, QNodeItem &newItem ) {
  QNodeItem *result = nullptr;
  auto it = std::find_if( list.begin(), list.end(), [&newItem](QNodeItem*& element) -> bool { return element->sameAs(newItem); } );
  if (it!=list.end()) { result = *it; }
  return result;
}

void QNodeController::loop() {
    for( auto i : items )
    { 
       i->actorUpdate();     
    } 
    if (pendingItems.size() > 0) {
      logMessage(LOGLEVEL_DEBUG, "Configuring controller:  attaching pending Items:");
      QNodeItem *pendingItem = nullptr;
      for (auto existing : items) {        
        pendingItem = findVectorItem( pendingItems, *existing );
        if (pendingItem) {
          logMessage( LOGLEVEL_DEBUG, "  Ignoring Item: " + pendingItem->getName() + " [" + pendingItem->getItemID() + "] duplicates Item: " + existing->getName() + " [" + existing->getItemID() + "]" );
          pendingItems.erase( std::remove(pendingItems.begin(), pendingItems.end(), pendingItem), pendingItems.end() );
          delete pendingItem;
          pendingItem = nullptr;
        }
      }      
      for (auto c : pendingItems) {               
        #ifdef QNODE_DEBUG_VERBOSE
        logMessage(LOGLEVEL_DEBUG, "  Adding Item:  " + c->getName() + " [" + c->getItemID() + "]" );          
        #endif         
        attachItem( c );
      }

      #ifdef QNODE_DEBUG_VERBOSE
      for (auto i : items ) {
         for( auto j : i->getTopicList() ) {
            logMessage("Currently subscribed to:  " + j );
          }
       }
      #endif

      pendingItems.clear();
    }
     
    if (!(configNewHostName.equals(""))) {
      subUnsubAllTopics(false);
      String topic=getHostConfigTopic();
      removeTopic( topic ) ;
      WiFi.hostname( configNewHostName );
      WiFi.disconnect();
      WiFi.hostname(configNewHostName);
      connect();
      setItemID(configNewHostName);
      //subUnsubAllTopics(true);
      topic=getHostConfigTopic();
      addTopic( topic ); 
      configNewHostName = "";
    }
}
