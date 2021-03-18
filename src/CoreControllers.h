#ifndef CORE_CONTROLLERS_H
#define CORE_CONTROLLERS_H

#define QNC_COLOR_LED           // Include support for 3 pin connected RGB LED 
#define QNC_PIR                 // Include support for single pin infrared motion sensor
#define QNC_LDR                 // Include support for single pin light sensor (analog)
#define QNC_VOLT                // Include support for voltage sensor (analog)
#define QNC_DHT                 // Include support for single pin DHT temp/humidity sensor (using DHT Sensor lib)
#define QNC_RELAY               // Include support for single pin relay 
#define QNC_OAS                 // Include support for single pin Obstacle Avoidance Sensor 
#define QNC_MOCHA_X10           // Include support for sending X10 commands to MocahD server daemon
#define QNC_TPLINK              // Include support for TPLink HS### Switches
#define QNC_LEDSTRIP            // Include support for controlling LED Strip/pixels via FastFX library

#include "QNodeItemController.h"
#include "LED/MonochromeLED.h"

#ifdef QNC_COLOR_LED
#include "LED/ColorLED.h"
#endif

#ifdef QNC_PIR
#include "Sensors/LatchingBinarySensor.h"
#endif

#ifdef QNC_DHT
#include "Sensors/DHTSensor.h"
#endif

#ifdef QNC_LEDSTRIP
#include "FFXController.h"

// #define LED_BUILTIN_TWO     16
#define NUM_LEDS_DEFAULT    100
#define DATA_PIN_DEFAULT    5
#define CHIPSET_DEFAULT     WS2811
#define COLOR_ORDER_DEFAULT BRG
#endif

class CoreControllers {
  public:
    static void registerControllers();
  private:
    CoreControllers();
    virtual ~CoreControllers();
};

class ESPLEDs {
  public:
    MonoVariableLED &led1() { return ledBuiltin1; }
    MonoVariableLED &led2() { return ledBuiltin2; }
    ESPLEDs() { }
  protected:
    MonoVariableLED ledBuiltin1 = MonoVariableLED( LED_BUILTIN, true, false );
    MonoVariableLED ledBuiltin2 = MonoVariableLED( LED_BUILTIN_AUX, true, false );
  }; // class ESPLEDs

class ESPHostController : public QNodeItemController {
  public:
    static void registerType();
    ESPHostController();
    ESPLEDs &getLeds() { return leds; }   
    // Overrides from QNodeItemController & QNodeItem
    virtual void onItemCommandElement( String context, String key, JsonVariant& value ) override ;   
    virtual boolean onControllerConfig(const JsonObject &msg ) override;
  private:
    ESPLEDs leds = ESPLEDs();
    void updateFirmware(const String &url );
};  // class ESPHostController


#ifdef QNC_COLOR_LED
class RGBLEDController : public ColorLED, public QNodeItemController {
  public:
    static void registerType();
    RGBLEDController();
    RGBLEDController( uint8_t pr, uint8_t pg, uint8_t pb );
    // Overrides from ColorLED
    virtual void setPins( uint8_t pr, uint8_t pg, uint8_t pb );
    virtual void onShow() override;    
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig(const JsonObject &msg ) override  ;
    virtual void onItemCommand(const JsonObject &msg ) override ;    
    virtual void update() override;

  private:
    CRGBWide visibleColor = CRGBWide(0, 0, 0);
    void writeColor( CRGBWide color );
    uint8_t pinR = 0;
    uint8_t pinG = 0;
    uint8_t pinB = 0;
    boolean initialized = false;
};  // class RGBLedController
#endif

#ifdef QNC_PIR
class PIRController : public LatchingBinarySensor, public QNodeItemController {
  public:
    static void registerType();
    PIRController();
    // Overrides from LatchingBinarySensor
    virtual void onSensorStateChange(boolean newState) override;
    virtual void onSensorLatchCountdown( unsigned long msRemaining ) override;
    virtual void onSensorEvent( EventType event ) override;
    // Overides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommand( const JsonObject& message );
    virtual void update() override;    
};  // class PIRController
#endif

#ifdef QNC_OAS
class OASController : public BinarySensor, public QNodeItemController {
  public:
    static void registerType();
    OASController();
    // Overrides from BinarySensor
    virtual void onSensorStateChange(boolean newState);
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommand( const JsonObject& message );
    virtual void update();
  private:
    boolean oasState = false;
};  // class OASController
#endif 

#ifdef QNC_LDR
class LDRController : public AnalogSensor, public QNodeItemController {
  public:
    static void registerType();
    LDRController();
    // Overrides from AnalogSensor
    void onSensorChange(uint16_t newState) override;
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void update() override;
};  // class LDRController
#endif

#ifdef QNC_VOLT
class VSensorController : public AnalogSensor, public QNodeItemController {
  public:
    static void registerType();
    VSensorController();
    // Overrides from AnalogSensor
    void onSensorChange(uint16_t newState) override;
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void update() override;
    float convertVoltage(float rawValue) { return ((rawValue+(float)zeroOffset)/1024.0)*baseMultiplier*voltageMultiplier; }
  private:
     float baseMultiplier = 3.3;
     float voltageMultiplier = 0.0;
     int16_t zeroOffset = 0;  
};  // class VSensorController
#endif

#ifdef QNC_DHT
class DHTController : public DHTSensor,  public QNodeItemController {
  public:
    static void registerType();
    DHTController();    
    // Overrides from DHTSensor
    void onSensorTempChange(const float newTemp) override;
    void onSensorHumidityChange(const float newHumidity) override;
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void update() override;
};  // class DHTController
#endif

#ifdef QNC_RELAY
class RelayController : public QNodeItemController {
  public:
    static void registerType();
    RelayController();
    void setRelay( boolean value );
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommand( const JsonObject& message ) override;
    virtual void update() override;
  private:
    boolean relayState = false;
    uint8_t onValue = HIGH;
    uint8_t offValue = LOW;
    // relayTimer implements timeout for relay - can set LOW or HIGH for n milliseconds by passing "timeout":n in command
    StepTimer relayTimer = StepTimer( 5000, false );
    uint8_t pin = 0;    
};  // class RelayController
#endif

#ifdef QNC_MOCHA_X10
class MochaX10Controller : public QNodeItemController {
  public:
    static void registerType();
    MochaX10Controller();
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommand( const JsonObject& message );
    virtual void update() override;  
  private:
    String server;
    uint16_t port;
    std::vector<String> commands;   
    String sendX10Command( String cmd );
};  // class MochaX10Controller
#endif

#ifdef QNC_TPLINK
class TPLinkController : public QNodeItemController {
  public:
    static void registerType();
    TPLinkController();
    // Overrides from QNodeItemController & QNodeItem
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommand( const JsonObject& message );
    virtual void update() override;  
  private:
    String host;
    uint16_t port = 9999;
    static uint8_t initKey;
    bool switchState = false;
    static uint8_t buffer[];

    static String decrypt( uint8_t* input );
    static uint8_t* encrypt( uint8_t* output, const String &input );
    void setState( boolean newState );  
    String sendCommand( const String &cmd );
};  // class MochaX10Controller
#endif

#ifdef QNC_LEDSTRIP
class QFXController : public QNodeItemController, public FFXController {
  public:
    static void registerType();
    QFXController();
    boolean isEffect( String tgtFXName, int tgtFXID, String FXName, int FXID );
    virtual void onFXStateChange(FFXSegment *segment) override;
    // Overrides from FXController
    virtual void onFXEvent( const String &segment, FXEventType event, const String &name ) override;
     // Overrides from QNodeItemController & QNodeItem
    virtual void onLongOpEnd( unsigned long timeTaken ) override;    
    virtual boolean onControllerConfig( const JsonObject &msg ) override;
    virtual void onItemCommandElement( String context, String key, JsonVariant& value ) override;
    virtual void onItemCommand( const JsonObject &msg ) override;
    virtual void update() override;
  private:
    boolean configured = false;
    uint16_t pixels = 0;
    uint8_t pin = D5;
    struct CRGB *leds = nullptr;    
};  // class QFXController
#endif

#endif
