/*
 *   Definition of CRGBWide - support for 10-bit color values for RGB LED control on ESP8266.  The data   
 *   pins on ESP8266 have a PWM range of 0-1023, so to take full advantage of that, CRGBWide provides a  
 *   FastLed::CRGB compatible object to manage color settings for the LED.
 *   
 *   Note:  The CRGBWide is not 100% drop-in compatible with CRGB in that the r, g, b values become functions r(), g() and b()
 *   and setting them involves calling setR(), setG(), setB().  They CAN be assigned to and from CRGB objects and tested for 
 *   (absolute) equality against CRGB objects - absolute meaning that equality os only possible up to 255,255,255 (no scaling is 
 *   performed to test relative equality).  
 *   
 */
#ifndef CRGBWIDE_H
#define CRGBWIDE_H

#include "FastLED.h"

#define MAX_10BIT_VALUE 1023

//  For efficiency, all 3 values for RGB can be stored in a single 32-bit unsigned (as opposed to 48bits for 3 16-bit ints).  Just need to use some bit manipulation to 
//  store and retrieve the values.  The first 2 bits of the unsigned are not used.  
                        
#define TEN_BITMASK 0x000003FF      // Mask last 10 bits from 32-bit unsigned:      00 000000000000000000001111111111

#define R_BITMASK  0x000FFFFF      // Mask 10-bit red bits from 32-bit unsigned:   00 000000000011111111111111111111
#define G_BITMASK  0x3FF003FF      // Mask 10-bit green bits from 32-bit unsigned: 00 111111111100000000001111111111
#define B_BITMASK  0x3FFFFC00      // Mask 10-bit blue bits from 32-bit unsigned:  00 111111111111111111110000000000
  
#define check_max(num) (num > MAX_10BIT_VALUE ? MAX_10BIT_VALUE : num)
#define eight_to_ten(num) map( num, 0, 255, 0, MAX_10BIT_VALUE )
#define ten_to_eight(num) map( num, 0, MAX_10BIT_VALUE, 0, 255 )

class CRGBWide {
  public:

    CRGBWide() {}
    CRGBWide(const uint16_t r, const uint16_t g, const uint16_t b, const boolean quickInit = false ) { setRGB( r, g, b, quickInit );}
    CRGBWide(const CRGBWide &src) : CRGBWide(src.r(), src.g(), src.b() ) { }
    CRGBWide( const CRGB &src ) : CRGBWide( eight_to_ten(src.r), eight_to_ten(src.g), eight_to_ten(src.b), true ) {}
    CRGBWide( const CHSV &src ) { setRGB(src); }
    void setRGB(const uint16_t red, const uint16_t green, const uint16_t blue, const boolean quickInit = false);
    CRGBWide( const CHSV &src, const uint16_t brightness ) { setRGB( src, brightness ); }

    uint16_t static blend10(const uint16_t a, const uint16_t b, const uint16_t amountOfB);

    uint16_t r() const { return (uint16_t)(rgb >> 20); }
    void setR( uint16_t value ) { rgb = (rgb & R_BITMASK) | ((uint32_t)value << 20); }

    uint16_t g() const { return (uint16_t)((rgb >> 10) & TEN_BITMASK); }
    void setG( uint16_t value ) { rgb = (rgb & G_BITMASK) | ((uint32_t)value << 10); }

    uint16_t b() const { return (uint16_t)(rgb & TEN_BITMASK); }
    void setB( uint16_t value ) { rgb = (rgb & B_BITMASK) | ((uint32_t)value); }
    
    void setRGB( const CRGBWide &newColor ) { if (!equals(newColor)) { setRGB( newColor.r(), newColor.g(), newColor.b() ); } }
    void setRGB( const CRGB &newCRGB ) { if (!equals(newCRGB)) { initRGB(eight_to_ten(newCRGB.r), eight_to_ten(newCRGB.g), eight_to_ten(newCRGB.b)); }    }
    void setRGB( const CHSV &newHSV );
    void setRGB( const CHSV &newHSV, const uint16_t brightness );
    CRGB asCRGB() { return CRGB( ten_to_eight( r() ), ten_to_eight( g() ), ten_to_eight( b() ) ); }
    String asString() { return (String(r()) +", "+String(g())+", "+String(b()) ); }
    boolean equals(const CRGBWide &comp ) { return (comp.r()==r() && comp.g() == g() && comp.b()==b());  }
    boolean equals(const CRGB &comp ) {return (eight_to_ten(comp.r)==r() && eight_to_ten(comp.g)==g() && eight_to_ten(comp.b)==b());    }
    boolean operator ==(const CRGBWide &comp ) { return equals(comp);  }
    boolean operator ==(const CRGB &comp ) { return equals(comp);  }
    boolean operator !=(const CRGB &comp ) { return !(equals(comp)); }
    const CRGBWide& operator =(const CRGBWide &src ) { setRGB( src.r(), src.g(), src.b() ); return *this; }
    const CRGBWide& operator =(const CRGB &src ) { initRGB( eight_to_ten(src.r), eight_to_ten(src.g), eight_to_ten(src.b) ); return *this; }
    const CRGBWide& operator =(const CHSV &src ) { setRGB( src ); return *this; }

    static CRGBWide blend10(const CRGBWide &a, const CRGBWide&b, const uint16_t amountOfB);
  private:

    uint32_t rgb = 0;

    /* shortcut initialization - performs no range checking on rgb values - can only used when source
     * values are casted from uint8_t or known to be <= 1023.
     */
    void initRGB(const uint16_t red, const uint16_t green, const uint16_t blue );
};


#endif
