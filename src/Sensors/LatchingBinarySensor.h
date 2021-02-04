#ifndef LATCHING_BINARY_SENSOR_H
#define LATCHING_BINARY_SENSOR_H

#include "Sensors.h"

class LatchingBinarySensor : public BinarySensor {
  public:
    enum OverrideState { OVERRIDE_NONE = 1, OVERRIDE_OFF = 2, OVERRIDE_ON = 3};
    enum ExtTriggerState { EXT_TRIGGER_NONE = 1, EXT_TRIGGER_OFF = 2, EXT_TRIGGER_ON  = 3};
    enum EventType { SENS_LATCH = 1, SENS_RELATCH = 2, SENS_ON = 3, SENS_OFF = 4, 
                     EXT_LATCH = 5, EXT_RELATCH = 6, EXT_UNLATCH = 7, 
                     OVR_ON = 8, OVR_OFF = 9, OVR_NONE = 10, UNLATCH = 11, SET_TIMEOUT = 12, 
                     SET_ACTIVE_TIMEOUT = 13 };

  public:
    LatchingBinarySensor() : BinarySensor() {}
    LatchingBinarySensor( uint8_t pin ): BinarySensor(pin) {};
    
    // Set the default latch timeout (in milliseconds)
    void setLatch( unsigned long newLatch );    
    unsigned long getLatch() { return defTimeout; }
        
    // setCurrLatch temporarily overrides the default latch timeout - the current value remains in effect until 
    // the state changes back to off (any re-latch during this cycle will use the override timeout value).
    void setCurrLatch( unsigned long newLatch );    
    unsigned long getCurrLatch() { return currTimeout; }
    
    virtual void onSensorStateChange( boolean newState ) {}
    virtual void onSensorLatchCountdown( unsigned long msRemaining ) {}
    virtual void onSensorEvent( EventType event ) {}
    boolean getState() { return ( calcState(sensorState, ovrState, extTriggerState ) ); }
    boolean getSensorState() { return sensorState; }
    boolean getLatchState() { return latchTimer.isStarted(); }

    OverrideState getOverride() { return ovrState; }
    void setOverride( OverrideState newORState );

    virtual void onSensorUpdate() {}
    void trigger( boolean tState );
    void trigger( boolean tState, unsigned long disableTimeMS) {
      trigger( tState );
      tempDisable( disableTimeMS );
    }
    void update();

protected:
  boolean calcState(boolean sState, OverrideState orState, ExtTriggerState tState );
  void latch(bool external);
  void unlatch(boolean external);

protected:
  StepTimer latchTimer = StepTimer(10000UL, false); // default to 10 seconds
  StepTimer countdownTimer = StepTimer(1000UL, false); // update countdown every second

private:
  boolean sensorState = false;
  unsigned long defTimeout = 10000UL;
  unsigned long currTimeout;
  ExtTriggerState extTriggerState = EXT_TRIGGER_NONE;
  OverrideState ovrState = OVERRIDE_NONE;
  unsigned long latchCount;
  unsigned long lastLatch;
  unsigned long lastUnlatch;
  bool externalTrigger;
  

};

#endif
