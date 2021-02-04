#ifndef QNODES_H
#define QNODES_H

/* 
 #define MQTT_MAX_PACKET_SIZE 768 // Note:  This DOES NOT overide the setting in PubSubClient.h - it gets compiled after So This setting MUST BE 
                                  //        updated manually in PubSubClient.h - Otherwise longer messages will not be received.  When this happens
                                  //        it is difficult to track down because incoming messages that are too long just NEVER arrive and the 
                                  //        MQTT Callback function in QNodes.h is never fired.  I've killed too much time (more than once) tracking down
                                  //        this issue (after updating PubSubClient, or reinstalling the library).   
                                  //        For PlatformIO, this parameter is added to build options in platformio.ini, so no need to keep it here.
*/
#define MQTT_BUFFER_SIZE 768      //        Above issue s/be fixed in v.2.8 of PubASub Client library - added call to setBufferSize( MQTT_BUFFER_SIZE ) right 
                                  //        creation of MQTT Client object in QNodeController::startMqtt()  */
#define DEFAULT_MQTT_PORT 1883

#define NTP_TIME_REFRESH_INTERVAL 1800000UL
#define TIME_ZONE_OFFSET -21600L

#define JSON_BUFFER_SIZE 2048
#define ARDUINOJSON_USE_LONG_LONG 1

#undef QNODE_DEBUG_VERBOSE
// #define QNODE_DEBUG_VERBOSE

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <GPTimer.h>

class QNodeController;

class QNodeObject {
   friend QNodeController;
   
   public:
     QNodeObject() { owner = nullptr; }
     virtual void publish( const String &topic, const String &msg, bool retain );
     virtual void publish( const String &topic, const JsonObject &msg, bool retain );

     virtual void logMessage( uint8_t level, String &msg );
     virtual void logMessage( String &msg );
     virtual void logMessage( const char *msg );
     virtual void logMessage( uint8_t level, const char *msg );
   
   protected:
     void setOwner( QNodeController *own ) { owner = own; }
     QNodeController *getOwner() { return owner; }
     virtual void onConfig( const String &topic, const String &message ) {};
     virtual void onConfig( const String &topic, const JsonObject& message ) {};

   private:
     QNodeController *owner = nullptr;  
};

class QNodeObserver : virtual public QNodeObject {
  friend QNodeController;
  public:
    QNodeObserver() : QNodeObject() { topics = std::vector<String>(); }
    std::vector<String>& getTopicList() { return topics; }
    void addTopic(const String &newTopic );
    void removeTopic(const  String &topic );
    virtual void onMessage( const String &topic, const String &message )=0;
    virtual void onMessage( const String &topic, const JsonObject& message )=0;
  
  private:
    std::vector<String> topics;
};

class QNodeActor : virtual public QNodeObject {

  public:  
    QNodeActor() : QNodeObject() { 
      setInactive(false);
      setUnthrottled(false);
      cycleCount = 0;
      cycleRollover = 0;
      if (!inactive) { updateTimer.start(); } 
    }
    void setName(const String &newName) { name = newName; }
    String getName() { return name; }

    unsigned long getCycleCount() { return cycleCount; }
    
    void setInactive( boolean newValue );
    boolean getInactive() { return inactive; }
    void setUnthrottled( boolean newValue );
    boolean getUnthrottled() { return unThrottled; }
    unsigned long getUpdateInterval() { return updateTimer.getInterval(); }
    void setUpdateInterval( unsigned long newInterval ) { 
      updateTimer.setInterval( newInterval );  
      logMessage( getName() + "  - Update timer set to: " + String(updateTimer.getInterval()) );
    }
    unsigned long long msSinceStarted();
    
    virtual void actorUpdate();    
    virtual void update()=0;
    unsigned long long getNumCycles();
  
  private:
    String name = "QNodeActor Base";
    boolean unThrottled = false;
    boolean inactive = false;     
    FlexTimer updateTimer = FlexTimer(250, false);
    unsigned long cycleCount = 0;
    unsigned long cycleRollover = 0;
};

class QNodeItem : public QNodeActor, public QNodeObserver {
  friend QNodeController;
  public:
   
    enum PublishFormat { PUB_TEXT = 0, PUB_JSON = 1, PUB_NONE = 2 };

    QNodeItem( const String &initTag, const String &initID ) : QNodeActor(), QNodeObserver() {
      itemConstructionCounter++;
      setItemTag(initTag);
      setItemID( initID );
      setInactive( true );
    }
    QNodeItem( const String &initTag ) : QNodeItem( initTag, initTag ) { }
    
    virtual ~QNodeItem() {}

    void setItemID( const String newID ) { 
      if (newID=="") {
        itemID = getItemTag() + "-" + String(itemConstructionCounter);
      }
      else if (newID != itemID) { itemID = newID; }
    }
    String getItemID() { return itemID; }

    void setItemTag( const String &newTag ) { if (itemTag != newTag) { itemTag = newTag; } }
    String getItemTag() { return itemTag; }
    
    virtual void onItemStateChange(String const &stateName, String const &stateValue) {}
    virtual void onItemStateChange( const JsonObject &stateMessage ) {}
    virtual void onItemConfig(const JsonObject &message) {}
    virtual void onItemCommand(const JsonObject &message) {}
    virtual void onItemEvent(const String &eventName ) {}
    virtual void fillItemProperties( JsonObject &props ) {}
    //virtual void onItemUpdate() {}

    virtual void onItemAttach( QNodeController *owner ) {}
    virtual void onItemDetach( QNodeController *owner ) {}

    virtual boolean isConfigMessage( const String &topic, const JsonObject &message ) = 0;    
    virtual void onConfig( const String &topic, const JsonObject &message ) override { if (isConfigMessage( topic, message )) { this->onItemConfig( message ); } }
    virtual void onConfig( const String &topic, const String &message) {};  
    
    virtual boolean isCommandMessage( const String &topic ) = 0;
    virtual void onMessage( const String &topic, const JsonObject &message ) override {if (isCommandMessage(topic)) { onItemCommand(message); } }
    virtual void onMessage( const String &topic, const String &message ) override {}

    virtual void onLongOpEnd( unsigned long timeTaken ) { } 

    void markLongOpStart() { longOpStartMillis = GET_TIME_MILLIS_ABS; }
    void markLongOpEnd() {
      if (longOpStartMillis) {
        onLongOpEnd( GET_TIME_MILLIS_ABS - longOpStartMillis );
        longOpStartMillis = 0;
      }
    }

    virtual void update() override {}; 
    virtual void actorUpdate() override;
    
    boolean isStarted() { return started; }
    
    void start();
    void stop();
   
    virtual String getCompareString() { return getItemID(); }
    boolean sameAs(QNodeItem &compare ) { return (getCompareString()==compare.getCompareString() ); }

  protected:
    void publishItem( const String topic, const String value );
    void publishItem( const String topic, JsonObject &content );
    void publishItem( const String topic, const String &attrName, const String &attrValue, PublishFormat format );
   
  private:
    static unsigned long itemConstructionCounter;
    String itemID = "";
    String itemTag = "";
    boolean started = false;  
    unsigned long longOpStartMillis = 0;
    
    QNodeItem() {} // Prevent construction without tag
};

class QNodeController : public QNodeItem {

public:
  static const uint8_t LOGLEVEL_SILENT    = 0;
  static const uint8_t LOGLEVEL_INFO      = 1;
  static const uint8_t LOGLEVEL_DEBUG     = 2;
  static const char* LOG_TO_SERIAL ;

  static const char slash = '/';
  static const char pound = '#';

public:
  
  QNodeController( const char *init_ssid, const char *password, const char *mqtt_server, const int mqtt_port, const char *mqtt_user, const char *mqtt_password, const String &rootHostTopic );
  QNodeController( const char *init_ssid, const char *password, const char *mqtt_server, const char *mqtt_user, const char *mqtt_password, const String &rootHostTopic ): QNodeController( init_ssid, password, mqtt_server, DEFAULT_MQTT_PORT, mqtt_user, mqtt_password, rootHostTopic ) {}
  QNodeController( const String &init_ssid, const String &password, const String &mqtt_server, const String &mqtt_user, const String &mqtt_password, const String &rootHostTopic ):QNodeController( init_ssid.c_str(), password.c_str(), mqtt_server.c_str(), DEFAULT_MQTT_PORT, mqtt_user.c_str(), mqtt_password.c_str(), rootHostTopic ) {}
  
  void setSketchVersion( const String &initVer ) { sketchVersion = initVer; }
  String getSketchVersion() { return sketchVersion; }

  void setLogLevel( uint8_t newLevel ) { logLevel = (newLevel > LOGLEVEL_DEBUG ? LOGLEVEL_DEBUG : newLevel);  }
  uint8_t getLogLevel() { return logLevel; }
  void setLogTopic(String newTopic) { logTopic = newTopic; }
  String getLogTopic() { return logTopic; }

  WiFiClient *getWiFiClient() { return( wifiClient ); }
  PubSubClient* getMqttClient() { return mqttClient; }
  NTPClient *getNTPClient() { return ntpClient; }
  
  String getHostRootTopic();
  String getHostConfigBaseTopic();
  String getHostConfigTopic();
  String getHostLogTopic();
  String getHostStateTopic();

  String getRootTopic() { return mqttHostRoot; }
  void setRootTopic( String &newRootTopic );

  time_t getTime() { if (!timeSet) { return 0; } else { return now(); } }

  virtual void onMQTTSend( String &topic, String &message ) {}
  void mqtt_publish( const String &topic, const String &msg, bool retain );
  void mqtt_publish( const String &topic, const JsonObject &msg, bool retain );

  void logMessage( uint8_t level, String &msg ) override;
  void logMessage( String &msg ) override { logMessage( LOGLEVEL_INFO, msg );}
  void logMessage( const char *msg ) override { String message = String(msg); logMessage( message ); }
  void logMessage( uint8_t level, const char *msg ) override { String message = String(msg); logMessage( level, message ); }

  void attachItem( QNodeItem *item );
  void detachItem( QNodeItem *item );

  virtual boolean isConfigMessage( const String &topic, const JsonObject &message ) override { return topic == getHostConfigTopic(); }    
  virtual void onConfig( const String &topic, const JsonObject &msg ) override;
  virtual boolean isCommandMessage( const String &topic ) override { return false; } 

  void mqttSubscribeTopic(const String &topic );
  void mqttUnsubscribeTopic(const String &topic, boolean force );

  virtual void onLongOpEnd( unsigned long timeTaken ) override {
    for (auto i : items) {
      if (i != this) { i->onLongOpEnd(timeTaken); }
    }
  }
  
  void fillItemProperties( JsonObject &props ) override;
  
  virtual void onMQTTReceive(String &topic, String &message ) {} 
  virtual void onMessage( const String &topic, const String &message ) override;
  virtual void onMessage( const String &topic, const JsonObject &msg ) override;
  virtual void update() override;
  void loop();
  void connect();
  void dispatchMessage( String topic, String message );
  void publishState();

protected:
  boolean topicInUse( const String &topic );
  void mqttCallback( char* topic, byte* payload, unsigned int length );
  void subUnsubAllTopics(bool sub);
  void setConfigItems();
  void sendStateJson();
  int dstOffset (unsigned long unixTime);
  void updateTime();

  unsigned long bootTime = 0;
  unsigned long cycleCount = 0;
  unsigned long recdTextMsg = 0;
  unsigned long recdJsonMsg = 0;
  unsigned long pubMsg = 0;
  unsigned long sentMsg = 0;
  unsigned long wifiReconnect = 0;
  unsigned long mqttReconnect = 0;
  String lastDisconnectReason = "";
  StepTimer pulseTimer = StepTimer(60000UL);  // 60 seconds

protected:
  bool startWifi();
  bool wifiConnected();
  void endWifi();
  bool startMqtt();
  bool mqttConnected();
  void endMqtt();
  bool startNtp();
  bool ntpConnected();
  void endNtp();
  
  PubSubClient *mqttClient = nullptr;
  NTPClient* ntpClient = nullptr;

  unsigned long nodeStarted = 0;
  String ssid = "";
  String netPassword = "";
  String mqttServerName = "";
  String mqttUserName = "";
  String mqttPassword = "";

  String mqttHostRoot;
  String logTopic = "log";
  String stateTopic = "state";
  String configTopic = "config";
  bool currItemsSet = false;
  String currHostName = "";
  String currChipID = "";
  String currIPAddr = "";
  String currMACAddr = "";
  String currBootTimeStr = "";
  
  int mqttPort = DEFAULT_MQTT_PORT;
  #ifdef QNODE_DEBUG_VERBOSE
  uint8_t logLevel = LOGLEVEL_DEBUG;
  #else
  uint8_t logLevel = LOGLEVEL_INFO;
  #endif

  bool timeSet = false;

private:
  String sketchVersion;
  WiFiUDP *ntpUdp = nullptr;
  bool ntpStarted = false;
  StepTimer ntpTimer = StepTimer( NTP_TIME_REFRESH_INTERVAL, false );
  WiFiClient *wifiClient = nullptr;
  String configNewHostName = "";
  QNodeItem *findVectorItem(std::vector<QNodeItem *> list, QNodeItem &newItem ); 
  std::vector<QNodeItem *> pendingItems = std::vector<QNodeItem *>();         // during configuration - items are added to this list then moved to the item collection 
                                                                              // at the end of the main processing loop, since adding them updates the internal list
                                                                              // ...which is being iterated during normal processing.                                                                           
  std::vector<QNodeItem *> items = std::vector<QNodeItem *>();                 
  std::vector<String> subdTopics = std::vector<String>();                     // Maintain central list of all subscribed topics
};

#endif
