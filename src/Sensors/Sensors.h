#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "FlexTimer.h"

class SinglePinSensor {
  public:
    SinglePinSensor() {}
    SinglePinSensor( uint8_t initPin ) { setPin(initPin); }
    void setPin( int newPin ) { pin = newPin; }
    uint8_t getPin() { return pin; }
    virtual void setPinMode() = 0;
    virtual boolean isConfigured() { return (pin != 0); }
    void setReadInterval( unsigned long newInterval ) { readTimer.setInterval( newInterval ); }
    void tempDisable( unsigned long msDuration ) { readTimer.addDelta(msDuration); }
    unsigned long getReadInterval() { return readTimer.getInterval(); }            
    boolean isStarted() { return readTimer.isStarted(); }
    virtual void readSensor() = 0;
    void read();
    void start();
    void stop();    
    
  protected:  
    uint8_t pin = 0;
    FlexTimer readTimer = FlexTimer(250, false);      
};

class SingleValuePinSensor : public SinglePinSensor {
  public:
    SingleValuePinSensor() {}
    SingleValuePinSensor( uint8_t initPin ) { setPin(initPin); } 
  protected:
    uint16_t pinStateValue = LOW;
    uint16_t setValue( uint8_t newPinStateValue ) { 
      if (newPinStateValue != pinStateValue) { pinStateValue = newPinStateValue; } 
      return pinStateValue;
      }
    uint16_t getValue() { read(); return pinStateValue; }
    uint16_t getLastValue() { return pinStateValue; }        
};


class BinarySensor : public SingleValuePinSensor {
  
  public:
    BinarySensor() {}
    BinarySensor( uint8_t initPin ) : SingleValuePinSensor(initPin) { start(); };
    uint16_t getTarget() { return targetValue; }
    void setTarget( uint16_t newTarget ) { targetValue = newTarget; }    
    virtual void setPinMode() override;
    virtual void readSensor() override;
    boolean getLastState() { return this->getLastValue()==targetValue; }
    boolean getState() { return this->getValue()==targetValue; }

  private:
    uint16_t targetValue = HIGH;
};

class AnalogSensor : public SingleValuePinSensor {
  public:
    AnalogSensor() : SingleValuePinSensor() { 
      inverted = false;
      threshold = 1;
      lastValueRaw = 0;
    }
    AnalogSensor( uint8_t initPin ) : SingleValuePinSensor( initPin ) {  inverted = false; start(); }
    void setInverted( boolean newInverted ) { inverted = newInverted; }
    boolean getInverted() { return inverted; }
    void setThreshold( uint16_t newThreshold ) { if (threshold!=newThreshold) { threshold = newThreshold; } }
    virtual void onSensorChange( const uint16_t newValue ) { }
    uint16_t getThreshold() { return threshold; }
    virtual void setPinMode() override;
    virtual void readSensor() override;
    uint16_t getLastValueRaw() { return lastValueRaw; }
  private:  
    boolean inverted;
    uint16_t threshold;
    uint16_t lastValueRaw;
};


#endif
