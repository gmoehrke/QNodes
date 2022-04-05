#ifndef MONOCHROME_LEDS_H
#define MONOCHROME_LEDS_H

class MonoBinaryLED {
  private:
    uint8_t pin;

  protected:
    boolean inverted = false;
    boolean state = true;

  public:
    MonoBinaryLED( const uint8_t initPin ) {
      pin = initPin;
      pinMode( pin, OUTPUT );
    }
    MonoBinaryLED( const uint8_t initPin, boolean initInvert ) : MonoBinaryLED( initPin ) {
      inverted = initInvert;
    }
    MonoBinaryLED( const uint8_t initPin, boolean initInvert, boolean initState ) : MonoBinaryLED( initPin, initInvert ) {
      writeState( initState );
    }
    void setPin( uint8_t newPin ) { pin = newPin; pinMode(pin, OUTPUT); this->writeState(state); }
    virtual uint8_t getPin() { return pin; }
    virtual uint16_t getOffValue() { return inverted ? 1023 : 0; }
    virtual uint16_t getOnValue() { return inverted ? 0 : 1023; }
    boolean getState() { return state; }
    void setState( boolean newState ) {
      if (newState != state) {
        writeState( newState );
      }
    }
    void toggle() {
      setState( !state );
    }
  protected:
    void writeState( boolean newState ) {
      state = newState;
      analogWrite( pin, state ? getOnValue() :  getOffValue() );
    }
};

class MonoVariableLED : public MonoBinaryLED {
  private:
    uint16_t currBrightness = 0;
    uint16_t brightness = 0;
    uint16_t fromBright = 0;
    uint16_t toBright = 0;
    StepTimer fadeTimer = StepTimer(250, false);

  public:
    MonoVariableLED( const uint8_t initPin ) : MonoBinaryLED( initPin ) { writeState(state); }
    MonoVariableLED( const uint8_t initPin, boolean initInverted ) : MonoBinaryLED( initPin, initInverted ) {}
    MonoVariableLED( const uint8_t initPin, boolean initInverted, boolean initState ) : MonoBinaryLED( initPin, initInverted, initState ) {}
    virtual uint16_t getOnValue() override { return inverted ? 1023-brightness : brightness; }
     boolean isFading() { return fadeTimer.isStarted(); }

    void setBrightness( uint16_t newBrightness, unsigned long duration = 0) {
        if (fadeTimer.isStarted()) { fadeTimer.stop(); }
        if (duration == 0) {
          brightness  = newBrightness;
          fromBright = newBrightness;
          toBright = newBrightness;
        }
        else {
         fadeTimer.setInterval( duration );
         fromBright = getBrightness();         
         toBright=newBrightness;
         fadeTimer.start();
        }
        this->onShow();
    }

    virtual void update() {
       if (isFading()) {
         brightness=fixed_map( fadeTimer.timeSinceTriggered(),0,fadeTimer.getInterval(), fromBright, toBright);
       }
       // else if not fading (because we're done, or were not fading to begin with
       if ( (!(fadeTimer.isStarted())) || (fadeTimer.isUp())) {
         brightness=toBright;
         fromBright=toBright;
         // if was fading and now time is expired, stop the timer
         if (fadeTimer.isUp()) { fadeTimer.stop(); }
       }       
       this->onShow();
    };

    virtual void onShow() {
      if (currBrightness != brightness) {
        analogWrite( getPin(), getBrightness() );
        currBrightness = getBrightness();
      }
    }

    uint16_t getBrightness() { return brightness; }
};

#endif
