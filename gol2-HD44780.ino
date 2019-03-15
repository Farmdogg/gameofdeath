/*
 * gameoflife. A Conway's game of life implementation for Arduino and MAX7219 
 * controlled led matrix.
 * 
 * Copyright (C) 2014  Rafael Bailón-Ruiz <rafaelbailon "en" ieee "punto" org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Wire.h> 
#include <LedControl.h>
// #include <LCD.h>   // **NOTE** - This source file has been modified for Hitachi HD44780 family display drivers
#include <LiquidCrystal.h>  // **ENSURE PINOUT MATCHES OBJECT DECLARATION BELOW - arduino.cc/en/Reference/LiquidCrystal**


LedControl lc=LedControl(10,11,12,1);   // MAX72XX - DIN, CLK, LD/CS, # of ics
//LiquidCrystal_I2C lcd(0x27,20,4); // set the LCD address to 0x27 for a 16 chars and 2 line display
//LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the default I2C bus address of the backpack-see article
LiquidCrystal lcd(2, 3, 6, 7, 8, 9);  // **HD44780 family - Set pinouts here. See comments in include directives

unsigned long resetDwell = 1000;
unsigned long genLimit = 3000;
unsigned long delaytime = 90;

int NUMROWS = 8;
int NUMCOLS = 8;

long screenupdate = millis();
long looptime = micros();
long elapsedloop = 0;
unsigned long gen = 0;
unsigned long limitHits = 0;
boolean resetit = false;
unsigned long deadcells = 0;

int gameBoard[] =  { 2, 4, 7, 0, 0, 0, 0, 0 };
int newGameBoard[] =  { 0, 0, 0, 0, 0, 0, 0, 0 };
int oldGameBoard[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int olderGameBoard[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int oldestGameBoard[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int olderthanoldestGameBoard[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void setup()  {

  Serial.begin(9600);
  Serial.println("YOSPOS bithc");
  Serial.println();
  Serial.println("BORN TO DIE WORLD IS");  // uncomment for larger displays
  Serial.println("A FUCK Kill Em All");
    
  /* 
   * Note - I tried to tweak this so that the LEDs don't all go full bright at initial power on  
   * but I'm p. sure it can't be done in software with most common breakout boards. At least not with 
   * LedControl(). For now, I would not recommend having it right in front of your eyes when you plug the f%$$er in.
   * Yes I found out the hard way.
   * 
   *    -- Farmdizzle
   */
  lc.setIntensity(0,1);   // Set second number to 0-15 for LED brightness - varies directly
  lc.clearDisplay(0);
  lc.shutdown(0,false);   // Wake MAX72XX
  
  lcd.begin (16,2);   // Columns, Rows of LCD diplay
  //lcd.setBacklightPin(3,POSITIVE); // BL, BL_POL    // not in LedControl(), 220R resistor inline instead
  //lcd.setBacklight(HIGH);
  lcd.setCursor(0,0);
  //lcd.print("BORN TO DIE WORLD IS");  // uncomment for larger displays
  //lcd.setCursor(0,1);
  //lcd.print("A FUCK Kill Em All");
  //lcd.setCursor(0,2); */
  lcd.print("I am MATTYZCAST");
  lcd.setCursor(12,1);
  lcd.print("DEAD");   // CELLS eats up display space needed for counter
  delay(200);
  //resetMap();
}

void resetMap() {
  randomSeed(analogRead(0));
  //for (int q=5; q >= 0; q--) {    // Uncomment/tweak this for a fade out on map reset...
  //  lc.setIntensity(0,q);
  //  delay(200);
  //}
  delay(resetDwell);
  //lc.setIntensity(0,5);   // ...this too

  for(int x = 0; x < NUMROWS; ++x)
  {
    gameBoard[x] = random(256);
  }
}

void nextGeneration() {
  int up;
  
  for(int x = 0; x < NUMROWS; ++x)
  {

    for(int y = 0; y < NUMCOLS; ++y)
    {
      int sum = sumNeighbours(gameBoard, NUMROWS, x, y);
      if(bitRead(gameBoard[x],y) == 1) //If Cell is alive
      {
        if(sum < 2 || sum > 3) //Cell dies
        {
          bitClear(newGameBoard[x],y);
          deadcells++;
        }
        else
        {
          bitSet(newGameBoard[x],y);
        }
      }
      else //If Cell is dead
      {
        if(sum == 3) //A new Cell is born
        {
          bitSet(newGameBoard[x],y);
        }
      }
    }
  }
  if((compareArray(newGameBoard, gameBoard, NUMROWS) == 0) || (compareArray(newGameBoard, oldGameBoard, NUMROWS) == 0) || (compareArray(newGameBoard, olderGameBoard, NUMROWS) == 0) || (compareArray(newGameBoard, oldestGameBoard, NUMROWS) == 0) || (compareArray(newGameBoard, olderthanoldestGameBoard, NUMROWS) == 0))
  {
        deathcount();
        resetit = true;
    resetMap();
  }
  copyArray(oldestGameBoard, olderthanoldestGameBoard, NUMROWS);
  copyArray(olderGameBoard, oldestGameBoard, NUMROWS);
  copyArray(oldGameBoard, olderGameBoard, NUMROWS);
  copyArray(gameBoard, oldGameBoard, NUMROWS);
  copyArray(newGameBoard, gameBoard, NUMROWS);
  
  
}

int sumNeighbours(int matrix[], int matrixLength, int x, int y) {
  int sum = 0;
  
  for (int ix = -1; ix <=1; ++ix)
  {
    for (int iy = -1; iy <=1; ++iy)
    {
      if(x + ix < 0 || x + ix > matrixLength - 1 || y + iy < 0 || y + iy > 7 || (ix == 0 && iy == 0))
      {
        continue;
      }
      
      bitRead(matrix[x + ix], y + iy) == 1 ? ++sum : 0;
    }
  }
  
  return sum;
}

void copyArray(int *from, int *to, int length)  {
  for(int i = 0; i < length; ++i)
  {
    to[i] = from[i];
  }
}

int compareArray(int *first, int *second, int length) {
  for(int i = 0; i < length; ++i)
  {
    if(first[i] != second[i])   // Return 0 if they are equal, else 1
    {
      return 1;
    }
  }
  return 0;
}

void rows (boolean wait) {
  for(int row=0; row<8; row++)
  {
    lc.setRow(0,row,gameBoard[row]);
    
  }
  if(wait == true){
    delay (delaytime);
  }
}

void deathcount() {

  // Note - most of these function calls are modified for the HD44780
  lcd.setCursor(0,1);
  lcd.print("           ");
  lcd.setCursor(0,1);
  lcd.print(deadcells);
  lcd.setCursor(0,0);
  lcd.print("               G");
  lcd.setCursor(0,0);
  lcd.print(gen);

  Serial.print(deadcells);
  Serial.println(" DEAD CELLS");
  Serial.print(gen);
  Serial.println(" Generations");
  Serial.print(limitHits);
  Serial.println(" Probable Oscillators");
  Serial.print(millis());
  Serial.println("ms uptime");
  Serial.println();
}

void loop() { 
  
  if(resetit) {
    gen = 0;
    resetMap();
    resetit = false;
    rows(true);
  }
  else  {
    rows(true);
  }

  if(genLimit == 0 || gen <= genLimit)  {   
    nextGeneration();
    gen++;
  }
  else  {
    resetit = true;
    limitHits++;
  }
  
  if(screenupdate + 1000 <= millis()) {
    deathcount();
    screenupdate = millis();  
  }
  
  elapsedloop = micros() - looptime;
  looptime = micros();
}
