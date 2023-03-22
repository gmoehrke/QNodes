
#include "ColorLED.h"

ColorLED::ColorLED() {
  currentColor = CRGBWide(0,0,0);
  fromColor = CRGBWide(0,0,0);
  toColor = CRGBWide(0,0,0);
  flashColor = CRGBWide(0,0,0);
  refreshTimer.start();
}

void ColorLED::setColor( const CRGBWide newColor ) {
  currentColor = newColor;
  toColor = newColor;
  currFadeColor = newColor;
  if (fadeTimer.isStarted()) { fadeTimer.stop(); }
}

void ColorLED::startFlash( const CRGBWide &color, const unsigned long duration, const int repeat, const int lag, boolean repeatInfinite ) {
  if (!(flashTimer.isStarted() || flashRepeatTimer.isStarted())) {
    flashDuration = duration;
    flashTimer.setInterval( flashDuration );
    flashRepeat = repeat;
    flashRepeatTimer.setInterval( lag );
    flashTimer.start();
    flashColor=color;
    toColor = currentColor;
  }
}

void ColorLED::startFlash( const CRGBWide &color, const unsigned long duration, const int repeat, const int lag ) {
   startFlash( color, duration, repeat, lag, false );
}

void ColorLED::startFlash( const CRGBWide &color, const unsigned long duration, const int lag ) {
  startFlash( color, duration, 0, lag, true );
}

void ColorLED::startFade( const CRGBWide &targetColor, const unsigned long duration ) {
  if (fadeTimer.isStarted()) { fadeTimer.stop(); }
  if (duration == 0) {
    currFadeColor = targetColor;
    currentColor = targetColor;
    toColor = targetColor;
  }
  else {
    fadeTimer.setInterval( duration );
    fromColor = currentColor;
    currFadeColor = currentColor;
    toColor=targetColor;
    fadeTimer.start();
  }
}

void ColorLED::update()
{
  if (refreshTimer.isUp()) {
    // If flash start time is set...
    if (flashTimer.isStarted()) {
      // if the current color isn't already the flash color and the flash time hasn't been exceeded
      if ((!flashTimer.isUp())) {
        // set to Flash (on) color - Ease in/out to make flash a little less drastic
        CRGBWide fColor;
        uint16_t flashprog = fixed_map( flashTimer.timeSinceTriggered(), 0, flashDuration, 0, 1023);
        uint16_t flashFract = (flashprog < 256 ? fixed_map( flashprog, 0, 256, 0, 1023 ) : ( flashprog > 768 ? fixed_map( flashprog, 768, 1023, 1023, 0 ) : 1023 ));
        if (fadeTimer.isStarted()) {
          fColor = CRGBWide::blend10( currFadeColor, flashColor, flashFract );
        }
        else {
          fColor = CRGBWide::blend10( toColor, flashColor, flashFract );
        }
        currentColor = fColor;
      }
    }
    // not flashing OR if the flash time has been exceeded - set the existing fade or color
   if (!(flashTimer.isStarted()) || (flashTimer.isStarted() && flashTimer.isUp())) {
     if (fadeTimer.isStarted()) {
       CRGBWide c = toColor;
       currFadeColor=CRGBWide::blend10( fromColor.asCRGB(), c, fixed_map(fadeTimer.timeSinceTriggered(),0,fadeTimer.getInterval(), 0, 1023) );
       currentColor=currFadeColor;
     }
     // else if not fading (because we're done, or were not fading to begin with
     if ( (!(fadeTimer.isStarted())) || (fadeTimer.isUp())) {
       currFadeColor=toColor;
       currentColor=toColor;
       // if was fading and now time is expired, stop the timer
       if (fadeTimer.isUp()) { fadeTimer.stop(); }
     }
     if (flashTimer.isStarted() && flashTimer.isUp()) {
       flashTimer.stop();
       currentColor = toColor;
       currFadeColor = toColor;
       if (flashRepeat > 0 || flashRepeatInfinite) {
         if (!flashRepeatInfinite) { flashRepeat--; }
         if (flashRepeat > 0 || flashRepeatInfinite) {
           flashRepeatTimer.start();
         }
       }
     }
   }
   if (flashRepeatTimer.isStarted() && flashRepeatTimer.isUp()) {
      flashRepeatTimer.stop();
      flashTimer.start();
      currentColor = toColor;
   }
  onShow();
  refreshTimer.step();
}
}
