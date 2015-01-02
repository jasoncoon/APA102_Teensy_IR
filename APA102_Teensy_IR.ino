#include <FastLED.h>
#include <IRremote.h>

#define LED_PIN     2
#define CLOCK_PIN   3
#define IR_RECV_PIN 12
#define COLOR_ORDER GRB
#define CHIPSET     APA102
#define NUM_LEDS    63

const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
uint8_t brightness = brightnessMap[0];

CRGB leds[NUM_LEDS];
IRrecv irReceiver(IR_RECV_PIN);

CRGB solidColor = CRGB::Red;

#include "Commands.h"

typedef unsigned int(*PatternFunctionPointer)(void);

CRGBPalette16 gPalette;

#include "BouncingBalls2014.h"
#include "Lightning2014.h"
#include "ColorTwinkles.h"

int autoPlayDurationSeconds = 10;
unsigned int autoPlayTimout = 0;
bool autoplayEnabled = false;

int currentIndex = 0;
PatternFunctionPointer currentPattern;

static const int PATTERN_COUNT = 7;
PatternFunctionPointer patterns[PATTERN_COUNT] = {
    ColorTwinkles,
    HueCycle,
    Fire2012WithPalette,
    QuadWave,
    BouncingBalls2014,
    Lightning2014,
    SolidColor,
};

// Fire2012 with programmable Color Palette
//
// This code is the same fire simulation as the original "Fire2012",
// but each heat cell's temperature is translated to color through a FastLED
// programmable color palette, instead of through the "HeatColor(...)" function.
//
// Four different static color palettes are provided here, plus one dynamic one.
// 
// The three static ones are: 
//   1. the FastLED built-in HeatColors_p -- this is the default, and it looks
//      pretty much exactly like the original Fire2012.
//
//  To use any of the other palettes below, just "uncomment" the corresponding code.
//
//   2. a gradient from black to red to yellow to white, which is
//      visually similar to the HeatColors_p, and helps to illustrate
//      what the 'heat colors' palette is actually doing,
//   3. a similar gradient, but in blue colors rather than red ones,
//      i.e. from black to blue to aqua to white, which results in
//      an "icy blue" fire effect,
//   4. a simplified three-step gradient, from black to red to white, just to show
//      that these gradients need not have four components; two or
//      three are possible, too, even if they don't look quite as nice for fire.
//
// The dynamic palette shows how you can change the basic 'hue' of the
// color palette every time through the loop, producing "rainbow fire".

void setup()
{
    //delay(3000); // sanity delay
    Serial.println("ready...");

    FastLED.addLeds<CHIPSET, LED_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness);

    // Initialize the IR receiver
    irReceiver.enableIRIn();
    irReceiver.blink13(true);

    gPalette = HeatColors_p;

    currentPattern = patterns[0];

    autoPlayTimout = millis() + (autoPlayDurationSeconds * 1000);
}

void loop()
{
    unsigned int requestedDelay = currentPattern();

    FastLED.show(); // display this frame

    handleInput(requestedDelay);

    if (autoplayEnabled && millis() > autoPlayTimout) {
        move(1);
        autoPlayTimout = millis() + (autoPlayDurationSeconds * 1000);
    }
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
        if (command == InputCommand::Power ||
            command == InputCommand::Brightness)
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

    if (currentIndex >= PATTERN_COUNT)
        currentIndex = 0;
    else if (currentIndex < 0)
        currentIndex = PATTERN_COUNT - 1;

    currentPattern = patterns[currentIndex];
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
}

void handleInput(unsigned int requestedDelay) {
    unsigned int requestedDelayTimeout = millis() + requestedDelay;

    while (true) {
        //unsigned long code = readIRCode();

        //if (code != 0)
        //    Serial.println(code);

        InputCommand command = readCommand(defaultHoldDelay); // getCommand(code);

        if (command != InputCommand::None) {
            Serial.print("command: ");
            Serial.println((int) command);
        }

        if (command == InputCommand::Up ||
            command == InputCommand::Right) {
            move(1);
            break;
        }
        else if (command == InputCommand::Down ||
            command == InputCommand::Left) {
            move(-1);
            break;
        }
        else if (command == InputCommand::Brightness) {
            bool wasHolding = isHolding;
            if (isHolding || cycleBrightness() == 0) {
                heldButtonHasBeenHandled();
                powerOff();
                break;
            }
        }
        else if (command == InputCommand::Power) {
            powerOff();
            break;
        }
        else if (command == InputCommand::BrightnessUp) {
            adjustBrightness(1);
        }
        else if (command == InputCommand::BrightnessDown) {
            adjustBrightness(-1);
        }
        else if (command == InputCommand::PlayMode) { // toggle pause/play
            autoplayEnabled = !autoplayEnabled;
        }
        //else if (command == InputCommand::Palette) { // cycle color pallete
        //    effects.CyclePalette();
        //}
        else if (command == InputCommand::Red) {
            solidColor = CRGB::Red;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::Green) {
            solidColor = CRGB::Green;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::Blue) {
            solidColor = CRGB::Blue;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::White) {
            solidColor = CRGB::White;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::RedUp) {
            solidColor.red += 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::GreenUp) {
            solidColor.green += 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::BlueUp) {
            solidColor.blue += 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::RedDown) {
            solidColor.red -= 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::GreenDown) {
            solidColor.green -= 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::BlueDown) {
            solidColor.blue -= 1;
            moveTo(6);
            break;
        }
        else if (command == InputCommand::Pattern1) {
            moveTo(0);
            break;
        }
        else if (command == InputCommand::Pattern2) {
            moveTo(1);
            break;
        }
        else if (command == InputCommand::Pattern3) {
            moveTo(2);
            break;
        }
        else if (command == InputCommand::Pattern4) {
            moveTo(3);
            break;
        }
        else if (command == InputCommand::Pattern5) {
            moveTo(4);
            break;
        }
        else if (command == InputCommand::Pattern6) {
            moveTo(5);
            break;
        }

        if (millis() >= requestedDelayTimeout)
            break;
    }
}

// scale the brightness of the screenbuffer down
void DimAll(byte value)
{
    for (int i = 0; i < NUM_LEDS; i++){
        leds[i].nscale8(value);
    }
}

byte count = 0;
unsigned int QuadWave() {
    leds[quadwave8(count) / 4] = CHSV(0, 255, 255);
    DimAll(200);
    count++;
    return 10;
}

unsigned int SolidColor() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = solidColor;
    }

    return 60;
}

CHSV color = CHSV(0, 255, 255);

unsigned int HueCycle() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }

    color.hue++;

    return 60;
}

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
//// 
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation, 
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// This simulation scales it self a bit depending on NUM_LEDS; it should look
// "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 
//
// I recommend running this simulation at anywhere from 30-100 frames per second,
// meaning an interframe delay of about 10-35 milliseconds.
//
// Looks best on a high-density LED setup (60+ pixels/meter).
//
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 55, suggested range 20-100 
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

unsigned int Fire2012WithPalette()
{
    gPalette = HeatColors_p;

    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Array of temperature readings at each simulation cell
    static byte heat[NUM_LEDS];

    // Step 1.  Cool down every cell a little
    for (int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for (int k = NUM_LEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if (random8() < SPARKING) {
        int y = random8(7);
        heat[y] = qadd8(heat[y], random8(160, 255));
    }

    // Step 4.  Map from heat cells to LED colors
    for (int j = 0; j < NUM_LEDS; j++) {
        // Scale the heat value from 0-255 down to 0-240
        // for best results with color palettes.
        byte colorindex = scale8(heat[j], 240);
        leds[j] = ColorFromPalette(gPalette, colorindex);
    }

    return 30;
}