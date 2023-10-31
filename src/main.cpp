#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <EEPROM.h>
#include "sleep.h"
#include "blink.h"
#include "pet.h"
#include "maotek.h"
#include "randoms.h"
#include "dizzy.h"
#include "sideeyes.h"
#include "study.h"
#include "memes.h"
#include "flappy.h"

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

extern int momentum;
extern int game_state;

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

  // Setup EEPROM
  EEPROM.begin(4);

  // Random number generator
  randomSeed(analogRead(A0));

  // Touch interrupt handler
  attachInterrupt(digitalPinToInterrupt(D5), IRQHandler, CHANGE);

  // Setup oled
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