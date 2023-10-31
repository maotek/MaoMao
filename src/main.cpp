#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "sleep.h"
#include "blink.h"
#include "pet.h"
#include "maotek.h"
#include "randoms.h"
#include "dizzy.h"
#include "sideeyes.h"
#include "study.h"
#include "memes.h"
#include <EEPROM.h>

#define MENU_BLINK 0
#define MENU_STUDY 2
#define MENU_SLEEP 1
#define MENU_STUDY 2
#define MENU_FLAPPY 3

#define MODE_BLINK 0
#define MODE_PETTING 1
#define MODE_DIZZY 2
#define MODE_SLEEP 3
#define MODE_SIDEEYE 4
#define MODE_STUDY 5
#define MODE_MEMES 6
#define MODE_FLAPPY 7

Adafruit_SSD1306 display(128, 64, &Wire, -1);
Adafruit_MPU6050 mpu;

// Game variables
#define GAME_SPEED 50
int game_state = 1; // 1 = game over screen, 0 = in game
int score = 0; // current game score
int high_score = 0; // highest score since the nano was reset
int bird_x = (int) display.width() / 4; // birds x position (along) - initialised to 1/4 the way along the screen
int bird_y; // birds y position (down)
int momentum = 0; // how much force is pulling the bird down
int wall_x[2]; // an array to hold the walls x positions
int wall_y[2]; // an array to hold the walls y positions
int wall_gap = 30; // size of the wall wall_gap in pixels
int wall_width = 10; // width of the wall in pixels


// Forward declaration
void changeMode(byte newMode, uint16_t expireTime, byte maxFrames);
void delayFrame(uint16_t delay);

// For the game
void screenWipe(int speed);
void textAt(int x, int y, String txt);
void textAtCenter(int y, String txt);
void outlineTextAtCenter(int y, String txt);
void boldTextAtCenter(int y, String txt);
void flappyLoop();


// Constants
#define BUTTON_DELAY 750           // Time after the first button press to process the button sequence
#define MPU_POLLING_INTERVAL 1000  // Interval for polling the mpu
#define SHAKE_THRESHOLD 15         // Acceleration threshold for shaking
#define PETTING_TIMER 2500
#define DIZZY_TIMER 3000
#define MEMES_TIMER 2000


volatile byte mode = MODE_BLINK;
volatile byte menu = 0;

// Variables to keep track of button presses
volatile byte buttonPressed = 0;        // is the button pressed?
volatile byte buttonPressedAmount = 0;  // amount of presses
long firstButtonPressedTime = 0;        // time the first button was pressed, to detect multiple presses

uint16_t frameDelay = 0;
unsigned long framePausedTime;

byte frameDirection = 0;
byte maxFrameCount = BLINK_FRAMES;
byte curFrameCount = BLINK_FRAMES / 2;  // start at eyes closed

unsigned long timerStartTime;
unsigned long timer;

unsigned long mpuPrevTime;  // Previous time for MPU scheduling

byte randomSideEye;

byte randomMeme;

void IRAM_ATTR IRQHandler() {
  if (digitalRead(D5)) {

    if(menu == MENU_FLAPPY) {
      // If we are in the flappy menu, and the game started, we want to act immediately after user presses button
      if(game_state == 0){
        momentum = -4;
      }
      
    }

    buttonPressed = 1;
    if (buttonPressedAmount == 0) {
      firstButtonPressedTime = millis();
    }
    buttonPressedAmount++;
  } else {
    buttonPressed = 0;
  }
}

void setup() {
  Serial.begin(9600);

  // Random number generator
  randomSeed(analogRead(A0));

  // Setup oled
  attachInterrupt(digitalPinToInterrupt(D5), IRQHandler, CHANGE);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;)
      ;
  }

  // Setup MPU
  if (!mpu.begin()) {
    for (;;)
      ;
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

  // Display informatics
  display.clearDisplay();
  display.clearDisplay();
  display.drawBitmap(0, 0, maotek, 128, 64, WHITE);
  display.display();
  delay(3000);
}

void loop() {

  if (!frameDelay) {
    display.clearDisplay();

    if (mode == MODE_BLINK) {
      display.drawBitmap(0, 0, blink[curFrameCount], 128, 64, WHITE);

      // Introduce custom delays
      if (curFrameCount == 0) {
        delayFrame(random(2000, 4000));
        if (random(0, 8) == 0) {
          changeMode(MODE_SIDEEYE, 0, SIDEEYE_FRAMES);
          randomSideEye = random(0, 2);
        }

      } else if (curFrameCount == maxFrameCount / 2) {
        delayFrame(random(20, 400));
      }

    } else if (mode == MODE_SIDEEYE) {
      display.drawBitmap(0, 0, sideEyes[randomSideEye][curFrameCount], 128, 64, WHITE);
      delayFrame(50);

      if (curFrameCount == maxFrameCount - 1) {  // return to main mode
        changeMode(MODE_BLINK, 0, BLINK_FRAMES);
      } else if (curFrameCount == maxFrameCount / 2) {  // stop during middle
        delayFrame(random(1000, 2000));
      }
    }

    else if (mode == MODE_PETTING) {
      display.drawBitmap(0, 0, petting[curFrameCount], 128, 64, WHITE);
      delayFrame(100);
    } else if (mode == MODE_DIZZY) {
      display.drawBitmap(0, 0, dizzy[curFrameCount], 128, 64, WHITE);
      delayFrame(50);
    } else if (mode == MODE_SLEEP) {
      display.drawBitmap(0, 0, sleep[curFrameCount], 128, 64, WHITE);
      delayFrame(300);
    } else if (mode == MODE_STUDY) {
      display.drawBitmap(0, 0, study[curFrameCount], 128, 64, WHITE);
      delayFrame(500);
    } else if (mode == MODE_MEMES) {
      display.drawBitmap(0, 0, memes[randomMeme], 128, 64, WHITE);
    } else if (mode == MODE_FLAPPY) {
      flappyLoop();
    }

    display.display();
    curFrameCount = ++curFrameCount % maxFrameCount;
  }


  // When to stop delays inbetween frames
  if (millis() - framePausedTime > frameDelay) {
    frameDelay = 0;
  }

  // When to stop the different modes
  if (timer && millis() - timerStartTime > timer) {
    if (menu == MENU_BLINK) {
      changeMode(MODE_BLINK, 0, BLINK_FRAMES);  // Set current frame to random frame
    } else if (menu == MENU_STUDY) {
      changeMode(MODE_STUDY, 0, STUDY_FRAMES);
    }
  }

  // poll the MPU
  if (millis() - mpuPrevTime > MPU_POLLING_INTERVAL) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    if (abs(a.acceleration.x) > SHAKE_THRESHOLD || abs(a.acceleration.y) > SHAKE_THRESHOLD || abs(a.acceleration.z - 9.8) > SHAKE_THRESHOLD) {
      if (menu == 0) {
        changeMode(MODE_DIZZY, DIZZY_TIMER, DIZZY_FRAMES);
      }
    }

    mpuPrevTime = millis();
  }

  // Process buttons
  if (firstButtonPressedTime != 0 && millis() - firstButtonPressedTime > BUTTON_DELAY) {


    if (buttonPressedAmount == 3) {
      Serial.println("Pressed thrice");
    }
    if (buttonPressedAmount == 2) {  // Pressed twice
      Serial.println("Pressed twice");

      if (menu == MENU_BLINK) {
        changeMode(MODE_PETTING, PETTING_TIMER, PET_FRAMES);
      } else if (menu == MENU_STUDY) {
        randomMeme = random(0, MEMESLEN);
        changeMode(MODE_MEMES, MEMES_TIMER, 1);
      }

    } else if (buttonPressedAmount == 1 && buttonPressed) {  // Hold button
      Serial.println("Pressed hold");

      if (menu == MENU_BLINK) {
        menu = MENU_SLEEP;
        mode = MODE_SLEEP;
        maxFrameCount = SLEEP_FRAMES;
        curFrameCount = random(0, maxFrameCount);
        frameDelay = 0;
      } else if (menu == MENU_SLEEP) {
        menu = MENU_STUDY;
        mode = MODE_STUDY;
        maxFrameCount = STUDY_FRAMES;
        curFrameCount = random(0, maxFrameCount);
        frameDelay = 0;
      } else if (menu == MENU_STUDY) {
        menu = MENU_FLAPPY;
        mode = MODE_FLAPPY;
        maxFrameCount = 1;
        curFrameCount = 0;
        frameDelay = 0;
      } else if (menu == MENU_FLAPPY && game_state == 1) { // we should not be ingame when we want to change
        menu = MENU_BLINK;
        mode = MODE_BLINK;
        maxFrameCount = BLINK_FRAMES;
        curFrameCount = random(0, maxFrameCount);
        frameDelay = 0;
      }
    } else if (buttonPressedAmount == 1) {  // Pressed once
      Serial.println("Pressed once");
      if (game_state == 1 && mode == MODE_FLAPPY) {
        screenWipe(10);
        game_state = 0;
      }
    }

    buttonPressed = 0;
    firstButtonPressedTime = 0;
    buttonPressedAmount = 0;
  }
}

void changeMode(byte newMode, uint16_t expireTime, byte maxFrames) {
  mode = newMode;
  frameDelay = 0;
  timerStartTime = millis();
  timer = expireTime;
  maxFrameCount = maxFrames;
  curFrameCount = 0;
}

void delayFrame(uint16_t delay) {
  frameDelay = delay;
  framePausedTime = millis();
}


/**
   Nano Bird - a flappy bird clone for arduino nano, oled screen & push on switch

   Based on the original code found here: https://kotaku.com/it-only-takes-17-lines-of-code-to-clone-flappy-bird-1678240994

   @author  Richard Allsebrook <richardathome@gmail.com>
*/


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


void flappyLoop() {

  if (game_state == 0) {
    // in game
    display.clearDisplay();

    // If the flap button is currently pressed, reduce the downward force on the bird a bit.
    // Once this foce goes negative the bird goes up, otherwise it falls towards the ground
    // gaining speed
    // if (digitalRead(D5) == HIGH) {
    //   momentum = -4;
    // }

    // increase the downward force on the bird
    momentum += 1;

    // add the downward force to the bird position to determine it's new position
    bird_y += momentum;

    // make sure the bird doesn't fly off the top of the screen
    if (bird_y < 0 ) {
      bird_y = 0;
    }

    // make sure the bird doesn't fall off the bottom of the screen
    // give it a slight positive lift so it 'waddles' along the ground.
    if (bird_y > display.height() - SPRITE_HEIGHT) {
      bird_y = display.height() - SPRITE_HEIGHT;
      momentum = -2;
    }

    // display the bird
    // if the momentum on the bird is negative the bird is going up!
    if (momentum < 0) {

      // display the bird using a randomly picked flap animation frame
      if (random(2) == 0) {
        display.drawBitmap(bird_x, bird_y, wing_down_bmp, 16, 16, WHITE);
      }
      else {
        display.drawBitmap(bird_x, bird_y, wing_up_bmp, 16, 16, WHITE);
      }

    }
    else {

      // bird is currently falling, use wing up frame
      display.drawBitmap(bird_x, bird_y, wing_up_bmp, 16, 16, WHITE);

    }

    // now we draw the walls and see if the player has hit anything
    for (int i = 0 ; i < 2; i++) {

      // draw the top half of the wall
      display.fillRect(wall_x[i], 0, wall_width, wall_y[i], WHITE);

      // draw the bottom half of the wall
      display.fillRect(wall_x[i], wall_y[i] + wall_gap, wall_width, display.height() - wall_y[i] + wall_gap, WHITE);

      // if the wall has hit the edge of the screen
      // reset it back to the other side with a new gap position
      if (wall_x[i] < 0) {
        wall_y[i] = random(0, display.height() - wall_gap);
        wall_x[i] = display.width();
      }

      // if the bird has passed the wall, update the score
      if (wall_x[i] == bird_x) {
        score++;

        // highscore is whichever is bigger, the current high score or the current score
        high_score = max(score, high_score);
      }

      // if the bird is level with the wall and not level with the gap - game over!
      if (
        (bird_x + SPRITE_WIDTH > wall_x[i] && bird_x < wall_x[i] + wall_width) // level with wall
        &&
        (bird_y < wall_y[i] || bird_y + SPRITE_HEIGHT > wall_y[i] + wall_gap) // not level with the gap
      ) {
        
        // display the crash and pause 1/2 a second
        display.display();
        delay(500);

        // switch to game over state
        game_state = 1; 

      }

      // move the wall left 4 pixels
      wall_x[i] -= 4;
    }

    // display the current score
    boldTextAtCenter(0, (String)score);

    // now display everything to the user and wait a bit to keep things playable
    // display.display();
    delay(GAME_SPEED);
  }
  else {
    outlineTextAtCenter(1, "Flappy MaoMao");
    
    textAtCenter(display.height() / 2 - 8, "Tap to start");
    textAtCenter(display.height() / 2, "Hold to exit");
    boldTextAtCenter(display.height() - 16, "HIGH SCORE");
    boldTextAtCenter(display.height()  - 8, String(high_score));

    // display.display();

    // setup a new game
    bird_y = display.height() / 2;
    momentum = -4;
    wall_x[0] = display.width() ;
    wall_y[0] = display.height() / 2 - wall_gap / 2;
    wall_x[1] = display.width() + display.width() / 2;
    wall_y[1] = display.height() / 2 - wall_gap / 1;
    score = 0;
  }

}

/**
 * clear the screen using a wipe down animation
 */
void screenWipe(int speed) {

  // progressivly fill screen with white
  for (int i = 0; i < display.height(); i += speed) {
    display.fillRect(0, i, display.width(), speed, WHITE);
    display.display();
  }

  // progressively fill the screen with black
  for (int i = 0; i < display.height(); i += speed) {
    display.fillRect(0, i, display.width(), speed, BLACK);
    display.display();
  }

}

/**
 * displays txt at x,y coordinates
 */
void textAt(int x, int y, String txt) {
  display.setCursor(x, y);
  display.print(txt);
}

/**
 * displays text centered on the line
 */
void textAtCenter(int y, String txt) {
  textAt(display.width() / 2 - txt.length() * 3, y, txt);
}

/**
 * displays outlined text centered on the line
 */
void outlineTextAtCenter(int y, String txt) {
  int x = display.width() / 2 - txt.length() * 3;

  display.setTextColor(WHITE);
  textAt(x - 1, y, txt);
  textAt(x + 1, y, txt);
  textAt(x, y - 1, txt);
  textAt(x, y + 1, txt);

  display.setTextColor(BLACK);
  textAt(x, y, txt);
  display.setTextColor(WHITE);

}

/**
 * displays bold text centered on the line
 */
void boldTextAtCenter(int y, String txt) {
  int x = display.width() / 2 - txt.length() * 3;

  textAt(x, y, txt);
  textAt(x + 1, y, txt);

}