#include "LatchingBinarySensor.h"

void LatchingBinarySensor::setOverride( OverrideState newORState ) {
  if (ovrState != newORState) {
    boolean prevState = calcState( sensorState, ovrState, extTriggerState );
    ovrState = newORState;
    onSensorEvent( (ovrState==OVERRIDE_NONE ? OVR_NONE : (ovrState==OVERRIDE_ON ? OVR_ON : OVR_OFF)) );
    if (ovrState!=OVERRIDE_NONE && latchTimer.isStarted()) { unlatch( false ); }
    if (prevState != getState()) { onSensorStateChange( !prevState ); }        
  }
}


void LatchingBinarySensor::setLatch( unsigned long newLatch ) {
      defTimeout = newLatch;
      currTimeout = defTimeout;
      latchTimer.setInterval( defTimeout );
      onSensorEvent( SET_TIMEOUT );
    }
    
void LatchingBinarySensor::setCurrLatch( unsigned long newLatch ) {
      if (latchTimer.isStarted()) {
        currTimeout = newLatch;
        latchTimer.setInterval( currTimeout );
        latchTimer.step();
        if (countdownTimer.isStarted()) { countdownTimer.step(); }
        onSensorEvent( SET_ACTIVE_TIMEOUT );
      }
    }


void LatchingBinarySensor::trigger( boolean tState ) {
  if (ovrState==OVERRIDE_NONE) {
    boolean prevState = calcState( sensorState, ovrState, extTriggerState );
    extTriggerState = tState ? EXT_TRIGGER_ON : EXT_TRIGGER_OFF;
    if (tState) {
      latch(true);
    }
    else {
      unlatch(true);
    }
    if (getState() != prevState) { onSensorStateChange( !prevState ); }
  }
}

void LatchingBinarySensor::update() {
  if (isStarted()) {
    boolean prevState = getState();
    boolean prevSensorState = sensorState;
    sensorState = BinarySensor::getState();
    if (sensorState && (sensorState!=prevSensorState)) {
      if (ovrState != OVERRIDE_OFF) { latch(false); }
    }
    else if (sensorState !=prevSensorState) {
      onSensorEvent( sensorState ? SENS_ON : SENS_OFF );
    }
    if (latchTimer.isStarted() and latchTimer.isUp()) { unlatch(false); }
    if (getState() != prevState) { onSensorStateChange( !prevState ); }
    if (extTriggerState != EXT_TRIGGER_NONE) { extTriggerState = EXT_TRIGGER_NONE; }
    onSensorUpdate();
    // Do this last so sensor updates come before timer updates if timer is just starting...
    if (latchTimer.isStarted() and countdownTimer.isUp()) {
      onSensorLatchCountdown( latchTimer.timeRemaining() );
      countdownTimer.step();
    }
  }
}

boolean LatchingBinarySensor::calcState(boolean sState, OverrideState orState, ExtTriggerState tState ) {
  return ((((sState || latchTimer.isStarted() || tState==EXT_TRIGGER_ON) && orState == OVERRIDE_NONE) || (orState==OVERRIDE_ON))  && !(orState==OVERRIDE_OFF));
}

void LatchingBinarySensor::latch(bool external) {
  latchCount++;
  lastLatch=GET_TIME_MILLIS_ABS;
  if (latchTimer.isStarted()) {
    latchTimer.step();
    onSensorEvent( external ? EXT_RELATCH : SENS_RELATCH );
  }
  else {
    currTimeout = defTimeout;
    latchTimer.setInterval( currTimeout );
    latchTimer.start();
    countdownTimer.start();
    onSensorEvent( external ? EXT_LATCH : SENS_LATCH );
  }
}

void LatchingBinarySensor::unlatch(boolean external) {
  latchTimer.stop();
  countdownTimer.stop();
  latchCount=0;
  lastUnlatch=GET_TIME_MILLIS_ABS;
  onSensorEvent( external ? EXT_UNLATCH : UNLATCH );
}
