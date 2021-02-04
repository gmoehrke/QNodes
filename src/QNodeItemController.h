
#ifndef QNODE_ITEM_CONTROLLER_H
#define QNODE_ITEM_CONTROLLER_H

#include "QNodes.h"
#include "QNFactory.h"

class QNodeItemController : public QNodeItem {
  friend QNodeController;
    
  private:
    static QNFactory<QNodeItemController> *f;  
    String description = "";
    
  public:
    static const String LOCAL_BCAST_TOPIC;
    static const String GLOBAL_BCAST_TOPIC;
     
    enum StatePubLevel { PUB_STATE = 0, PUB_STATE_DETAIL = 1 };
    
    static QNFactory<QNodeItemController>* getFactory();

    /* Note:  ItemID is used as the subtopic for configuration messages.  We construct with no ItemID, so it defaults to the itemTag.  This will 
     *  work if only one controller of each type is used.  When a second controller of the same type is added, it must use a different subtopic 
     *  and therfore have a different item ID - which can be set by calling setConfigSubtopic() (or setItemID() since they are synonymous)
     */
    QNodeItemController(const String &initTag) : QNodeItem( initTag ) {  
      statePubFormat[PUB_STATE] = PUB_JSON;
      statePubFormat[PUB_STATE_DETAIL] = PUB_TEXT;
    }

    String getDescription() { if (description.length()==0) { return( getItemID() + " (" + getName() + ")" ); } else { return description; }}
    String setDescription( String& desc ) { return (description = desc); }

    String getConfigSubtopic() { return getItemID(); }
    void setConfigSubtopic(const String &newSt) { setItemID(newSt); }
    
    virtual boolean isConfigMessage(const String &topic, const JsonObject &message ) override { return (topic.endsWith(getConfigSubtopic()) ); }
    /*  Configuration hook - override in descendant to do any additional config - return true to start the 
     *   controller.  If returns false - controller may still be configured, but needs to be started 
     *   manually by calling start() to begin handling updates.
     */
     
    virtual boolean onControllerConfig( const JsonObject &message ) { return true; }
    virtual void onItemConfig( const JsonObject &message) override;
    /* Calling direct configuration - bypasses the need to have configuration in retained messages in MQTT, 
     * however, after successful direct configuration - the caller needs to call start() to begin
     * processing updates.
     */
    virtual void directConfig( String stTopic, String evTopic, String cmTopic );


    void localBroadcast( String message ) { if (getOwner()) { getOwner()->dispatchMessage( LOCAL_BCAST_TOPIC, message ); } }
    void localBroadcast( String message, String itemTag ) { if (getOwner()) { getOwner()->dispatchMessage( LOCAL_BCAST_TOPIC + itemTag, message ); } }
    
    //PublishFormat getStatePubFormat() { return statePubFormat; }
    //void setStatePubFormat( PublishFormat newFormat ) { statePubFormat = newFormat; }           
    
    PublishFormat getStatePublishFormat( const StatePubLevel level ) { return statePubFormat[level]; }
    void setPublishFormat( const StatePubLevel level, const PublishFormat newFormat ) {
      if (statePubFormat[level] != newFormat) {
        statePubFormat[level] = newFormat;
      }
    }

    virtual void onItemStateChange( const String &stateValue ) { publishItem( stateTopic, stateValue ); }
    virtual void onItemStateChange(const String &stateName, const String &stateValue) { if (stateTopic != "") { publishItem( stateTopic, stateName, stateValue, statePubFormat[PUB_STATE] ); } }
    virtual void onItemStateChange( const JsonObject &stateMessage ) { if (stateTopic != "") { publish( stateTopic, stateMessage, true ); } }
    
    /* stateDetail is published one level below the stateTopic (ex:  /home/sensor/state/<detailName>)
     *    This allows for publishing further detail regarding the state of an item - potentially when
     *    the detail itself has changed, but the overall state has not.  For example, when publishing
     *    a countdown timer for a motion sensor.  The sensor is still "on" but the time remaining is 
     *    continually changing.  This allows for publishing the changing element without re-publishing
     *    the state itself.
     */
    virtual void onItemStateDetail(const String &detailName, const String &detailValue );
    virtual void onItemStateDetail( const String detailName, const JsonObject &detailMessage );       

    
    virtual void onItemAttach( QNodeController *owner ) override { addTopic( owner->getHostConfigBaseTopic()+"/"+getConfigSubtopic() ); }
  
    virtual boolean isCommandMessage( const String &topic ) { 
      return topic.equals(cmdTopic) || 
             topic.equals(LOCAL_BCAST_TOPIC) || 
             topic.equals(GLOBAL_BCAST_TOPIC) ||
             topic.equals(LOCAL_BCAST_TOPIC+getItemTag()) || topic.equals(LOCAL_BCAST_TOPIC+getItemID()) ||
             topic.equals(GLOBAL_BCAST_TOPIC+getItemTag()) || topic.equals(GLOBAL_BCAST_TOPIC+getItemID()); 
    }

    //
    // There are 2 ways of processing incoming commands via Json:
    //     1.)  override onItemCommandElement and process each element of the Json command individually.  This is the simplest
    //          method as it only requires overriding a single method.  The <context> parameter contains a "map" to where the 
    //          element was located in the Json document using dot notation.  A single dot represents the root element.  Given
    //          the following Json:  {"id":"my_item", "color" : { "r":50, "g": 100, "b": 150 }, "brightness" : 255 }
    // 
    //          onItemCommandElement will be called 5 times with the following values:
    //             
    //          context         key         value
    //         ---------       -----      ---------
    //            "."           "id"       "my_item"
    //           ".color"       "r"           50
    //           ".color"       "g"          100
    //           ".color"       "b"          150
    //            "."       "brightness"     255
    //
    //    2.)  If multiple command elements are needed simultaneously, or it doesn't make sense to process commands
    //         individually for some other reason, simply override onItemCommand and parse the Json document 
    //         manually to process the command.
    //
    //    If a mixed approach is preferred, both methods can be overridden - and after dealing any overall or combined keys in the 
    //    Json - onItemCommandObject() can be called, which will then process all of the keys and pass them to onItemCommandElement - 
    //    then, simply ignore keys that have already been dealt with. 
    // 
    //    Note:  Arrays are passed as a single JsonVariant, so if they contain embedded objects, those would need to processed within the 
    //           onItemCommandElement method. onItemCommandElement, could use subsequent calls to onItemCommandObject to iterate through
    //           those complex objects as well - just pass an appropriate context, so onItemCommandElement know from where they came.
    //
    virtual void onItemCommandElement( String context, String key, JsonVariant& value ) {}

    void onItemCommandObject( String context, const JsonObject& node ) {
      for (auto kvp : node) {
        if (kvp.value().is<JsonObject>()) {
          String k = context;
          k += ".";
          k += String(kvp.key().c_str());
          JsonObject o = kvp.value().as<JsonObject>();
          onItemCommandObject( k, o );
        }
        else {
          JsonVariant v = kvp.value();
          String k = String(kvp.key().c_str());
          onItemCommandElement( context, k, v );
        }
      }
    }
    
    virtual void onItemCommand(const JsonObject &message) { 
      onItemCommandObject(String(""), message); 
    }
    
    void command( const String &cmd, boolean bypass );
    void command( const String &cmd ) { command( cmd, false); }
    
    virtual void onItemEvent(const String &eventName, const String &addtlAttributes );
    virtual void onItemEvent(const String &eventName) { onItemEvent( eventName, "" ); }

    virtual void onMessage( const String &topic, const String &message ) override { if (isCommandMessage(topic)) { onItemEvent( "Invalid Command Message" ); }}

    virtual void fillItemProperties( JsonObject &props ) override;

  private:
    PublishFormat statePubFormat[StatePubLevel::PUB_STATE_DETAIL+1];
    String stateTopic = "";
    String eventTopic = "";
    String cmdTopic = ""; 
    String currConfigStr = "";  
    time_t lastConfig = 0;   
    String lastConfigStr = "";   
};


#endif
