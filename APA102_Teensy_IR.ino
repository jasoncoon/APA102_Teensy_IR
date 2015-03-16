#include <FastLED.h>
#include <IRremote.h>
#include <EEPROM.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define LED_PIN     3
#define CLOCK_PIN   2
#define IR_RECV_PIN 12
#define COLOR_ORDER GBR
#define CHIPSET     APA102
#define NUM_LEDS    144

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
uint8_t brightness = brightnessMap[0];

CRGB leds[NUM_LEDS];
IRrecv irReceiver(IR_RECV_PIN);

CRGB solidColor = CRGB::Red;

#include "Commands.h"

typedef uint16_t(*PatternFunctionPointer)();
typedef PatternFunctionPointer PatternList [];
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGBPalette16 gPalette;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

int autoPlayDurationSeconds = 10;
unsigned int autoPlayTimout = 0;
bool autoplayEnabled = false;

int currentIndex = 0;
PatternFunctionPointer currentPattern;

#include "SoftTwinkles.h"
#include "Fire2012WithPalette.h"
#include "BouncingBalls2014.h"
#include "Lightning2014.h"
#include "ColorTwinkles.h"

const PatternList patterns = {
    softTwinkles,
    colorTwinkles,
    fire2012WithPalette,
    sinelon,
    lightning2014,
    bouncingBalls2014,
    juggle,
    bpm,
    confetti,
    rainbow,
    rainbowWithGlitter,
    hueCycle,
    showSolidColor,
};

const int patternCount = ARRAY_SIZE(patterns);

void setup()
{
    //delay(3000); // sanity delay
    Serial.println("ready...");

    loadSettings();

    FastLED.addLeds<CHIPSET, LED_PIN, CLOCK_PIN, COLOR_ORDER, DATA_RATE_MHZ(12)>(leds, NUM_LEDS);
    FastLED.setCorrection(0xB0FFFF);
    FastLED.setBrightness(brightness);

    // Initialize the IR receiver
    irReceiver.enableIRIn();
    irReceiver.blink13(true);

    gPalette = HeatColors_p;

    currentPattern = patterns[currentIndex];

    autoPlayTimout = millis() + (autoPlayDurationSeconds * 1000);
}

void loop()
{
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random());
    
    unsigned int requestedDelay = currentPattern();

    FastLED.show(); // display this frame

    handleInput(requestedDelay);

    if (autoplayEnabled && millis() > autoPlayTimout) {
        move(1);
        autoPlayTimout = millis() + (autoPlayDurationSeconds * 1000);
    }

    // do some periodic updates
    EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
}

void loadSettings() {
    // load settings from EEPROM

    // brightness
    brightness = EEPROM.read(0);
    if (brightness < 1)
        brightness = 1;
    else if (brightness > 255)
        brightness = 255;

    // currentIndex
    currentIndex = EEPROM.read(1);
    if (currentIndex < 0)
        currentIndex = 0;
    else if (currentIndex >= patternCount)
        currentIndex = patternCount - 1;

    // solidColor
    solidColor.r = EEPROM.read(2);
    solidColor.g = EEPROM.read(3);
    solidColor.b = EEPROM.read(4);

    if (solidColor.r == 0 && solidColor.g == 0 && solidColor.b == 0)
        solidColor = CRGB::White;
}

void setSolidColor(CRGB color) {
    solidColor = color;

    EEPROM.write(2, solidColor.r);
    EEPROM.write(3, solidColor.g);
    EEPROM.write(4, solidColor.b);

    moveTo(patternCount - 1);
}

void powerOff()
{
    // clear the display
    // fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        FastLED.show(); // display this frame
        delay(1);
    }

    FastLED.show(); // display this frame

    while (true) {
        InputCommand command = readCommand();
        if (command == Power ||
            command == Brightness)
            return;

        // go idle for a while, converve power
        delay(250);
    }
}

void move(int delta) {
    moveTo(currentIndex + delta);
}

void moveTo(int index) {
    currentIndex = index;

    if (currentIndex >= patternCount)
        currentIndex = 0;
    else if (currentIndex < 0)
        currentIndex = patternCount - 1;

    currentPattern = patterns[currentIndex];

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    EEPROM.write(1, currentIndex);
}

int getBrightnessLevel() {
    int level = 0;
    for (int i = 0; i < brightnessCount; i++) {
        if (brightnessMap[i] >= brightness) {
            level = i;
            break;
        }
    }
    return level;
}

uint8_t cycleBrightness() {
    adjustBrightness(1);

    if (brightness == brightnessMap[0])
        return 0;

    return brightness;
}

void adjustBrightness(int delta) {
    int level = getBrightnessLevel();

    level += delta;
    if (level < 0)
        level = brightnessCount - 1;
    if (level >= brightnessCount)
        level = 0;

    brightness = brightnessMap[level];
    FastLED.setBrightness(brightness);

    EEPROM.write(0, brightness);
}

void handleInput(unsigned int requestedDelay) {
    unsigned int requestedDelayTimeout = millis() + requestedDelay;

    while (true) {
        InputCommand command = readCommand(defaultHoldDelay);

        if (command != None) {
            Serial.print("command: ");
            Serial.println((int) command);
        }

        if (command == Up ||
            command == Right) {
            move(1);
            break;
        }
        else if (command == Down ||
            command == Left) {
            move(-1);
            break;
        }
        else if (command == Brightness) {
            bool wasHolding = isHolding;
            if (isHolding || cycleBrightness() == 0) {
                heldButtonHasBeenHandled();
                powerOff();
                break;
            }
        }
        else if (command == Power) {
            powerOff();
            break;
        }
        else if (command == BrightnessUp) {
            adjustBrightness(1);
        }
        else if (command == BrightnessDown) {
            adjustBrightness(-1);
        }
        else if (command == PlayMode) { // toggle pause/play
            autoplayEnabled = !autoplayEnabled;
        }
        //else if (command == Palette) { // cycle color pallete
        //    effects.CyclePalette();
        //}

        // pattern buttons

        else if (command == Pattern1) {
            moveTo(0);
            break;
        }
        else if (command == Pattern2) {
            moveTo(1);
            break;
        }
        else if (command == Pattern3) {
            moveTo(2);
            break;
        }
        else if (command == Pattern4) {
            moveTo(3);
            break;
        }
        else if (command == Pattern5) {
            moveTo(4);
            break;
        }
        else if (command == Pattern6) {
            moveTo(5);
            break;
        }
        else if (command == Pattern7) {
            moveTo(6);
            break;
        }
        else if (command == Pattern8) {
            moveTo(7);
            break;
        }
        else if (command == Pattern9) {
            moveTo(8);
            break;
        }
        else if (command == Pattern10) {
            moveTo(9);
            break;
        }
        else if (command == Pattern11) {
            moveTo(10);
            break;
        }
        else if (command == Pattern12) {
            moveTo(11);
            break;
        }

        // custom color adjustment buttons

        else if (command == RedUp) {
            solidColor.red += 1;
            setSolidColor(solidColor);
            break;
        }
        else if (command == RedDown) {
            solidColor.red -= 1;
            setSolidColor(solidColor);
            break;
        }
        else if (command == GreenUp) {
            solidColor.green += 1;
            setSolidColor(solidColor);

            break;
        }
        else if (command == GreenDown) {
            solidColor.green -= 1;
            setSolidColor(solidColor);
            break;
        }
        else if (command == BlueUp) {
            solidColor.blue += 1;
            setSolidColor(solidColor);
            break;
        }
        else if (command == BlueDown) {
            solidColor.blue -= 1;
            setSolidColor(solidColor);
            break;
        }

        // color buttons

        else if (command == Red) {
            setSolidColor(CRGB::Red);
            break;
        }
        else if (command == RedOrange) {
            setSolidColor(CRGB::OrangeRed);
            break;
        }
        else if (command == Orange) {
            setSolidColor(CRGB::Orange);
            break;
        }
        else if (command == YellowOrange) {
            setSolidColor(CRGB::Goldenrod);
            break;
        }
        else if (command == Yellow) {
            setSolidColor(CRGB::Yellow);
            break;
        }

        else if (command == Green) {
            setSolidColor(CRGB::Green);
            break;
        }
        else if (command == Lime) {
            setSolidColor(CRGB::Lime);
            break;
        }
        else if (command == Aqua) {
            setSolidColor(CRGB::Aqua);
            break;
        }
        else if (command == Teal) {
            setSolidColor(CRGB::Teal);
            break;
        }
        else if (command == Navy) {
            setSolidColor(CRGB::Navy);
            break;
        }

        else if (command == Blue) {
            setSolidColor(CRGB::Blue);
            break;
        }
        else if (command == RoyalBlue) {
            setSolidColor(CRGB::RoyalBlue);
            break;
        }
        else if (command == Purple) {
            setSolidColor(CRGB::Purple);
            break;
        }
        else if (command == Indigo) {
            setSolidColor(CRGB::Indigo);
            break;
        }
        else if (command == Magenta) {
            setSolidColor(CRGB::Magenta);
            break;
        }

        else if (command == White) {
            setSolidColor(CRGB::White);
            break;
        }
        else if (command == Pink) {
            setSolidColor(CRGB::Pink);
            break;
        }
        else if (command == LightPink) {
            setSolidColor(CRGB::LightPink);
            break;
        }
        else if (command == BabyBlue) {
            setSolidColor(CRGB::CornflowerBlue);
            break;
        }
        else if (command == LightBlue) {
            setSolidColor(CRGB::LightBlue);
            break;
        }

        if (millis() >= requestedDelayTimeout)
            break;
    }
}

// scale the brightness of the screenbuffer down
void dimAll(byte value)
{
    for (int i = 0; i < NUM_LEDS; i++){
        leds[i].nscale8(value);
    }
}

uint16_t showSolidColor() {
    fill_solid(leds, NUM_LEDS, solidColor);

    return 60;
}

uint16_t rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow(leds, NUM_LEDS, gHue, 7);

    return 8;
}

uint16_t rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
    return 8;
}

void addGlitter(fract8 chanceOfGlitter)
{
    if (random8() < chanceOfGlitter) {
        leds[random16(NUM_LEDS)] += CRGB::White;
    }
}

uint16_t confetti()
{
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, 255);
    return 8;
}

uint16_t bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
    return 8;
}

uint16_t juggle() {
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++) {
        leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
    return 8;
}

// An animation to play while the crowd goes wild after the big performance
uint16_t applause()
{
    static uint16_t lastPixel = 0;
    fadeToBlackBy(leds, NUM_LEDS, 32);
    leds[lastPixel] = CHSV(random8(HUE_BLUE, HUE_PURPLE), 255, 255);
    lastPixel = random16(NUM_LEDS);
    leds[lastPixel] = CRGB::White;
    return 8;
}

// An "animation" to just fade to black.  Useful as the last track
// in a non-looping performance-oriented playlist.
uint16_t fadeToBlack()
{
    fadeToBlackBy(leds, NUM_LEDS, 10);
    return 8;
}

uint16_t sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(leds, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS);
    leds[pos] += CHSV(gHue, 255, 192);
    return 8;
}

uint16_t hueCycle() {
    fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
    return 60;
}
