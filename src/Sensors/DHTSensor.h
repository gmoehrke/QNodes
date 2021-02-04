
#ifndef DHTSENSOR_H
#define DTHSENSOR_H

#include "Sensors.h"
#include <DHTesp.h>

class DHTSensor : public SinglePinSensor {

  public:
    DHTSensor() : SinglePinSensor() { }

    DHTSensor( uint8_t initPin, uint8_t DHTType ) : SinglePinSensor(initPin) {
      setDHTType(DHTType);
      setReadInterval( 60000 );  /* Default to once per minute */
    }

  ~DHTSensor() {
    if (dht) { delete dht; }
  }

  virtual boolean isConfigured() override { 
    return (SinglePinSensor::isConfigured() && (dht));
  }

  void setDHTType( uint8_t newType ) {
    if (SinglePinSensor::isConfigured()) {
      if (isStarted()) { stop(); }
      if (dht) { delete dht; dht=nullptr; }
      dhtType = newType;
      dht = new DHTesp();
      dht->setup(getPin(), DHTesp::DHT22 );      
    }
  }

  float getTemperature() { return temp; }
  float getHumidity() { return humidity; }

  virtual void onSensorTempChange( const float newTemp ) {}
  virtual void onSensorHumidityChange( const float newHumidity ) {}
  virtual void setPinMode() override { pinMode( getPin(), INPUT ); }
  virtual void readSensor() override {
    if (isStarted() && dht) {
      TempAndHumidity newValues = dht->getTempAndHumidity();
      if (convertToF) { newValues.temperature = (newValues.temperature * 1.8)+32.0; }
      if  (newValues.temperature != temp ){ 
        onSensorTempChange( newValues.temperature ); 
        temp = newValues.temperature;
      }
      if  (newValues.humidity != humidity ) { 
        onSensorHumidityChange( newValues.humidity ); 
        humidity = newValues.humidity;
      }           
    }
  }

  private:
    boolean convertToF = true;
    uint8_t dhtType = 0;
    DHTesp *dht = nullptr;
    float temp = 0.0F;
    float humidity = 0.0F;
};


#endif
