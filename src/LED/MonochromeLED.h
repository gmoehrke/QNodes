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
    uint16_t brightness = 0;

  public:
    MonoVariableLED( const uint8_t initPin ) : MonoBinaryLED( initPin ) { writeState(state); }
    MonoVariableLED( const uint8_t initPin, boolean initInverted ) : MonoBinaryLED( initPin, initInverted ) {}
    MonoVariableLED( const uint8_t initPin, boolean initInverted, boolean initState ) : MonoBinaryLED( initPin, initInverted, initState ) {}
    virtual uint16_t getOnValue() override { return inverted ? 1023-brightness : brightness; }
    void setBrightness( uint16_t newBrightness ) {
      brightness = newBrightness > 1023 ? 1023 : newBrightness;
      if (getState()) { writeState(true); }
    }
};

#endif
