#include "Sensors.h"

void SinglePinSensor::start() {
  if (isConfigured()) {
    setPinMode();
    readTimer.start();
  }
}
void SinglePinSensor::stop() {
  if (isStarted()) {
    readTimer.stop();
  }
}

void SinglePinSensor::read() {
  if (readTimer.isUp()) {
    this->readSensor();
    readTimer.step();
  }
}

void BinarySensor::setPinMode() {
  pinMode( pin, INPUT );
}

void BinarySensor::readSensor() {
  pinStateValue = digitalRead( pin );
}


void AnalogSensor::setPinMode() {
  pinMode( pin, INPUT );
}

void AnalogSensor::readSensor() {
  int readResult = analogRead( pin );
  // Make sure value is in range 0-1023
  readResult = (readResult <= 1023 ? (readResult < 0 ? 0 : readResult) : 1023);
  uint16_t result = (inverted ? (1023-readResult) : readResult);
  lastValueRaw = getLastValue();
  uint16_t diff = result>lastValueRaw ? result-lastValueRaw : lastValueRaw-result;
  if ((diff <= threshold) || (diff==0)) {
    pinStateValue = lastValueRaw;
  }
  else {
    onSensorChange( result );
    pinStateValue = result;  
  }

}
