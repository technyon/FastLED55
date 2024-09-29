#include "FastLED.h"

/*************************************************
   ANDROID AMBILIGHT APPLICATION ARDUINO SKETCH

   set following values to your needs
 *************************************************/

#define INITIAL_LED_TEST_ENABLED true
#define INITIAL_LED_TEST_BRIGHTNESS 16  // 0..255
#define INITIAL_LED_TEST_TIME_MS 250  // 10..

// Number of leds in your strip.
// Tested with 500 leds and is fine (despite the warning)
// We will use a maximum of 300
#define MAX_LEDS 82

// type of your led controller, possible values, see below
#define LED_TYPE WS2812B

// 3 wire (pwm): NEOPIXEL BTM1829 TM1812 TM1809 TM1804 TM1803 UCS1903 UCS1903B UCS1904 UCS2903 WS2812 WS2852
//               S2812B SK6812 SK6822 APA106 PL9823 WS2811 WS2813 APA104 WS2811_40 GW6205 GW6205_40 LPD1886 LPD1886_8BIT
// 4 wire (spi): LPD8806 WS2801 WS2803 SM16716 P9813 APA102 SK9822 DOTSTAR

// For 3 wire led stripes line Neopixel/Ws2812, which have a data line, ground, and power, you just need to define DATA_PIN.
// For led chipsets that are SPI based (four wires - data, clock, ground, and power), both defines DATA_PIN and CLOCK_PIN are needed

// DATA_PIN, or DATA_PIN, CLOCK_PIN
#define LED_PINS 6        // 3 wire leds
//#define LED_PINS 6, 13  // 4 wire leds

#define COLOR_ORDER GRB  // colororder of the stripe, set RGB in hyperion

#define OFF_TIMEOUT 8000    // ms to switch off after no data was received, set 0 to deactivate

#define BRIGHTNESS 255                      // maximum brightness 0-255
#define DITHER_MODE BINARY_DITHER           // BINARY_DITHER or DISABLE_DITHER
#define COLOR_TEMPERATURE CRGB(255,255,255) // RGB value describing the color temperature
#define COLOR_CORRECTION  TypicalLEDStrip   // predefined fastled color correction
//#define COLOR_CORRECTION  CRGB(255,255,255) // or RGB value describing the color correction

// Baudrate, higher rate allows faster refresh rate and more LEDs
#define BAUD_RATE 115200      // use 115200 for ftdi based boards


/*************************************************
   ANDROID AMBILIGHT APPLICATION ARDUINO SKETCH

   no user changes needed
 *************************************************/

// Adalight sends a "Magic Word" before sending the pixel data
uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;

unsigned long endTime;

// Define the array of leds
CRGB leds[MAX_LEDS];

// set color to all leds
void showColor(const CRGB& led) {
#if MAX_LEDS > 1
    LEDS.showColor(led);
#endif
}

// switch of leds
void switchOff() {
#if MAX_LEDS > 1
    memset(leds, 0, MAX_LEDS * sizeof(struct CRGB));
    FastLED.show();
#endif
}

// function to check if serial data is available
// if timeout occured leds switch of, if configured
bool checkIncommingData() {

    boolean dataAvailable = true;
    while (!Serial.available()) {
        if ( OFF_TIMEOUT > 0 && endTime < millis()) {
            switchOff();
            dataAvailable = false;
            endTime = millis() + OFF_TIMEOUT;
        }
    }
    return dataAvailable;
}

// main function that setups and runs the code
void setup() {

    Serial.begin(BAUD_RATE);
    Serial.print("Ada\n"); // Send "Magic Word" string to host

    int ledCount = MAX_LEDS;

#if MAX_LEDS > 1
    FastLED.addLeds<LED_TYPE, LED_PINS, COLOR_ORDER>(leds, ledCount);
#endif

    // color adjustments
    FastLED.setBrightness ( BRIGHTNESS );
    FastLED.setTemperature( COLOR_TEMPERATURE );
    FastLED.setCorrection ( COLOR_CORRECTION );
    FastLED.setDither     ( DITHER_MODE );

// initial RGB flash
#if INITIAL_LED_TEST_ENABLED == true
    for (int v=0;v<INITIAL_LED_TEST_BRIGHTNESS;v++)
    {
        showColor(CRGB(255,0,0));
        delay(INITIAL_LED_TEST_TIME_MS/2/INITIAL_LED_TEST_BRIGHTNESS);
    }
    for (int v=0;v<INITIAL_LED_TEST_BRIGHTNESS;v++)
    {
        showColor(CRGB(0,255,0));
        delay(INITIAL_LED_TEST_TIME_MS/2/INITIAL_LED_TEST_BRIGHTNESS);
    }
    for (int v=0;v<INITIAL_LED_TEST_BRIGHTNESS;v++)
    {
        showColor(CRGB(0,0,255));
        delay(INITIAL_LED_TEST_TIME_MS/2/INITIAL_LED_TEST_BRIGHTNESS);
    }
#endif
    showColor(CRGB(0, 0, 0));


    boolean transmissionSuccess;
    unsigned long sum_r, sum_g, sum_b;

    // loop() is avoided as even that small bit of function overhead
    // has a measurable impact on this code's overall throughput.
    for(;;) {
        // wait for first byte of Magic Word
        for (i = 0; i < sizeof prefix; ++i) {
            // If next byte is not in Magic Word, the start over
            if (!checkIncommingData() || prefix[i] != Serial.read()) {
                i = 0;
            }
        }

        // Hi, Lo, Checksum
        if (!checkIncommingData()) continue;
        hi = Serial.read();
        if (!checkIncommingData()) continue;
        lo = Serial.read();
        if (!checkIncommingData()) continue;
        chk = Serial.read();

        // if checksum does not match go back to wait
        if (chk != (hi ^ lo ^ 0x55)) continue;

        int num_leds = min ( MAX_LEDS, (hi<<8) + lo + 1 );
        memset(leds, 0, num_leds * sizeof(struct CRGB));
        transmissionSuccess = true;
        sum_r = 0;
        sum_g = 0;
        sum_b = 0;

        // read the transmission data and set LED values
        for (int idx = 0; idx < num_leds; idx++) {
            byte r, g, b;
            if (!checkIncommingData()) {
                transmissionSuccess = false;
                break;
            }
            r = Serial.read();
            if (!checkIncommingData()) {
                transmissionSuccess = false;
                break;
            }
            g = Serial.read();
            if (!checkIncommingData()) {
                transmissionSuccess = false;
                break;
            }
            b = Serial.read();
            leds[idx].r = r;
            leds[idx].g = g;
            leds[idx].b = b;
        }

        // shows new values
        if (transmissionSuccess) {
            endTime = millis() + OFF_TIMEOUT;
#if MAX_LEDS > 1
            FastLED.show();
#endif
        }
    }
} // end of setup

void loop() {
    // Not used. See note in setup() function.
}

