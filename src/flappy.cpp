#include "flappy.h"
#include <EEPROM.h>

/**
   Nano Bird - a flappy bird clone for arduino nano, oled screen & push on switch

   Based on the original code found here: https://kotaku.com/it-only-takes-17-lines-of-code-to-clone-flappy-bird-1678240994

   @author  Richard Allsebrook <richardathome@gmail.com>
*/

// Game variables
#define GAME_SPEED 50

int game_state = 1; // 0 = game over screen, 1 = in game
int score = 0; // current game score
int high_score = 0; // highest score since the nano was reset
int bird_x = (int)display.width() / 4; // birds x position (along) - initialised to 1/4 the way along the screen
int bird_y; // birds y position (down)
int momentum = 0; // how much force is pulling the bird down
int wall_x[2]; // an array to hold the walls x positions
int wall_y[2]; // an array to hold the walls y positions
int wall_gap = 30; // size of the wall wall_gap in pixels
int wall_width = 10; // width of the wall in pixels

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
        // high_score = max(score, high_score);
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
    EEPROM.get(0, high_score);
    
    if (score > high_score) {
      EEPROM.put(0, score);
      EEPROM.commit();
    }

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