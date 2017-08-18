#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define BUTTON_0_PIN 0
#define BUTTON_1_PIN 1
#define SIGNAL_PIN 4
#define LEDS 24

// definitions for methods with optional arguments
void cycleI(uint16_t upper, uint8_t wait = 0);
byte brightness(uint16_t brightness, uint16_t maxBrightness = 255);
uint32_t color(byte r, byte g, byte b, byte maxBrightness = 255);
uint32_t colorByHue(byte hue, byte brightness = 255);
void infinity(uint32_t color, uint16_t leds, uint8_t wait, uint32_t tail = 0);

// initialize neo pixels
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, SIGNAL_PIN, NEO_GRB + NEO_KHZ800);

// variables for inputs
int inputPins[2] = {BUTTON_0_PIN, BUTTON_1_PIN};
bool inputStates[2] = {HIGH, HIGH};
unsigned long inputLongpressAts[2] = {0, 0};

// animation specific variables
unsigned long nextRandomAnimationAt = 0;
byte selectedAnimation = 0;
unsigned long previousNextRandomAnimationAt = 0;
byte previousSelectedAnimation = 0;
uint16_t i = 0;
uint16_t j = 0;
unsigned long nextCycleAt = 0;
unsigned long now = 0;

uint32_t off = strip.Color(0, 0, 0);
uint32_t mirroredPositions[LEDS] = {};
uint16_t brightnessCap = 50;

void setup() {
  // initialize all pixels to 'off'
  strip.begin();
  strip.show();

  // setup input pins
  for (byte i = 0; i < sizeof(inputPins); i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
  }

  // setup array for easier mirrored animation
  uint16_t mirrorOffset = strip.numPixels() + strip.numPixels() / 2 - 1;
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (i < strip.numPixels() / 2) {
      mirroredPositions[i] = i;
    } else {
      mirroredPositions[i] = mirrorOffset - i;
    }
  }
}

void loop() {
  now = millis();

  switch (checkInput(0)) {
    case 1:
      // cycle to next animation
      nextRandomAnimationAt = 0;
      selectAnimation((selectedAnimation + 1) % 6);
      break;
    case 2:
      // activate random animation cycling
      selectRandomAnimation();
      break;
  }
  switch (checkInput(1)) {
    case 1:
      // go into torch mode
      previousSelectedAnimation = selectedAnimation;
      previousNextRandomAnimationAt = nextRandomAnimationAt;
      nextRandomAnimationAt = 0;
      selectAnimation(98);
      break;
    case 0:
      selectAnimation(99);
      break;
  }

  if (nextRandomAnimationAt != 0 && now > nextRandomAnimationAt) {
    selectRandomAnimation();
  } else if (now > nextCycleAt) {
    switch(selectedAnimation) {
      case 0:
        infinity(color(0, 0, 255), 6, 50);
        break;
      case 1:
        infinity(color(0, 0, 255), 6, 50, color(0, 32, 0));
        break;
      case 2:
        circle(color(0, 255, 0), 3, 50);
        break;
      case 3:
        uniformlyCycleThroughColors(5);
        break;
      case 4:
        pumpColors(1);
        break;
      case 5:
        theaterChase(color(255, 0, 0), 50);
        break;
      case 98: // torch mode
        torch(1000);
        break;
      case 99: // untorch mode
        untorch(1000);
        break;
    }
  }
}

//////////////////////////////////////////////////
// helper methods ////////////////////////////////
//////////////////////////////////////////////////

// Return wether the given input has changed to high
byte checkInput(byte which) {
  bool state = digitalRead(inputPins[which]);
  if (state != inputStates[which]) {
    inputStates[which] = state;
    delay(50); // to avoid bouncing
    if (state) {
      inputLongpressAts[which] = 0;
      return 0;
    } else {
      inputLongpressAts[which] = now + 1000;
      return 1;
    }
  } else {
    unsigned long longpressAt = inputLongpressAts[which];
    if (longpressAt != 0 && now > longpressAt) {
      inputLongpressAts[which] = 0;
      return 2;
    } else {
      return -1;
    }
  }
}

// cycle through the i loop
void cycleI(uint16_t upper, uint8_t wait) {
  i = (i + 1) % upper;
  nextCycleAt += wait;
}

// cycle through the j loop (eventually also cycling through i)
void cycleJ(uint16_t upperI, uint16_t upperJ, uint8_t wait) {
  j = (j + 1) % upperJ;
  if (j == 0) {
    cycleI(upperI);
  }
  nextCycleAt += wait;
}

// selects an animation and resets cycle counters and timing so that it starts
// immediately
void selectAnimation(byte animation) {
  clear();
  strip.show();
  selectedAnimation = animation;
  i = j = 0;
  nextCycleAt = now;
}

// selects a random animation and also activates a random animation cycle
void selectRandomAnimation() {
  byte newAnimation;
  do {
    newAnimation = random(6);
  } while (newAnimation == selectedAnimation);
  selectAnimation(newAnimation);
  nextRandomAnimationAt = now + 5000;
}

// Input a brightness for a color channel and get the dimmed version back,
// according to brightnessCap which is the max brightness.
byte brightness(uint16_t brightness, uint16_t maxBrightness) {
  return brightness * (maxBrightness / 255.0) * (brightnessCap / 255.0);
}

// Input three color values (r, g, b) and get the dimmed color back, see
// brightness.
uint32_t color(byte r, byte g, byte b, byte maxBrightness) {
  return strip.Color(brightness(r, maxBrightness), brightness(g, maxBrightness), brightness(b, maxBrightness));
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t colorByHue(byte hue, byte brightness) {
  if (hue < 85) {
    return color(255 - hue * 3, 0, hue * 3, brightness);
  } else if (hue < 170) {
    hue -= 85;
    return color(0, hue * 3, 255 - hue * 3, brightness);
  } else {
    hue -= 170;
    return color(hue * 3, 255 - hue * 3, 0, brightness);
  }
}

void all(uint32_t color) {
  for (uint16_t led = 0; led < strip.numPixels(); led++) {
    strip.setPixelColor(led, color);
  }
}

void clear() {
  all(off);
}

//////////////////////////////////////////////////
// animation methods /////////////////////////////
//////////////////////////////////////////////////

void infinity(uint32_t color, uint16_t leds, uint8_t wait, uint32_t tail) {
  all(tail);
  for (uint16_t led = i; led < i + leds; led++) {
    strip.setPixelColor(mirroredPositions[led % strip.numPixels()], color);
  }
  strip.show();
  cycleI(strip.numPixels(), wait);
}

void circle(uint32_t color, uint16_t leds, uint8_t wait) {
  clear();
  for (uint16_t led = i; led < i + leds; led++) {
    strip.setPixelColor(mirroredPositions[led % strip.numPixels()], color);
  }
  for (uint16_t led = i; led < i + leds; led++) {
    strip.setPixelColor(mirroredPositions[(led + strip.numPixels() / 2) % strip.numPixels()], color);
  }
  strip.show();
  cycleI(strip.numPixels(), wait);
}

void uniformlyCycleThroughColors(uint8_t wait) {
  all(colorByHue(i));
  strip.show();
  cycleI(256, wait);
}

void pumpColors(uint8_t wait) {
  uint16_t hue = i * 85; // cycle through colors in 3 steps
  uint16_t brightness = j;
  if (brightness > 255) {
    brightness = 510 - brightness; // counting down again
  }
  all(colorByHue(hue, brightness));
  strip.show();
  cycleJ(3, 510, wait);
}

void torch(uint8_t wait) {
  uint16_t brightness = 15 * i;
  all(strip.Color(brightness, brightness, brightness));
  strip.show();
  if (brightness == 255) {
    nextCycleAt += 1000;
  } else {
    cycleI(18, wait / 18);
  }
}

void untorch(uint8_t wait) {
  uint16_t brightness = 255 - 15 * i;
  all(strip.Color(brightness, brightness, brightness));
  strip.show();
  if (brightness == 0) {
    selectAnimation(previousSelectedAnimation);
    nextRandomAnimationAt = previousNextRandomAnimationAt;
  } else {
    cycleI(18, wait / 18);
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  uint16_t realJ = j / 2;
  if (j % 2 == 1) {
    c = 0;
    realJ = (j -1) / 2;
  }
  for (uint16_t led = 0; led < strip.numPixels(); led += 3) {
    strip.setPixelColor(led + realJ, c);
  }
  if (j % 2 == 0) {
    strip.show();
    cycleJ(10, 6, wait);
  } else {
    cycleJ(10, 6, 0);
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
