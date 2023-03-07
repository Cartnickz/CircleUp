// Zach Cartnick & Brooke Abeles
// PHYS 336 - Electronics
// Digital Project
// Player 2

#include <ILI9341_t3.h>
#include <font_Arial.h> // from ILI9341_t3
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

#include <string.h>
#include <Arduino.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

// Communicate with Player 1 Teensy
#define P1_PIN 5

// Touchscreen Stuff
#define CS_PIN  8
#define TFT_DC  9
#define TFT_CS 10

// Bluetooth Stuff
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BLUEFRUIT_HWSERIAL_NAME      Serial1
Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, 14);
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
XPT2046_Touchscreen ts(CS_PIN);

// initialize program variables
float x_pos, y_pos;
float x_goal, y_goal;
float x, y;
float x_sens = 0.01;
float y_sens = 0.01;


void setup() {
  // Setup Teensy 
  while (!Serial);  // required for Flora & Micro
  delay(50);
  Serial.begin(115200);
  pinMode(P1_PIN, INPUT);

  // picking seed for random number
  randomSeed(42);

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
  uint8_t len = readPacket(&ble, 500);
  if (len == 0) return;

  printHex(packetbuffer, len);
  tft.fillRect(75, 100, 200, 50, ILI9341_BLACK);

  // starting screen
  while(state == 1) {
    tft.setCursor(80, 150);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Waiting for player 1......");

    for (int color = 0; color < 256; color += 20) {
      tft.setCursor(30, 100);
      tft.setTextSize(5);
      tft.setTextColor(tft.color565(0, color, 255-color));
      tft.print("CircleUp!");
      
      // look for player 1 to press start
      if (state == 1 && digitalRead(P1_PIN)) {
        state = 2;
        break;
      }


    }
    if (state == 1) {
      for (int color = 0; color < 256; color += 20) {
        tft.setCursor(30, 100);
        tft.setTextSize(5);
        tft.setTextColor(tft.color565(0, 255-color, color));
        tft.print("CircleUp!");

        // look for player 1 to press start
        if (state == 1 && digitalRead(P1_PIN)) {
        state = 2;
        break;
      }
      }
    }

  }

  while(state == 2) {
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
  x_goal = random(10, 305);
  y_goal = random(10, 225);
  tft.drawRect(x_goal, y_goal, 5, 5, ILI9341_GREEN);

  x_pos = 158;
  y_pos = 118;



  while(state == 3) {
    // player pixel movement
    // fetch the phone's accelerometer data
    readPacket(&ble, 200);
    if (packetbuffer[1] == 'A') {
      y = -y_sens * parsefloat(packetbuffer+2);
      x = -x_sens * parsefloat(packetbuffer+6);
    }
    
    for(int update = 0; update < 5000; update += 1) {

    // modify players position based on phone data (if not out of bounds); x and y are swapped due to orientation of screen
    if (abs(x) > 0.0003 || abs(y) > 0.0003) {
      tft.fillRect(x_pos, y_pos, 3, 3, ILI9341_BLACK);
      if ((x_pos + x < 310) && (x_pos + x > 8)) {
        x_pos += x;
      }
      if ((y_pos + y < 230) && (y_pos + y > 8)) {
        y_pos += y;
      }

        // draw player position
      tft.fillRect(x_pos, y_pos, 3, 3, ILI9341_YELLOW);
      if ( ((x_pos > x_goal - 3) && (x_pos < x_goal + 5)) && ( (y_pos > y_goal - 3) && (y_pos < y_goal + 5)) ) {
        state = 4;
        break;
      }
    }
    }
  }
}
