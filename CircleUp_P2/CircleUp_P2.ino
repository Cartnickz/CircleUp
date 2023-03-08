// Zach Cartnick & Brooke Abeles
// PHYS 336 - Electronics
// Digital Project
// Player 2

#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <SPI.h>

#include <string.h>
#include <Arduino.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

// Screen Stuff
#define TFT_DC  9
#define TFT_CS 10

// Teensy Pin
#define P2_IN 15  // sends signal to other teensy about end of the game 
#define P2_OUT 16  // looks for signal about game end )
#define START_IN 17  // looks for 3 game counter (goes into )
#define START_OUT 18 // if reached three games, reset counter
#define RESET_IN 8 // start game, output on player one, input on player 2
#define WIN_PIN 7  // adds to counter

// Bluetooth Stuff
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BLUEFRUIT_HWSERIAL_NAME      Serial1
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, 14);

// functions
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

// the packet buffer
extern uint8_t packetbuffer[];

// Rectangle is 320 x 240 (x and y)
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

// initialize program variables
float x_pos, y_pos;
float x_goal, y_goal;
float x_vel, y_vel;
float x, y;
float x_sens = 6;
float y_sens = 6;
float goal_sens = 0; // 0.35;


void setup(void) {
  // put your setup code here, to run once:
  while (!Serial);  // required for Flora & Micro
  delay(50);
  Serial.begin(115200);

  pinMode(P2_IN, INPUT);
  pinMode(P2_OUT, OUTPUT);
  pinMode(START_IN, INPUT);
  pinMode(START_OUT, OUTPUT);
  pinMode(WIN_PIN, OUTPUT);
  pinMode(RESET_IN, INPUT);

  digitalWrite(P2_OUT, LOW);
  digitalWrite(START_OUT, LOW);

  // picking seed for random number
  // randomSeed(42);

  // -----------------------------------------------------------------------------------
  // A bunch of touchscreen stuff

  // initialize screen and fill black background
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  // draw the border of the screen
  tft.fillRect(0, 0, 320, 5, ILI9341_WHITE);
  tft.fillRect(0, 0, 5, 240, ILI9341_WHITE);
  tft.fillRect(315, 0, 5, 240, ILI9341_WHITE);
  tft.fillRect(0, 235, 320, 5, ILI9341_WHITE);

  // -----------------------------------------------------------------------------------
  // A bunch of bluetooth stuff
  Serial.println(F("Adafruit Bluefruit App Controller Example"));
  Serial.println(F("-----------------------------------------"));
  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));
  if ( !ble.begin(VERBOSE_MODE) ) {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );
  if ( FACTORYRESET_ENABLE ) {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);


  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();
  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();


  ble.verbose(false);  // debug info is a little annoying after this point!
  tft.setCursor(120, 120);
  tft.setTextSize(1);
  tft.print("Please Connect.");
  /* Wait for connection */
  while (! ble.isConnected()) {
    delay(500);
  }

  tft.fillRect(70, 100, 200, 50, ILI9341_BLACK);
  tft.setCursor(130, 120);
  tft.print("Connected.");
  tft.setCursor(80, 130);
  tft.print("Please Turn On Accelerometer.");

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  ble.sendCommandCheckOK("AT+GAPDEVNAME=CircleUp_P2");
  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}
// -----------------------------------------------------------------------------------

/*!
    @brief  Constantly poll for new command or response data
*/

void loop(void) {

  int state = 1;
  uint8_t len = readPacket(&ble, 200);
  if (len == 0) return;

  printHex(packetbuffer, len);
  tft.fillRect(75, 100, 200, 50, ILI9341_BLACK);

  // starting screen
  while(state == 1) {
    tft.setCursor(80, 150);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Waiting for P1 to start...");

    for (int color = 0; color < 256; color += 15) {
      tft.setCursor(30, 100);
      tft.setTextSize(5);
      tft.setTextColor(tft.color565(0, color, 255-color));
      tft.print("CircleUp!");
      
      // check for button press
        if (digitalRead(START_IN)) {
          digitalWrite(START_OUT, HIGH);
          delay(50);
          digitalWrite(START_OUT, LOW);
          state = 2;
          break;
        }
      delay(50);
    }

    if (state == 1) {
      for (int color = 0; color < 256; color += 15) {
        tft.setCursor(30, 100);
        tft.setTextSize(5);
        tft.setTextColor(tft.color565(0, 255-color, color));
        tft.print("CircleUp!");

        // check for button press
        if (digitalRead(START_IN)) {
          digitalWrite(START_OUT, HIGH);
          delay(50);
          digitalWrite(START_OUT, LOW);
          state = 2;
          break;
        }
      delay(50);
      }
    }

    }

  while (state == 2) {
    tft.fillRect(5, 5, 310, 230, ILI9341_BLACK);
    tft.setCursor(80, 100);
    tft.setTextSize(5);
    tft.setTextColor(tft.color565(255, 255, 255));

    // Ready!
    tft.print("Ready?");
    delay(400);
      for (int color = 255; color > 1; color -= 1) {
        tft.setCursor(80, 100);
        tft.setTextSize(5);
        tft.setTextColor(tft.color565(color, color, color));
        tft.print("Ready?");
        tft.fillRect(0, 0, 320 * (255-color) / 255, 5, ILI9341_WHITE);
        tft.fillRect(0, 0, 5, 240  * (255-color) / 255, ILI9341_WHITE);
        tft.fillRect(315, 0, 5, 240  * (255-color) / 255, ILI9341_WHITE);
        tft.fillRect(0, 235, 320  * (255-color) / 255, 5, ILI9341_WHITE);
      }
    
    delay(100);
    // Set..
    tft.setCursor(80, 100);
    tft.setTextSize(5);
    tft.setTextColor(tft.color565(255, 255, 255));
    tft.print(" Set..");
    delay(200);
      for (int color = 255; color > 1; color -= 1) {
        tft.setCursor(80, 100);
        tft.setTextSize(5);
        tft.setTextColor(tft.color565(color, color, color));
        tft.print(" Set..");
        delay(3);
    }
    delay(100);

    // Go!!!
    tft.setCursor(80, 100);
    tft.setTextSize(5);
    tft.setTextColor(tft.color565(255, 255, 255));
    tft.print(" Go!!!");
    delay(400);
      for (int color = 255; color > 1; color -= 1) {
        tft.setCursor(80, 100);
        tft.setTextSize(5);
        tft.setTextColor(tft.color565(color, color, color));
        tft.print(" Go!!!");
      }
    delay(100);
    state = 3;
  }
  
  // draw the goal
  int x_goal_pos[6] = {random(10, 305), random(10, 305), random(10, 305), random(10, 305), random(10, 305), random(10, 305)};
  int y_goal_pos[6] = {random(10, 225), random(10, 225), random(10, 225), random(10, 225), random(10, 225), random(10, 225)};
  int red_val[6] = {246, 255, 255, 77, 55, 72};
  int green_val[6] = {0, 140, 238, 233, 131, 21};
  int blue_val[6] = {0, 0, 0, 76, 255, 170};

  int x_goal_vel[6] = {0, 0, 0, 0, 0, 0};
  int y_goal_vel[6] = {0, 0, 0, 0, 0, 0};

  for (int goal = 0; goal < 6; goal++) {
    tft.drawRect(x_goal_pos[goal], y_goal_pos[goal], 5, 5, tft.color565(red_val[goal], green_val[goal], blue_val[goal]));
  }

  x_pos = 158;
  y_pos = 118;
  int target = 0;
  int ticks = 0;

  while (state == 3) {
    // player pixel movement
    // fetch the phone's accelerometer data
    readPacket(&ble, 50);
    if (packetbuffer[1] == 'A') {
      y_vel = -y_sens * parsefloat(packetbuffer+2);
      x_vel = -x_sens * parsefloat(packetbuffer+6);
    } else if (packetbuffer[1] == 'B');

  
    x_goal_vel[0] += goal_sens * random(-5, 6); y_goal_vel[0] += goal_sens * random(-5, 6);
    x_goal_vel[1] += goal_sens * random(-6, 7); y_goal_vel[1] += goal_sens * random(-6, 7);
    x_goal_vel[2] += goal_sens * random(-6, 7); y_goal_vel[2] += goal_sens * random(-6, 7);
    x_goal_vel[3] += goal_sens * random(-7, 8); y_goal_vel[3] += goal_sens * random(-7, 8);
    x_goal_vel[4] += goal_sens * random(-9, 10); y_goal_vel[4] += goal_sens * random(-9, 10);
    x_goal_vel[5] += goal_sens * random(-15, 16); y_goal_vel[5] += goal_sens * random(-15, 16);

    
    int size = 3;

    tft.fillRect(x_pos, y_pos, size, size, ILI9341_BLACK);
    for (int goal = 0; goal < 6; goal++) {
      tft.drawRect(x_goal_pos[goal], y_goal_pos[goal], 5, 5, ILI9341_BLACK);
    }
    // modify players position based on phone data (if not out of bounds); x and y are swapped due to orientation of screen
      if ((x_pos + x_vel < (320 - (10 + size)) && (x_pos + x_vel > (5 + size)))) {
          x_pos += x_vel;
      }
      if ((y_pos + y_vel < (240 - (10 + size)) && (y_pos + y_vel > (5 + size)))) {
          y_pos += y_vel;
      }

      for (int goal = 0; goal < 6; goal++) {
        if ((x_goal_pos[goal] + x_goal_vel[goal] < 305) && (x_goal_pos[goal] + x_goal_vel[goal] > 10)) {
          x_goal_pos[goal] += x_goal_vel[goal];
        } else {
          x_goal_vel[goal] = 0;
        }
        if ((y_goal_pos[goal] + y_goal_vel[goal] < 225) && (y_goal_pos[goal] + y_goal_vel[goal] > 10)) {
          y_goal_pos[goal] += y_goal_vel[goal];
        } else {
          y_goal_vel[goal] = 0;
        }
      }

        // draw player position
      tft.fillRect(x_pos, y_pos, size, size, ILI9341_YELLOW);
      for (int goal = 0; goal < 6; goal++) {
        tft.drawRect(x_goal_pos[goal], y_goal_pos[goal], 5, 5, tft.color565(red_val[goal], green_val[goal], blue_val[goal]));
      }
      

      if ( ((x_pos > x_goal_pos[target] - size) && (x_pos < x_goal_pos[target] + 5)) && ( (y_pos > y_goal_pos[target] - size) && (y_pos < y_goal_pos[target] + 5)) ) {
            tft.fillRect(0, 0, 320, 5, tft.color565(red_val[target], green_val[target], blue_val[target]));
            tft.fillRect(0, 0, 5, 240, tft.color565(red_val[target], green_val[target], blue_val[target]));
            tft.fillRect(315, 0, 5, 240, tft.color565(red_val[target], green_val[target], blue_val[target]));
            tft.fillRect(0, 235, 320, 5, tft.color565(red_val[target], green_val[target], blue_val[target]));
        red_val[target] = 0; green_val[target] = 0; blue_val[target] = 0;
        digitalWrite(WIN_PIN, HIGH);
        delay(50);
        digitalWrite(WIN_PIN, LOW);
        target++;

        // this player wins
        if (target == 6 && !digitalRead(P2_IN)){
          state = 1;

          // send signal to other player
          digitalWrite(P2_OUT, HIGH);
          
          // wait for other player to respond
          while (true) {
            // check to see if other player responded
            if (digitalRead(P2_IN)) {
              digitalWrite(P2_OUT, LOW);
              break;
            }
          }
          break;
        }
        // this player loses
        else if (digitalRead(P2_IN)) {
          digitalWrite(P2_OUT, HIGH);
          while (true) {
            // look to see if other player turns off pin
            if (!digitalRead(P2_IN)) {
              digitalWrite(P2_OUT, LOW);
              break;
            }
          }
        }
      }
      ticks++;
      }
    }
