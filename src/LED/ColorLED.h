#ifndef COLOR_LED_H
#define COLOR_LED_H

#include "FastLED.h"
#include "CRGBWide.h"
#include "FlexTimer.h"

class ColorLED {
  public:
    CRGBWide currentColor;

  public:
    ColorLED();
    void setRefreshInterval( unsigned long newInterval ) { refreshTimer.setInterval( newInterval ); }
    unsigned long getRefreshInterval() { return refreshTimer.getInterval(); }

    void setColor( const CRGBWide newColor );
    void startFlash( const CRGBWide &color, const unsigned long duration, const int repeat, const int lag, boolean repeatInfinite );
    void startFlash( const CRGBWide &color, const unsigned long duration, const int repeat, const int lag );
    void startFlash( const CRGBWide &color, const unsigned long duration, const int lag );

    boolean isFading() { return fadeTimer.isStarted(); }
    void startFade( const CRGBWide &targetColor, const unsigned long duration );
    virtual void onShow()=0;
    void update();

  private:
    CRGBWide fromColor;
    CRGBWide toColor;
    CRGBWide flashColor;
    CRGBWide currFadeColor;
    StepTimer fadeTimer = StepTimer(250, false);
    StepTimer flashDelayTimer = StepTimer(250, false);
    StepTimer flashRepeatTimer = StepTimer( 250, false );
    StepTimer flashTimer = StepTimer( 200, false );
    StepTimer refreshTimer = StepTimer( 15, false);
    unsigned long flashDuration = 0;
    int flashRepeat = 0;
    boolean flashRepeatInfinite = false;
};

#endif
