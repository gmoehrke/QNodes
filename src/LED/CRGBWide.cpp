#include "CRGBWide.h"

CRGBWide CRGBWide::blend10(const CRGBWide &a, const CRGBWide&b, const uint16_t amountOfB) {
  CRGBWide result( CRGBWide::blend10( a.r(), b.r(), amountOfB ),
                 CRGBWide::blend10( a.g(), b.g(), amountOfB ),
                 CRGBWide::blend10( a.b(), b.b(), amountOfB ), false );
  return result;
}


void CRGBWide::initRGB(const uint16_t red, const uint16_t green, const uint16_t blue ) {
    setR(red);
    setG(green);
    setB(blue);
}

void CRGBWide::setRGB(const uint16_t red, const uint16_t green, const uint16_t blue, const boolean quickInit) {
  if (quickInit) {
    initRGB( red, green, blue );
  }
  else {
    setR(check_max(red));
    setG(check_max(green));
    setB(check_max(blue));
  }
}

uint16_t CRGBWide::blend10(const uint16_t a, const uint16_t b, const uint16_t amountOfB) {
  uint32_t partial = (a * (uint32_t)(MAX_10BIT_VALUE-amountOfB)) + a + (uint32_t)(b * amountOfB) + b;
  uint16_t result;
  result = partial >> 10;
  return result;
}

void CRGBWide::setRGB( const CHSV &newHSV ) {
  CRGB rgb = newHSV;
  if (!equals(rgb)) {
    initRGB( r(), g(), b() );
  }
}

void CRGBWide::setRGB( const CHSV &newHSV, const uint16_t brightness ) {
  CRGB rgb = CRGB(newHSV);
  rgb.maximizeBrightness();
  uint16_t brt = map(brightness, 0, MAX_10BIT_VALUE, 0, 65535);
  initRGB(scale16(map( rgb.r, 0, 255, 0, MAX_10BIT_VALUE), brt),
          scale16(map( rgb.g, 0, 255, 0, MAX_10BIT_VALUE), brt),
          scale16(map( rgb.b, 0, 255, 0, MAX_10BIT_VALUE), brt) );
}
