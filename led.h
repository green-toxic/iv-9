#define FRAMES_PER_SECOND  60

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255 );
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
#define LED_EFFECTS_NUM 3
SimplePatternList gPatterns = { rainbow, sinelon, confetti, juggle };

void flasher(byte odd) {
    FastLED.setBrightness(255);  
    for(int i = 0; i < NUM_LEDS/2; i++) {
      if (odd) {
        leds[i] = CRGB::Red;
        leds[i+3] = CRGB::Blue;
      } else {
        leds[i] = CRGB::Blue;
        leds[i+3] = CRGB::Red;
      }
      FastLED.show();
    }
}

void offLed() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}
