// =============================================================================
//
// Arduino - Flappy Bird clone
// ---------------------------
// by Themistokle "mrt-prodz" Benetatos
//
// This is a Flappy Bird clone made for the ATMEGA328 and a Sainsmart 1.8" TFT
// screen (ST7735). It features an intro screen, a game over screen containing
// the player score and a similar gameplay from the original game.
//
// Developed and tested with an Arduino UNO and a Sainsmart 1.8" TFT screen.
//
// TODO: - debounce button ?
//
// Dependencies:
// - https://github.com/adafruit/Adafruit-GFX-Library
// - https://github.com/adafruit/Adafruit-ST7735-Library
//
// References:
// - http://www.tweaking4all.com/hardware/arduino/sainsmart-arduino-color-display/
// - http://www.koonsolo.com/news/dewitters-gameloop/
// - http://www.arduino.cc/en/Reference/PortManipulation
//
// --------------------
// http://mrt-prodz.com
// http://github.com/mrt-prodz/ATmega328-Flappy-Bird-Clone
//
// =============================================================================

#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <SPI.h>
#include <TouchScreen.h>

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD TFT(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD tft;

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 220
#define TS_MINY 220
#define TS_MAXX 920
#define TS_MAXY 940

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// instead of using TFT.width() and TFT.height() set constant values
// (we can change the size of the game easily that way)
#define TFTW            240     // screen width
#define TFTH            320     // screen height
#define TFTW2           120     // half screen width
#define TFTH2           160     // half screen height
// #define TFTW            240     // screen width
// #define TFTH            320     // screen height
// #define TFTW2            120     // half screen width
// #define TFTH2            160     // half screen height
// game constant
#define SPEED             1
#define GRAVITY         9.8
#define JUMP_FORCE     2.15
#define SKIP_TICKS     20.0     // 1000 / 50fps
#define MAX_FRAMESKIP     5
// bird size
#define BIRDW             8     // bird width
#define BIRDH             8    // bird height
#define BIRDW2            4     // half width
#define BIRDH2            8     // half height
// pipe size
#define PIPEW            24     // pipe width
#define GAPHEIGHT        72     // pipe gap height
// floor size
#define FLOORH           40     // floor height (from bottom of the screen)
// grass size
#define GRASSH            8     // grass height (inside floor, starts at floor y)

// background
const unsigned int BCKGRDCOL = TFT.Color565(138,235,244);
// bird
const unsigned int BIRDCOL = TFT.Color565(255,254,174);
// pipe
const unsigned int PIPECOL  = TFT.Color565(99,255,78);
// pipe highlight
const unsigned int PIPEHIGHCOL  = TFT.Color565(250,255,250);
// pipe seam
const unsigned int PIPESEAMCOL  = TFT.Color565(0,0,0);
// floor
const unsigned int FLOORCOL = TFT.Color565(246,240,163);
// grass (col2 is the stripe color)
const unsigned int GRASSCOL  = TFT.Color565(141,225,87);
const unsigned int GRASSCOL2 = TFT.Color565(156,239,88);

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
#define C0 BCKGRDCOL
#define C1 TFT.Color565(195,165,75)
#define C2 BIRDCOL
#define C3 WHITE
#define C4 RED
#define C5 TFT.Color565(251,216,114)
static unsigned int birdcol[] =
{ C0, C0, C1, C1, C1, C1, C1, C0,
  C0, C1, C2, C2, C2, C1, C3, C1,
  C0, C2, C2, C2, C2, C1, C3, C1,
  C1, C1, C1, C2, C2, C3, C1, C1,
  C1, C2, C2, C2, C2, C2, C4, C4,
  C1, C2, C2, C2, C1, C5, C4, C0,
  C0, C1, C2, C1, C5, C5, C5, C0,
  C0, C0, C1, C5, C5, C5, C0, C0};

// bird structure
static struct BIRD {
  unsigned int x, y, old_y;
  unsigned int col;
  float vel_y;
} bird;
// pipe structure
static struct PIPE {
  int x, gap_y;
  unsigned int col;
} pipe;

// score
static short score;
// temporary x and y var
static short tmpx, tmpy;

// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and pushColor
// using macro to force inlining
#define drawPixel(a, b, c) TFT.drawPixel(a, b, c)

// ---------------
// initial setup
// ---------------
void setup() {
  TFT.reset();

  uint16_t identifier = TFT.readID();
  if(identifier==0x0101)
      identifier=0x9341;
  identifier=0x9341;
  TFT.begin(identifier);
}

// ---------------
// main loop
// ---------------
void loop() {
  game_start();
  game_loop();
  game_over();
}

// ---------------
// game loop
// ---------------
void game_loop() {
  // ===============
  // prepare game variables
  // draw floor
  // ===============
  // instead of calculating the distance of the floor from the screen height each time store it in a variable
  unsigned int GAMEH = TFTH - FLOORH;
  // draw the floor once, we will not overwrite on this area in-game
  // black line
  TFT.drawFastHLine(0, GAMEH, TFTW, BLACK);
  // grass and stripe
  TFT.fillRect(0, GAMEH+1, TFTW2, GRASSH, GRASSCOL);
  TFT.fillRect(TFTW2, GAMEH+1, TFTW2, GRASSH, GRASSCOL2);
  // black line
  TFT.drawFastHLine(0, GAMEH+GRASSH, TFTW, RED);
  // mud
  TFT.fillRect(0, GAMEH+GRASSH+1, TFTW, FLOORH-GRASSH, FLOORCOL);
  // grass x position (for stripe animation)
  int grassx = TFTW;
  // game loop time variables
  double delta, old_time, next_game_tick, current_time;
  next_game_tick = current_time = millis();
  int loops;
  // passed pipe flag to count score
  bool passed_pipe = false;
  // temp var for setAddrWindow
  unsigned int px;

  while (1) {
    loops = 0;
    while( millis() > next_game_tick && loops < MAX_FRAMESKIP) {
      // ===============
      // input
      // ===============
      digitalWrite(13, HIGH);
      TSPoint p = ts.getPoint();
      digitalWrite(13, LOW);
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
         // if the bird is not too close to the top of the screen apply jump force
         if (bird.y > BIRDH2*0.5) bird.vel_y = -JUMP_FORCE;
         // else zero velocity
         else bird.vel_y = 0;
      }
      // ===============
      // update
      // ===============
      // calculate delta time
      // ---------------
      old_time = current_time;
      current_time = millis();
      delta = (current_time-old_time)/1000;

      // bird
      // ---------------
      bird.vel_y += GRAVITY * delta;
      bird.y += bird.vel_y;

      // pipe
      // ---------------
      pipe.x -= SPEED;
      // if pipe reached edge of the screen reset its position and gap
      if (pipe.x < -PIPEW) {
        pipe.x = TFTW;
        pipe.gap_y = random(10, GAMEH-(10+GAPHEIGHT));
      }

      // ---------------
      next_game_tick += SKIP_TICKS;
      loops++;
    }

    // ===============
    // draw
    // ===============
    // pipe
    // ---------------
    // we save cycles if we avoid drawing the pipe when outside the screen
    if (pipe.x >= 0 && pipe.x < TFTW) {
      // pipe color
      TFT.drawFastVLine(pipe.x+3, 0, pipe.gap_y, PIPECOL);
      TFT.drawFastVLine(pipe.x+3, pipe.gap_y+GAPHEIGHT+1, GAMEH-(pipe.gap_y+GAPHEIGHT+1), PIPECOL);
      // highlight
      TFT.drawFastVLine(pipe.x, 0, pipe.gap_y, PIPEHIGHCOL);
      TFT.drawFastVLine(pipe.x, pipe.gap_y+GAPHEIGHT+1, GAMEH-(pipe.gap_y+GAPHEIGHT+1), PIPEHIGHCOL);
      // bottom and top border of pipe
      drawPixel(pipe.x, pipe.gap_y, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y+GAPHEIGHT, PIPESEAMCOL);
      // pipe seam
      drawPixel(pipe.x, pipe.gap_y-6, PIPESEAMCOL);
      drawPixel(pipe.x, pipe.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
      drawPixel(pipe.x+3, pipe.gap_y-6, PIPESEAMCOL);
      drawPixel(pipe.x+3, pipe.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
    }
    // erase behind pipe
    if (pipe.x <= TFTW) TFT.drawFastVLine(pipe.x+PIPEW, 0, GAMEH, BCKGRDCOL);
    if (pipe.x <= TFTW) TFT.drawFastVLine(pipe.x+PIPEW+1, 0, GAMEH, BCKGRDCOL);
    if (pipe.x <= TFTW) TFT.drawFastVLine(pipe.x+PIPEW+2, 0, GAMEH, BCKGRDCOL);

    // bird
    // ---------------
    tmpx = BIRDW-1;
    do {
          px = bird.x+tmpx+BIRDW;
          // clear bird at previous position stored in old_y
          // we can't just erase the pixels before and after current position
          // because of the non-linear bird movement (it would leave 'dirty' pixels)
          tmpy = BIRDH - 1;
          do {
            drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
          } while (tmpy--);
          // draw bird sprite at new position
          tmpy = BIRDH - 1;
          do {
            drawPixel(px, bird.y + tmpy, birdcol[tmpx + (tmpy * BIRDW)]);
          } while (tmpy--);
    } while (tmpx--);
    // save position to erase bird on next draw
    bird.old_y = bird.y;

    // grass stripes
    // ---------------
    grassx -= SPEED;
    if (grassx < 0) grassx = TFTW;
    TFT.drawFastVLine( grassx    %TFTW, GAMEH+1, GRASSH-1, GRASSCOL);
    TFT.drawFastVLine((grassx+64)%TFTW, GAMEH+1, GRASSH-1, GRASSCOL2);

    // ===============
    // collision
    // ===============
    // if the bird hit the ground game over
    if (bird.y > GAMEH-BIRDH) break;
    // checking for bird collision with pipe
    if (bird.x+BIRDW >= pipe.x-BIRDW2 && bird.x <= pipe.x+PIPEW-BIRDW) {
      // bird entered a pipe, check for collision
      if (bird.y < pipe.gap_y || bird.y+BIRDH > pipe.gap_y+GAPHEIGHT) break;
      else passed_pipe = true;
    }
    // if bird has passed the pipe increase score
    else if (bird.x > pipe.x+PIPEW-BIRDW && passed_pipe) {
      passed_pipe = false;
      // erase score with background color
      TFT.setTextColor(BCKGRDCOL);
      TFT.setCursor( TFTW2, 4);
      TFT.print(score);
      // set text color back to white for new score
      TFT.setTextColor(WHITE);
      // increase score since we successfully passed a pipe
      score++;
    }

    // update score
    // ---------------
    TFT.setCursor( TFTW2, 4);
    TFT.print(score);
  }

  // add a small delay to show how the player lost
  delay(1200);
}

// ---------------
// game start
// ---------------
void game_start() {
  TFT.fillScreen(BLACK);
  TFT.fillRect(10, TFTH2 - 20, TFTW-20, 1, WHITE);
  TFT.fillRect(10, TFTH2 + 32, TFTW-20, 1, WHITE);
  TFT.setTextColor(WHITE);
  TFT.setTextSize(3);
  // half width - num char * char width in pixels
  TFT.setCursor( TFTW2-(6*9), TFTH2 - 16);
  TFT.println("FLAPPY");
  TFT.setTextSize(3);
  TFT.setCursor( TFTW2-(6*9), TFTH2 + 8);
  TFT.println("-BIRD-");
  TFT.setTextSize(0);
  TFT.setCursor( 10, TFTH2 - 28);
  TFT.println("ATMEGA328");
  TFT.setCursor( TFTW2 - (12*3) - 1, TFTH2 + 34);
  TFT.println("Touch Screen");
  while (1) {
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      break;
    }
  }

  // init game settings
  game_init();
}

// ---------------
// game init
// ---------------
void game_init() {
  // clear screen
  TFT.fillScreen(BCKGRDCOL);
  // reset score
  score = 0;
  // init bird
  bird.x = 20;
  bird.y = bird.old_y = TFTH2 - BIRDH;
  bird.vel_y = -JUMP_FORCE;
  tmpx = tmpy = 0;
  // generate new random seed for the pipe gape
  randomSeed(analogRead(0));
  // init pipe
  pipe.x = TFTW;
  pipe.gap_y = random(20, TFTH-60);
}

// ---------------
// game over
// ---------------
void game_over() {
  TFT.fillScreen(BLACK);
  TFT.setTextColor(WHITE);
  TFT.setTextSize(2);
  // half width - num char * char width in pixels
  TFT.setCursor( TFTW2 - (9*6), TFTH2 - 4);
  TFT.println("GAME OVER");
  TFT.setTextSize(0);
  TFT.setCursor( 10, TFTH2 - 14);
  TFT.print("score: ");
  TFT.print(score);
  TFT.setCursor( TFTW2 - (12*3), TFTH2 + 12);
  TFT.println("press button");
  while (1) {
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      break;
    }
  }
}

