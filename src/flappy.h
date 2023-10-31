#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/**
   Nano Bird - a flappy bird clone for arduino nano, oled screen & push on switch

   Based on the original code found here: https://kotaku.com/it-only-takes-17-lines-of-code-to-clone-flappy-bird-1678240994

   @author  Richard Allsebrook <richardathome@gmail.com>
*/

extern Adafruit_SSD1306 display;

// Initialise 'sprites'
#define SPRITE_HEIGHT   16
#define SPRITE_WIDTH    16

// Two frames of animation
static const unsigned char PROGMEM wing_down_bmp[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000011, B11000000,
  B00011111, B11110000,
  B00111111, B00111000,
  B01111111, B11111110,
  B11111111, B11000001,
  B11011111, B01111110,
  B11011111, B01111000,
  B11011111, B01111000,
  B11001110, B01111000,
  B11110001, B11110000,
  B01111111, B11100000,
  B00111111, B11000000,
  B00000111, B00000000,
  B00000000, B00000000,
};

static const unsigned char PROGMEM wing_up_bmp[] =
{ B00000000, B00000000,
  B00000000, B00000000,
  B00000011, B11000000,
  B00011111, B11110000,
  B00111111, B00111000,
  B01110001, B11111110,
  B11101110, B11000001,
  B11011111, B01111110,
  B11011111, B01111000,
  B11111111, B11111000,
  B11111111, B11111000,
  B11111111, B11110000,
  B01111111, B11100000,
  B00111111, B11000000,
  B00000111, B00000000,
  B00000000, B00000000,
};

void flappyLoop();
void screenWipe(int speed);
void textAt(int x, int y, String txt);
void textAtCenter(int y, String txt);
void outlineTextAtCenter(int y, String txt);
void boldTextAtCenter(int y, String txt);