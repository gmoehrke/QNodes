#include "CoreControllers.h"
#include <ESP8266httpUpdate.h>

#ifdef QNC_LEDSTRIP
#include "FastFX.h"
#endif

void CoreControllers::registerControllers() { 
      ESPHostController::registerType();
      #ifdef QNC_COLOR_LED
      RGBLEDController::registerType();
      #endif
      #ifdef QNC_PIR
      PIRController::registerType();
      #endif
      #ifdef QNC_OAS
      OASController::registerType();
      #endif
      #ifdef QNC_RELAY
      RelayController::registerType();
      #endif
      #ifdef QNC_LDR
      LDRController::registerType();
      #endif
      #ifdef QNC_VOLT
      VSensorController::registerType();
      #endif
      #ifdef QNC_DHT
      DHTController::registerType();
      #endif
      #ifdef QNC_LEDSTRIP
      QFXController::registerType();
      #endif
      #ifdef QNC_MOCHA_X10
      MochaX10Controller::registerType();
      #endif
      #ifdef QNC_TPLINK
      TPLinkController::registerType();
      #endif
  }

//  *********** ESPHostController methods

void ESPHostController::registerType() {
  QNodeItemController::getFactory()->registerType< ESPHostController >("HOST");
}

ESPHostController::ESPHostController() : QNodeItemController( "HOST" ) {
      setName(String(F("ESP Host Controller")));
      setUpdateInterval(1000);
  }

void ESPHostController::updateFirmware(const String &url ) {
      logMessage("Firmware Update:  ");
      logMessage("  Downloading from: " + url );
      logMessage("  System will reset... " );
      ESPhttpUpdate.setLedPin( D3, 10 );
      ESPhttpUpdate.rebootOnUpdate( true );
      WiFiClient *wfc = this->getOwner()->getWiFiClient();      
      if (wfc) {      
        HTTPUpdateResult result = ESPhttpUpdate.update(*wfc,  url);
        String stUR;
        if (result==HTTP_UPDATE_FAILED) { stUR = F("Failed. "); }
        if (result==HTTP_UPDATE_NO_UPDATES) { stUR = F("No Updates."); }
        if (result==HTTP_UPDATE_OK){ stUR = F("Success? "); }
        String stResult = "Firmware update result: "+stUR;
        // ** Note:  The firmware update result must include rebootOnUpdate=true or it doesn't
        //           work.  The process downloads the firmware, then reboots to installl it
        //           setting rebootOnUpdate to false downloads the file, but it never gets
        //           installed - and doesn't appear to carry over until the next reboot either...
      }
      else {
        String stUR = F("Firmware update failed - unable to retrieve WiFi Client.");
        logMessage(stUR);
      }
  }

void ESPHostController::onItemCommandElement( String context, String key, JsonVariant& value ) {
    String vStr = value.as<String>();
    String path = context + "." + key;
    if (context.equals(".led1") || context.equals("led2")) {
      MonoVariableLED& led = context.equals(".led1") ? leds.led1() : leds.led2();
      if (key.equals("state")) { led.setState( vStr.equals("on") ); }
      if (key.equals("level")) { led.setBrightness( value ); }
    }
    if (path.equals(".firmware")) { updateFirmware( vStr ); }
    if (path.equals(".restart") && (vStr.equals("yes")||vStr.equals("true"))) { ESP.restart(); }
    if (path.equals(".report")) { getOwner()->publishState(); }
    if (path.equals(".debug")) { getOwner()->setLogLevel( (vStr.equals("yes")||vStr.equals("true")) ? QNodeController::LOGLEVEL_DEBUG : QNodeController::LOGLEVEL_INFO ); }
  }

boolean ESPHostController::onControllerConfig(const JsonObject &msg ) {
    leds.led1().setState(false);
    leds.led2().setState(false);
    return false;
  }

//  *********** RGBLEDController methods
#ifdef QNC_COLOR_LED
void RGBLEDController::registerType() {
  QNodeItemController::getFactory()->registerType< RGBLEDController >("LED");  
}

RGBLEDController::RGBLEDController() : ColorLED(), QNodeItemController( "LED" ) {
      writeColor(visibleColor);
      setName(String(F("RGB LED Controller")));
      setUpdateInterval(getRefreshInterval());
  }

    
RGBLEDController::RGBLEDController( uint8_t pr, uint8_t pg, uint8_t pb ) : RGBLEDController() {
      setPins( pr, pg, pb);
  }

void RGBLEDController::setPins( uint8_t pr, uint8_t pg, uint8_t pb ) {
     pinR = pr;
     pinMode(pinR, OUTPUT);
     pinG = pg;
     pinMode(pinG, OUTPUT);
     pinB = pb;
     pinMode(pinB, OUTPUT);
     initialized = true;
   }

boolean RGBLEDController::onControllerConfig(const JsonObject &msg ) {
      if (msg.containsKey("RGBPins")) {
        setPins( msg["RGBPins"]["r"], msg["RGBPins"]["g"], msg["RGBPins"]["b"] );
      }
      else {
        setPins( D1, D2, D3 );
      }
      return true;
    }

void RGBLEDController::onItemCommand(const JsonObject &msg ) {
      CRGBWide newColor;    
      String st = F("RGBLEDController: Command received");
      logMessage( st );
      if (msg.containsKey("enable")) {
        if (msg["enable"] == "yes") { start(); }
        else if (msg["enable"] != "yes") { stop(); setColor(CRGBWide(0,0,0)); }
      }
      if (msg.containsKey("color")) {
        newColor = CRGBWide( msg["color"]["r"], msg["color"]["g"], msg["color"]["b"] );
        if (msg.containsKey("fade")) {
           startFade( newColor, msg["fade"] );
           onItemStateChange( String(newColor.r())+","+String(newColor.g())+","+String(newColor.b()) );
        }
        else if (msg.containsKey("flash")) {
          startFlash( newColor, msg["flash"]["duration"], msg["flash"]["repeat"], msg["flash"]["lag"] );
        }
        else {
          startFade( newColor, 10 );
          onItemStateChange( String(newColor.r())+","+String(newColor.g())+","+String(newColor.b()) );
        }
     } 
  }
    
 void RGBLEDController::onShow() {
    if (!(currentColor==visibleColor)) {
      writeColor( currentColor );
      visibleColor = currentColor;
    }
  }

void RGBLEDController::writeColor( CRGBWide color ) {
    if (initialized) {
      analogWrite( pinR, color.r() );
      analogWrite( pinG, color.g() );
      analogWrite( pinB, color.b() );
     }
  }   

void RGBLEDController::update() {
      ColorLED::update();
  }
#endif

//  *********** PIRController methods

#ifdef QNC_PIR
void PIRController::registerType() {
    QNodeItemController::getFactory()->registerType< PIRController >("PIR");
}

PIRController::PIRController() : LatchingBinarySensor(), QNodeItemController("PIR") {
      setName( String(F("PIR Sensor Controller")) );
  }

void PIRController::onSensorStateChange(boolean newState) {
        onItemStateChange( "motion", String(newState ? "on" : "off") );
        if (!newState) { onItemStateDetail( "timeout", String(0) ); }
  }

void PIRController::onSensorLatchCountdown( unsigned long msRemaining ) {
      onItemStateDetail( "timeout", String(msRemaining/1000UL) );
    }

boolean PIRController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("pirpin")) {
          LatchingBinarySensor::stop();
          setPin( msg["pirpin"] );
          String st = F("  Motion sensor reading on pin: "); 
          logMessage( st + String(getPin()) );
          LatchingBinarySensor::start();
        }
        if (msg.containsKey("timeout")) {
          setLatch( msg["timeout"] );
        }
        trigger(false);
        return true;
  }  

void PIRController::onItemCommand( const JsonObject& message ) {
      if (message.containsKey("overridemotion")) {
        String val = message["overridemotion"];
        if (val=="none") { setOverride( OverrideState::OVERRIDE_NONE ); }
        if (val=="on") { setOverride( OverrideState::OVERRIDE_ON ); }
        if (val=="off") { setOverride( OverrideState::OVERRIDE_OFF ); }
      }
      if (message.containsKey("trigger")) {
        if (message.containsKey("temp_disable")) {
          LatchingBinarySensor::trigger( message["trigger"]=="on", message["temp_disable"].as<unsigned long>() );  
        }
        else {
          LatchingBinarySensor::trigger( message["trigger"]=="on" );
        }
      }
      if (message.containsKey("timeout")) {
        unsigned long toval = message["timeout"];
        logMessage("Timeout command setting to:"+String(toval));
        setLatch( toval );
      }
      if (message.containsKey("curr_timeout")) {
        unsigned long toval = message["curr_timeout"];
        LatchingBinarySensor::setCurrLatch( toval );
      }
    }

void PIRController::onSensorEvent( EventType event ) {
    String stEvent;
    switch (event) {
      case SENS_LATCH   : { stEvent = F("Sensor Latch"); 
                            onItemStateDetail( "source", "sensor" );
                            break; 
                          }
      case SENS_RELATCH : { stEvent = F("Sensor Re-latch"); 
                            onItemStateDetail( "source", "sensor" );                           
                            break; 
                          }
      case SENS_ON      : { stEvent = F("Sensor On"); 
                            onItemStateDetail( "source", "sensor" );
                            break; 
                          }
      case SENS_OFF     : { stEvent = F("Sensor Off"); 
                            break; 
                          }
      case EXT_LATCH    : { stEvent = F("External Latch"); 
                            onItemStateDetail( "source", "external" );
                            break; 
                          }
      case EXT_RELATCH  : { stEvent = F("External Re-Latch"); 
                            onItemStateDetail( "source", "external" );
                            break; 
                          }
      case EXT_UNLATCH  : { stEvent = F("External Unlatch"); 
                            onItemStateDetail( "source", "external" );                            
                            break; 
                          }
      case OVR_ON       : { stEvent = F("Override On");  
                            onItemStateDetail( "override", "on" ); 
                            onItemStateDetail( "source", "external" );                            
                            break; 
                          }
      case OVR_OFF      : { stEvent = F("Override Off");  
                            onItemStateDetail( "override", "off" ); 
                            onItemStateDetail( "source", "external" );                            
                            break; 
                          }
      case OVR_NONE     : { stEvent = F("Override Neutral"); 
                            onItemStateDetail( "override", "none" ); 
                            onItemStateDetail( "source", "external" );
                            break; 
                          }
      case UNLATCH      : { stEvent = F("Latch Timeout");  
                            break; 
                          }
      case SET_TIMEOUT  : { stEvent = F("Set Timeout"); 
                            onItemStateDetail( "timeout", String( getLatch()/1000UL) );
                            break; 
                          }
      case SET_ACTIVE_TIMEOUT  : 
                          { stEvent = F("Set Active Timeout"); 
                            onItemStateDetail( "timeout", String(getLatch()/1000UL) );
                            break; 
                          }                          
                          
    }
    onItemEvent( stEvent );
 } 

void PIRController::update() {
      LatchingBinarySensor::update();
    }
#endif

//  *********** LDRController methods

#ifdef QNC_LDR
void LDRController::registerType() {
  QNodeItemController::getFactory()->registerType< LDRController >("LDR");
}

LDRController::LDRController() :  AnalogSensor(), QNodeItemController("LDR") {
      setName( String(F("LDR Sensor Controller")) );
      setInverted(true);
      setThreshold(10);
  }

void LDRController::onSensorChange(uint16_t newState) {
      this->onItemStateChange( "light_value", String(newState) );
      float percent = (newState/1023.0F)*100.0F;
      this->onItemStateDetail( "percent", String( percent ) );
  }
  

boolean LDRController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("inverted")) {
          setInverted( msg["inverted"]=="yes" );
          String st = F("LDR sensor value inverted: ");
          logMessage( st + (getInverted() ? "yes" : "no") );
        }
        if (msg.containsKey("ldrpin")) {
          AnalogSensor::stop();
          setPin( msg["ldrpin"] );
          String st = F("  LDR sensor reading on pin: ");
          logMessage( st  + String(getPin()) );
          setUpdateInterval(1000);
          AnalogSensor::setReadInterval(5000);
          AnalogSensor::start();
        }
        return true;
  }  

void LDRController::update() {
      AnalogSensor::read();
  }  
#endif

#ifdef QNC_VOLT
void VSensorController::registerType() {
  QNodeItemController::getFactory()->registerType< VSensorController >("VOLTAGE");
}

VSensorController::VSensorController() :  AnalogSensor(), QNodeItemController("VOLTAGE") {
      setName( String(F("Voltage Sensor Controller")) );
      setThreshold(1);
      this->setPublishFormat(StatePubLevel::PUB_STATE,PublishFormat::PUB_TEXT);
      this->setPublishFormat(StatePubLevel::PUB_STATE,PublishFormat::PUB_TEXT);      
  }

void VSensorController::onSensorChange(uint16_t newState) {
      this->onItemStateChange( "voltage", String( convertVoltage(newState*1.0)) );
      this->onItemStateDetail( "raw_pin_state", String(newState));
  }
  

boolean VSensorController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("pin")) {
          AnalogSensor::stop();
          setPin( msg["pin"] );
          String st = F("  Voltage sensor reading on pin: ");
          logMessage( st  + String(getPin()) );
          setUpdateInterval(1000);
          AnalogSensor::setReadInterval(1000);
          AnalogSensor::start();
        }
        if (msg.containsKey("zero_offset")) {
          zeroOffset = msg["zero_offset"];
          String st = F("  Voltage sensor calibration offset: ");
          logMessage( st  + String(zeroOffset) );
        }
        if (msg.containsKey("multiplier")) {
          voltageMultiplier = msg["multiplier"];
          String st = F("  Voltage sensor multiplier: ");
          logMessage( st  + String(voltageMultiplier) );
        }
        return true;
  }  

void VSensorController::update() {
      AnalogSensor::read();
  }  
#endif

//  *********** OASController methods    

#ifdef QNC_OAS
void OASController::registerType() {
  QNodeItemController::getFactory()->registerType< OASController >("OAS");
}

OASController::OASController() : BinarySensor(), QNodeItemController("OAS") {
      setName( String(F("OAS Sensor Controller")) );
      setTarget(LOW);
    }

void OASController::onSensorStateChange(boolean newState) {      
      onItemStateChange( "obstacle_detected", String(newState ? "on" : "off") );
  }

boolean OASController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("oaspin")) {
          BinarySensor::stop();
          setPin( msg["oaspin"] );
          String st = F("  Obstacle sensor reading on pin: ");
          logMessage( st + String(getPin()) );
          BinarySensor::setReadInterval(500);
          setUpdateInterval(500);
          BinarySensor::start();
        }
        return true;
  }

void OASController::onItemCommand( const JsonObject& message ) {
  }

void OASController::update()  {
    boolean newState = getState();
    if (oasState!= newState) {
      onSensorStateChange( newState );
      oasState = newState;
    }
  }
#endif

//  *********** DHTController methods    

#ifdef QNC_DHT
void DHTController::registerType() {
  QNodeItemController::getFactory()->registerType< DHTController >("DHT");
}

DHTController::DHTController() :  DHTSensor(), QNodeItemController("DHT") {
      setName( String(F("DHT Sensor Controller")) );
  }

void DHTController::onSensorTempChange(const float newTemp) {
      this->onItemStateDetail( "temperature", String(newTemp) );    
  }

void DHTController::onSensorHumidityChange(const float newHumidity) {
      this->onItemStateDetail( "humidity", String(newHumidity) );
  }

boolean DHTController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("dhtpin")) {
          DHTSensor::stop();
          setPin( msg["dhtpin"] );
          if (msg["dhttype"]) { setDHTType( msg["dhttype"] ); } else { setDHTType( DHTesp::DHT22 ); }
          String st = String(F("  DHT sensor reading on pin: "));
          logMessage( st + String(getPin()) );
          setUpdateInterval(30000);
          DHTSensor::setReadInterval(10000);
          DHTSensor::start();
        }
        return true;
  }

void DHTController::update() {
    read();
  }
#endif

//  *********** RelayController methods    

#ifdef QNC_RELAY  
void RelayController::registerType() {
  QNodeItemController::getFactory()->registerType< RelayController >("RLY");
}

RelayController::RelayController() : QNodeItemController("RLY") {
      setName( String(F("Relay Controller")) );
      pin = 0;
      relayState = false;      
    }

void RelayController::setRelay( boolean value ) {
    if (relayState != value) {
      relayState = value;
      digitalWrite( pin, relayState ? onValue : offValue );
      onItemStateChange( "relay", value ? "ON" : "OFF" );
    }
  }

boolean RelayController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("rlypin")) {
          pin =  msg["rlypin"];
          pinMode( pin, OUTPUT );
          String st = String(F("  relay writing on pin: "));
          logMessage( st + String(pin) );
          digitalWrite( pin, relayState ? onValue : offValue );
        }
        return true;
  }

void RelayController::onItemCommand( const JsonObject& message ) {
    if (message.containsKey("relay")) {
      setRelay( (message["relay"]=="on") || (message["relay"]=="ON") );      
    }
    if (message.containsKey("timeout")) {      
      relayTimer.setInterval(message["timeout"]);
      relayTimer.start();
    }
  }

void RelayController::update() {
    if (relayTimer.isUp()) {
      setRelay( !relayState );
      relayTimer.stop();
    }
  }
#endif

//  *********** MochaX10Controller methods   

#ifdef QNC_MOCHA_X10
void MochaX10Controller::registerType() {
  QNodeItemController::getFactory()->registerType< MochaX10Controller >("X10");  
}

MochaX10Controller::MochaX10Controller() : QNodeItemController("X10") {
      setName( String(F("X10 Mocha Interface Controller")) );
      setUpdateInterval( 1500 );
  }

String MochaX10Controller::sendX10Command( String cmd ) {
    String result = "";
    unsigned long timeout;
    WiFiClient client;
    if (client.connect( server, port )) {
      logMessage("X10 Controller:  Sending command to " + server + ":" + cmd );
      client.print( String(cmd+"\r\n").c_str() );
      
      timeout = GET_TIME_MILLIS_ABS+5000;
      logMessage("Waiting for response...");
      while (GET_TIME_MILLIS_ABS <= timeout && (client.available() || client.connected())) {
        if (client.available()) {  
            while (client.available()) {
              result += ((char)client.read());        
            }
            client.stop();
        }  
      }
      if (GET_TIME_MILLIS_ABS>=timeout) { 
        result = "Server timed out..."; 
      }
      if (client.connected()) { 
        client.stop(); 
      }
    }
    else {
      result = "Couldn't connect to "+server+" on port " + String(port);
    }
    logMessage(result);
    return result;
  }

boolean MochaX10Controller::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("server")) {
          server =  msg["server"].as<String>();
          String st = String(F("  server: "));
          logMessage( st + server );
        }
        if (msg.containsKey("port")) {
          port =  msg["port"];
          String st = String(F("  port: "));
          logMessage( st + String(port) );
        }
        return true;
  }

void MochaX10Controller::onItemCommand( const JsonObject& message ) {
    if (message.containsKey("command")) {
      uint8_t repeat = 1;
      if (message.containsKey("repeat")) { repeat = message["repeat"]; }
      for (uint8_t i=repeat; i > 0; i--) {
        commands.push_back(message["command"]);   
        onItemEvent( "CommandAdded: "+String(commands.size()) + " commands queued" );     
      }
    }
  }    

void MochaX10Controller::update() {
    if (commands.size() > 0) {
      String cmd = commands.front();
      String result;
      result = sendX10Command( cmd );
      onItemStateChange("LastResult", result );
      commands.erase(commands.begin()); 
    }
  }
#endif

//  *********** TPLinkController methods   

#ifdef QNC_TPLINK

uint8_t TPLinkController::initKey = 171;
uint8_t TPLinkController::buffer[2048];

void TPLinkController::registerType() {
  QNodeItemController::getFactory()->registerType< TPLinkController >("TPLINK");  
}

TPLinkController::TPLinkController() : QNodeItemController("TPLINK") {
      setName( String(F("TPLink Switch Controller")) );
      setUpdateInterval( 5000 );
  }

void TPLinkController::setState( boolean newState ) {
  if (newState != switchState) {
    switchState = newState;
    this->onItemStateChange( "switch", String((switchState ? "on" : "off")) );
  }
}

String TPLinkController::sendCommand( const String &cmd ) {
    String result = "";
    unsigned long timeout;
    WiFiClient client;
    String command;
    uint16_t bytesRead;
    
    if (cmd.equalsIgnoreCase("on")) { command = F("{\"system\":{\"set_relay_state\":{\"state\":1}}}"); }
    else if (cmd.equalsIgnoreCase("off")) { command = F("{\"system\":{\"set_relay_state\":{\"state\":0}}}"); }
    else { command = F("{\"system\":{\"get_sysinfo\":{}}}"); }
    // logMessage("TPLink Controller:  Connecting to " + host + ":" + cmd );
    if (client.connect( host, port )) {
      client.setSync(true);
      // logMessage("Message:  " + command);  
      // logMessage("Encrypting.");
      encrypt( buffer, command );
      client.write(buffer, static_cast<size_t>(command.length()+4u) );
      // logMessage("Sent: " + command );
      // logMessage("Waiting for response...");
      timeout = GET_TIME_MILLIS_ABS+5000;
      while ((GET_TIME_MILLIS_ABS <= timeout) && (client.available()==0)) { 
        delay(5);
      }
      bytesRead = 0;
      uint32_t expected = 4;
      timeout = GET_TIME_MILLIS_ABS+5000;
      while ((GET_TIME_MILLIS_ABS <= timeout) && (bytesRead<expected)) {
          if (client.available()) {
            buffer[bytesRead] = static_cast<uint8_t>(client.read()); 
            bytesRead++;   
            if (bytesRead==4) {
              expected = 4 + ((uint32_t)buffer[0] << 24 | (uint32_t)buffer[1] << 16 | (uint32_t)buffer[2] << 8 | (uint32_t)buffer[3]);
            }
          }
          else {
            delay(1);
          }
      }            
      //logMessage("Read " + String(bytesRead) + " bytes, expected " + String(expected) + " bytes."); 
      if (bytesRead==expected) {
        result = decrypt(buffer);
        if (result.length()>0) {
          // logMessage("Decrypted: " + result + "<end>");
          DynamicJsonDocument doc(JSON_BUFFER_SIZE);
          auto error = deserializeJson( doc, result );
          if (!error) {
            if (cmd.equalsIgnoreCase("status")) {            
              bool current = doc["system"]["get_sysinfo"]["relay_state"].as<String>()=="1";
              if (current != switchState) {
                if (current) { onItemEvent("State update: on"); ; }
                else { onItemEvent("State update off"); }
              }
              setState(current);
              // logMessage( "State:  " + String(switchState) );
            }
            else {
              bool target = cmd.equalsIgnoreCase("on");
              if (doc["system"]["set_relay_state"]["err_code"].as<String>()=="0") {
                if (target) { onItemEvent("Command sent: on" ); } 
                else { onItemEvent("Command sent: off" ); }                                 
                setState( target );
              }
              else {
                onItemEvent("Command error" , "\"code\":" + doc["system"]["set_relay_state"]["err_code"].as<String>() );
              }
            }
          }
        }
        logMessage( getDescription() + " : " + cmd + " - relay status from host " + host + " : "  + static_cast<String>(switchState ? "ON" : "OFF") );                
      }
      else {
        result = "Timed-out waiting for response:  " + String(bytesRead) +" of " + String(expected) + " bytes read.";
        logMessage(result);
      }
      if (client.connected()) { 
        client.stop(); 
      }            
    }
    else {
      result = "Couldn't connect to "+host+" on port " + String(port);
      logMessage(result);
    }
  return result;
}

boolean TPLinkController::onControllerConfig( const JsonObject &msg ) {
        if (msg.containsKey("host")) {
          host =  msg["host"].as<String>();
          String st = String(F("  host: "));
          logMessage( st + host );
        }
        if (msg.containsKey("port")) {
          port =  msg["port"];
          String st = String(F("  port: "));
          logMessage( st + String(port) );
        }
        return true;
  }

void TPLinkController::onItemCommand( const JsonObject& message ) {
    if (message.containsKey("command")) {
      String cmd = message["command"];
      logMessage( "TPLinkController processing message: " + cmd );
      if (cmd.equalsIgnoreCase("on")) { sendCommand( "ON" ); }
      else if (cmd.equalsIgnoreCase("off")) { sendCommand( "OFF" ); }
    }
  }    

void TPLinkController::update() {
    sendCommand("STATUS");    
  }

String TPLinkController::decrypt( uint8_t* input) {
   String result = "";
   uint32_t size = (uint32_t)input[0] << 24 | (uint32_t)input[1] << 16 | (uint32_t)input[2] << 8 | (uint32_t)input[3];   
   if (input != nullptr && size > 0) {     
     uint8_t xorkey = initKey;
     for (uint16_t i = 4; i < (uint16_t)size+4; i++) {
       uint8_t b = (uint8_t)(xorkey ^ input[i] );
       xorkey = input[i];
       result += char(b);
     }
   }
  return result;
}  

uint8_t* TPLinkController::encrypt( uint8_t* output, const String &input ) {
  uint32_t size = input.length();
  if (size > 0) {
     output[0] = (uint8_t)(size >> 24);
     output[1] = (uint8_t)(size >> 16);
     output[2] = (uint8_t)(size >> 8);
     output[3] = (uint8_t)(size);
     uint8_t xorkey = initKey;
     for (uint16_t i = 0; i < ((uint16_t)size); i++) {
       uint8_t b = (uint8_t) ( xorkey ^ (uint8_t)input[i] );
       xorkey = b;
       output[i+4] = b;
     }
  }
  return output;
}
#endif

//  *********** QFXController methods   

#ifdef QNC_LEDSTRIP
void QFXController::registerType() {
  QNodeItemController::getFactory()->registerType< QFXController >("LEDStrip");
}

QFXController::QFXController() : QNodeItemController( "LEDStrip" ), FFXController()  {
      setName(String(F("Led StripFX Controller")));
      setUnthrottled( true );             // No timer used for calls to update() - FXController handles necessary timing to send LED commands
      leds = nullptr;
    }
    
void QFXController::onLongOpEnd( unsigned long timeTaken ) {
      // attempt to minimize visual impact of long operations that may delay timing...      
      //if (getFX()) { getFX()->addDelta( timeTaken ); }
      //if (getOverlayFX()) { getOverlayFX()->addDelta( timeTaken ); }
  }
    
boolean QFXController::onControllerConfig( const JsonObject &msg ) {
        configured = false;
        pixels = msg["pixels"];
        if (pixels == 0) { 
          pixels = 100; 
        }
        if (leds) { 
          delete [] leds; 
          leds = nullptr; 
        }
        logMessage(QNodeController::LOGLEVEL_INFO, "Allocating LED buffer: " + String(ESP.getFreeHeap()) + " available heap.  Leds: " + String(pixels) );
        leds = new CRGB[pixels]; 
        
        fill_solid( leds, pixels, CRGB::Black );
        logMessage(QNodeController::LOGLEVEL_INFO, "LED Buffer allocated: " + String(ESP.getFreeHeap()) + " available heap.  Leds:  " + String(pixels) );
        if (msg["colororder"] == "brg") { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, BRG>(leds, pixels).setCorrection(TypicalLEDStrip ); 
        }
        else if (msg["colororder"] == "grb") { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, GRB>(leds, pixels).setCorrection(TypicalLEDStrip); 
        }
        else if (msg["colororder"] == "rbg") { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, RBG>(leds, pixels).setCorrection(TypicalLEDStrip); 
        }
        else if (msg["colororder"] == "gbr") { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, GBR>(leds, pixels).setCorrection(TypicalLEDStrip); 
        }
        else if (msg["colororder"] == "bgr") { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, BGR>(leds, pixels).setCorrection(TypicalLEDStrip); 
        }        
        else { 
          FastLED.addLeds<CHIPSET_DEFAULT, DATA_PIN_DEFAULT, RGB>(leds, pixels).setCorrection(TypicalLEDStrip);
        }
        FastLED.setDither(DISABLE_DITHER);
        FFXController::initialize( new FFXFastLEDPixelController( leds, pixels ) );
        configured = true;        
        if (msg.containsKey("segments")) {
          logMessage("Creating Segments:  ");
          for (auto ctx : msg["segments"].as<JsonArray>()) {
            if (ctx.as<JsonObject>().containsKey("name")) {
              this->addSegment( ctx["name"], ctx["start"], ctx["end"], nullptr );      
              logMessage(QNodeController::LOGLEVEL_INFO,"LED Controller - adding segment "+ctx["name"].as<String>());
            }
          }
        } 
        QNodeItemController::start();         
        FFXController::update();
      return true;
  }

void QFXController::onFXStateChange(FFXSegment *segment) {
    markLongOpStart();
    DynamicJsonDocument doc(JSON_BUFFER_SIZE); 
    JsonObject root = doc.to<JsonObject>();   
    FFXSegment *currSeg = segment;  
    FFXBase *currEffect = currSeg->getFX();
    root["segment"] = currSeg->getTag();
    root["effect"] = currEffect->getFXName();
    root["effectid"]= currEffect->getFXID();
    root["brightness"] = currSeg->getBrightness();
    if (!currSeg->isPrimary()) { root["seg_dimming"] = (currSeg->hasDimmer() ? "on" : "off"); }
    root["pct_brightness"] = fixed_map( getBrightness(), 0, 255, 0, 100 );    
    if (currSeg != getPrimarySegment()) { root["opacity"] = currSeg->getOpacity(); }
    root["pct_speed"] = fixed_map( currEffect->getSpeed(), 0, 255, 0, 100 );
    root["speed"] = currEffect->getSpeed();
    root["interval"] = currEffect->getInterval();
    root["updated"] = currSeg->getFX()->isUpdated();
    root["fade"] = (currSeg->getFrameProvider()->getCrossFadePref() ? "on" : "off");    
    root["fade_actual"] = (currSeg->getFrameProvider()->getCrossFade() ? "on" : "off");    
    root["last_xfade_blend_amt"] = (currSeg->getFrameProvider()->getLastBlendAmount());
    root["curr_fade_steps_est"] = currSeg->getFrameProvider()->getLastBlendSteps();
    root["show_count"] = showCount;
    root["fade_up"] = FFXBase::fadeMethodStr(currSeg->getFrameProvider()->getFadeMethodUp());
    root["fade_down"] = FFXBase::fadeMethodStr(currSeg->getFrameProvider()->getFadeMethodDown());
    if (currEffect->getFXID()==CHASE_FX_ID) {
       root["chase_dot_width"] = ((ChaseFX *)currEffect)->getDotWidth();
       root["chase_dot_spacing"] = ((ChaseFX *)currEffect)->getDotSpacing();
       root["chase_blur"] = ((ChaseFX *)currEffect)->getBlurAmount();
    }
    else {
       root["chase_dot_width"] = 0;
       root["chase_dot_spacing"] = 0;
       root["chase_blur"] = 0;
    }
    root.createNestedObject("color");
    FFXColor::FFXColorMode currMode = currEffect->getFXColor().getColorMode();
    CRGB currRGB = currEffect->getFXColor().peekCRGB();
    CHSV currHSV = currEffect->getFXColor().peekCHSV();
    switch (currMode) {
      case FFXColor::FFXColorMode::singleCRGB : { root["color"]["mode"] = "rgb"; break;  }
      case FFXColor::FFXColorMode::singleCHSV : { root["color"]["mode"] = "hsv"; break;  }
      case FFXColor::FFXColorMode::palette16  : { root["color"]["mode"] = "p16"; break;  } 
      case FFXColor::FFXColorMode::palette256 : { root["color"]["mode"] = "p256"; break; } 
    }
    root["color"]["r"] = currRGB.r;
    root["color"]["g"] = currRGB.g;
    root["color"]["b"] = currRGB.b;
    root["color"]["h"] = currHSV.h;
    root["color"]["s"] = currHSV.s;
    root["color"]["v"] = currHSV.v;
    if (currMode==FFXColor::FFXColorMode::palette16 || currMode==FFXColor::FFXColorMode::palette256) {
      CRGBPalette16 &currPal = currEffect->getFXColor().getPalette();
      uint8_t range = currEffect->getFXColor().getPaletteRange();
      ((JsonObject)root["color"]).createNestedObject("palette");
      root["color"]["palette"]["size"] = range;
      root["color"]["palette"]["index"] = currEffect->getFXColor().relativePaletteIndex();
      root["color"]["palette"]["r"] = currEffect->getFXColor().peekCRGB().r;
      root["color"]["palette"]["g"] = currEffect->getFXColor().peekCRGB().g;
      root["color"]["palette"]["b"] = currEffect->getFXColor().peekCRGB().b;
      JsonArray arr = ((JsonObject)root["color"]["palette"]).createNestedArray("colors");
      for (uint8_t i=0; i<range; i++) {
        JsonObject nested = arr.createNestedObject();
        nested["r"] = currPal[i].r;
        nested["g"] = currPal[i].g;
        nested["b"] = currPal[i].b;
      }
    }
    if (segment==getPrimarySegment()) {
      onItemStateChange( root );
    }
    else {
      onItemStateDetail( currSeg->getTag(), root );
    }
    markLongOpEnd();
    //localBroadcast( "{\"color\":{\"r\":1023,\"g\":512,\"b\":0},\"flash\": {\"duration\":30,\"repeat\":1, \"lag\":0} }", "LED");
  }

void QFXController::onFXEvent( const String &segment, FXEventType event, const String &name ) {
    String stEvent = "";
      switch (event) {
        case FX_STARTED           : { stEvent = F("Effect Started"); break; }
        case FX_STOPPED           : { stEvent = F("Effect Stopped"); break; }
        case FX_OVERLAY_STARTED   : { stEvent = F("Overlay Started"); break; }
        case FX_OVERLAY_STOPPED   : { stEvent = F("Overlay Stopped"); break; }
        case FX_OVERLAY_UPDATED   : { stEvent = F("Overlay Updated"); break; }
        case FX_OVERLAY_COMPLETED : { stEvent = F("Overlay Completed"); break; }
        case FX_PAUSED            : { stEvent = F("Effect Paused"); break; }
        case FX_RESUMED           : { stEvent = F("Effect Resumed"); break; }   
        case FX_PARAM_CHANGE      : { stEvent = "Parameter changed: "+name; break; }
        case FX_BRIGHTNESS_CHANGED: { stEvent = F("Brightness Changed"); break; }
        case FX_LOG               : { QNodeItemController::logMessage( QNodeController::LOGLEVEL_INFO, "" + name); }
      }
    if (stEvent != "")   {
      onItemEvent( (segment =="" ? "" : segment + ": ") + name + " " + stEvent );
    }
  }

boolean QFXController::isEffect( String tgtFXName, int tgtFXID, String FXName, int FXID ) {
    tgtFXName.toLowerCase();
    return (tgtFXName.equals(FXName) || tgtFXID==FXID);
  }

  
void QFXController::onItemCommandElement( String context, String key, JsonVariant& value ) {
    logMessage( QNodeController::LOGLEVEL_INFO, "LEDStrip:  " + context + "." + key + " = " + String(value.as<String>().c_str()) );
  } 

void QFXController::onItemCommand( const JsonObject &msg ) {
    
    String msgStr;    
   
    serializeJson( msg, msgStr );
    logMessage(QNodeController::LOGLEVEL_DEBUG, "LEDStripController - Command Received: " + msgStr );

    if (configured) {    
      std::vector<FFXSegment *> segs = std::vector<FFXSegment *>();
      if (msg.containsKey("segments")) {
        for (auto ctx : msg["segments"].as<JsonArray>()) {
          String segName = ctx;
          FFXSegment *currSeg = findSegment(segName);
          if (currSeg){ segs.push_back(currSeg); }
        }
      }
      if (msg.containsKey("segment")) {
        String segName = msg["segment"];
        FFXSegment *currSeg = findSegment(segName);
        if (currSeg) {
          segs.push_back(currSeg);
        }
      }
      if (segs.size()==0) {
        segs.push_back( getPrimarySegment() );
      }
      for (FFXSegment *currSeg : segs) {
        logMessage(QNodeController::LOGLEVEL_DEBUG, "Applying command to segment: " + currSeg->getTag() );      
        FFXBase *currEffect = currSeg->getFX();
        if (msg.containsKey("effect") || msg.containsKey("effectid")) {
          FFXBase* newFX = nullptr;
          if (isEffect(CHASE_FX_NAME, CHASE_FX_ID, msg["effect"], msg["effectid"])) { newFX = new ChaseFX( currSeg->getLength() ); }
          if (isEffect(SOLID_FX_NAME, SOLID_FX_ID, msg["effect"], msg["effectid"])) { newFX = new SolidFX( currSeg->getLength() ); }
          if (isEffect(MOTION_FX_NAME, MOTION_FX_ID, msg["effect"], msg["effectid"])) { newFX = new MotionFX( currSeg->getLength() ); }
          if (isEffect(RAINBOW_FX_NAME, RAINBOW_FX_ID, msg["effect"], msg["effectid"])) { newFX = new RainbowFX( currSeg->getLength() ); }
          if (isEffect(JUGGLE_FX_NAME, JUGGLE_FX_ID, msg["effect"], msg["effectid"])) { newFX = new JuggleFX( currSeg->getLength() ); }
          if (isEffect(CYLON_FX_NAME, CYLON_FX_ID, msg["effect"], msg["effectid"])) { newFX = new CylonFX( currSeg->getLength() ); }
          if (isEffect(CYCLE_FX_NAME, CYCLE_FX_ID, msg["effect"], msg["effectid"])) { newFX = new CycleFX( currSeg->getLength() ); }
          if (isEffect(TWINKLE_FX_NAME, TWINKLE_FX_ID, msg["effect"], msg["effectid"])) { newFX = new TwinkleFX( currSeg->getLength() ); }
          if (isEffect(DIM_PAL_FX_NAME, DIM_PAL_FX_ID, msg["effect"], msg["effectid"])) { newFX = new DimUsingPaletteFX( currSeg->getLength() ); }
          if (isEffect(PACIFICA_FX_NAME, PACIFICA_FX_ID, msg["effect"], msg["effectid"])) { newFX = new PacificaFX( currSeg->getLength() ); }
          if (isEffect(PALETTE_FX_NAME, PALETTE_FX_ID, msg["effect"], msg["effectid"])) { newFX = new PaletteFX( currSeg->getLength() ); }
          if (newFX) {
            if (getFX()) {
              newFX->setSpeed(currEffect->getSpeed());
            }
            currSeg->setFX(newFX);
            currEffect = currSeg->getFX();
          }          
        }
        if (msg.containsKey("opacity")) { currSeg->setOpacity(msg["opacity"].as<uint8_t>()); }     
        if (msg.containsKey("seg_dimming")) { if (msg["seg_dimming"]=="off") { currSeg->removeDimmer(); } }   
        if (msg.containsKey("brightness")) { currSeg->setBrightness( msg["brightness"] ); }
        if (msg.containsKey("pct_brightness")) { currSeg->setBrightness( fixed_map( msg["pct_brightness"].as<uint8_t>(), 0, 100, 0, 255 ) ); }
        if (msg.containsKey("interval")) { currEffect->setInterval( msg["interval"].as<unsigned long>() ); }
        if (msg.containsKey("speed")) { currEffect->setSpeed(msg["speed"]); }
        if (msg.containsKey("pct_speed")) { currEffect->setSpeed( fixed_map( msg["pct_speed"].as<uint8_t>(), 0, 100, 0, 255 ) ); }
        if (msg.containsKey("movement")) {
          if (msg["movement"]=="forward") { currEffect->setMovement( FFXBase::MovementType::MVT_FORWARD ); }
          else if (msg["movement"]=="backward") { currEffect->setMovement( FFXBase::MovementType::MVT_BACKWARD ); }
          else if (msg["movement"]=="still") { currEffect->setMovement( FFXBase::MovementType::MVT_STILL ); }
        }
        if (msg.containsKey("fade")) { currSeg->getFrameProvider()->setCrossFadePref( msg["fade"]=="on" ); }
        if (msg.containsKey("fade_up")) { 
          currSeg->getFrameProvider()->setFadeMethod( msg["fade_up"]=="gamma" ? FFXBase::FadeType::GAMMA : (msg["fade_up"]=="cubic" ? FFXBase::FadeType::CUBIC : FFXBase::FadeType::LINEAR ), currSeg->getFrameProvider()->getFadeMethodDown());  
        }
        if (msg.containsKey("fade_down")) { 
          currSeg->getFrameProvider()->setFadeMethod( currSeg->getFrameProvider()->getFadeMethodUp(), msg["fade_down"]=="gamma" ? FFXBase::FadeType::GAMMA : (msg["fade_down"]=="cubic" ? FFXBase::FadeType::CUBIC : FFXBase::FadeType::LINEAR ));  
        }
        if (msg.containsKey("chase_dot_width") && (currEffect->getFXName()==CHASE_FX_NAME)) {
          ((ChaseFX *)currEffect)->setDotWidth( msg["chase_dot_width"] );
        }
        if (msg.containsKey("chase_dot_spacing") && (currEffect->getFXName()==CHASE_FX_NAME)) {
          ((ChaseFX *)currEffect)->setDotSpacing( msg["chase_dot_spacing"] );
        }
        if (msg.containsKey("chase_blur") && (currEffect->getFXName()==CHASE_FX_NAME)) {
          ((ChaseFX *)currEffect)->setBlurAmount( msg["chase_blur"] );
        }
        if (msg.containsKey("chase_shift") && (currEffect->getFXName()==CHASE_FX_NAME)) {
          ((ChaseFX *)currEffect)->setShift( msg["chase_shift"]=="true" || msg["chase_shift"]=="yes" );
        }
        if (msg.containsKey("motion_range") && (currEffect->getFXName()==MOTION_FX_NAME)) {
          ((MotionFX *)currEffect)->setNormalizationRange( msg["motion_range"] );
        }
        if (msg.containsKey("color")) {
          String cmode = msg["color"]["mode"].as<String>();
          if (cmode != "") {
            if (cmode.equals("p16")) { currEffect->getFXColor().setColorMode(FFXColor::FFXColorMode::palette16);  }
            else if (cmode.equals("p256")) { currEffect->getFXColor().setColorMode(FFXColor::FFXColorMode::palette256);   }
            else if (cmode.equals("rgb")) { currEffect->getFXColor().setColorMode(FFXColor::FFXColorMode::singleCRGB);   }
            else if (cmode.equals("hsv")) { currEffect->getFXColor().setColorMode(FFXColor::FFXColorMode::singleCHSV);   }
          }
          uint8_t ccolors = msg["color"]["colors"];
          if (currEffect->getFXColor().getColorMode()==FFXColor::FFXColorMode::singleCRGB) {
              CRGB newColor = currEffect->getFXColor().getCRGB();
              newColor.r = msg["color"]["r"] | newColor.r; 
              newColor.g = msg["color"]["g"] | newColor.g; 
              newColor.b = msg["color"]["b"] | newColor.b; 
              currEffect->getFXColor().setCRGB( newColor );
          }
          else if (currEffect->getFXColor().getColorMode()==FFXColor::FFXColorMode::singleCHSV) {
            CHSV newColor = currEffect->getFXColor().getCHSV();
            newColor.h = msg["color"]["h"] | newColor.h;
            newColor.s = msg["color"]["s"] | newColor.s;
            newColor.v = msg["color"]["v"] | newColor.v;
            currEffect->getFXColor().setCHSV( newColor );
          }
          else {
            String cpal =  msg["color"]["palette"];
            if (cpal.equals("rainbow")) { currEffect->getFXColor().setPalette( RainbowColors_p, ccolors ); }
            else if (cpal.equals("multi")) { currEffect->getFXColor().setPalette( ::Multi_p, ::Multi_size ); }
            else if (cpal.equals("rwb")) { currEffect->getFXColor().setPalette( ::rwb_p, ::rwb_size ); }
            else { currEffect->getFXColor().setPalette(  NamedPalettes::getInstance()[cpal]); }
          }
          if ( ccolors != 0) { currEffect->getFXColor().setPaletteRange( ccolors ); }
        }
        if (msg.containsKey("report")) { onFXStateChange(currSeg); }
      
        if (msg.containsKey("flash")) {
          String pcolor = msg["flash"]["color"];
          const CRGBPalette16& pal =  NamedPalettes::getInstance()[pcolor];  
          uint8_t cycles = msg["flash"]["repeat"];
          uint8_t ovlspeed = 220;
          uint8_t alpha = 255;
          unsigned long lag = 0;
          if (msg["flash"]["max"]) { 
              alpha = msg["flash"]["max"]; 
          }
          lag = msg["flash"]["lag"];
          if (msg["flash"]["speed"]) ovlspeed = msg["flash"]["speed"];
          if (cycles==0) { cycles=1; }
          if (msg["flash"]["effect"] == "wave") {
            FFXOverlay *newFX;
            if (msg["flash"]["direction"]=="backward") {
              newFX = new WaveOverlayFX(currSeg->getLength(), ovlspeed, cycles, FFXBase::MovementType::MVT_BACKWARD );
            } else if (msg["flash"]["direction"]=="backandforth") {
              newFX = new WaveOverlayFX(currSeg->getLength(), ovlspeed, cycles, FFXBase::MovementType::MVT_BACKFORTH );
            } else {
              newFX = new WaveOverlayFX(currSeg->getLength(), ovlspeed, cycles );
            }
            newFX->getFXColor().setPalette( pal );
            if (alpha != 255) { newFX->setMaxAlpha(alpha); }
            if (lag) { newFX->setLag( lag ); }
            setOverlayFX( newFX );
          }
          if ((msg["flash"]["effect"] == "pulse") || (msg["flash"]["effect"] == "zip")) {
            FFXOverlay *newFX;
            if (msg["flash"]["effect"] == "pulse") { newFX = new PulseOverlayFX( currSeg->getLength(), ovlspeed, cycles ); }
            else { newFX = new ZipOverlayFX( currSeg->getLength(), ovlspeed, cycles ); }
            newFX->getFXColor().setPalette( pal );
            if (alpha != 255) { newFX->setMaxAlpha(alpha); }
            if (msg["flash"]["direction"]=="backward") { newFX->setMovement(FFXBase::MovementType::MVT_BACKWARD);}
            if (msg["flash"]["direction"]=="backforth") { newFX->setMovement(FFXBase::MovementType::MVT_BACKFORTH); }
            if (msg["flash"].as<JsonObject>().containsKey("start"))  { 
              uint16_t lo = msg["flash"]["start"];
              uint16_t hi = msg["flash"]["end"];
              ((PulseOverlayFX *)newFX)->setPixelRange( lo, hi ); 
            }
            if (lag) { newFX->setLag( lag ); }
            currSeg->setOverlay( newFX );
          }
        }      
      }
      if (msg.containsKey("temperature")) {
        if (msg["temperature"]==F("Candle")) { FastLED.setTemperature(Candle); }
        if (msg["temperature"]==F("Tungsten40W")) { FastLED.setTemperature(Tungsten40W); }
        if (msg["temperature"]==F("Tungsten100W")) { FastLED.setTemperature(Tungsten100W); }
        if (msg["temperature"]==F("Halogen")) { FastLED.setTemperature(Halogen); }
        if (msg["temperature"]==F("CarbonArc")) { FastLED.setTemperature(CarbonArc); }
        if (msg["temperature"]==F("DirectSunlight")) { FastLED.setTemperature(DirectSunlight); }
        if (msg["temperature"]==F("OvercastSky")) { FastLED.setTemperature(OvercastSky); }
        if (msg["temperature"]==F("ClearBlueSky")) { FastLED.setTemperature(ClearBlueSky); }
      } 
      if (msg.containsKey("correction")) {
        FastLED.setCorrection( CRGB(msg["correction"]["r"],msg["correction"]["g"],msg["correction"]["b"]) );
      }
    }
}

void QFXController::update() {
    if (configured) {
      FFXController::update();
    }
  }
  
#endif
