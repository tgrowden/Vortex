
#include <FastLED.h>
#include <FlashStorage.h>
#include "Modes.h"
#include "Buttons.h"

#include <Adafruit_DotStar.h>

#include <IRLibSendBase.h>
#include <IRLibDecodeBase.h>
#include <IRLib_P01_NEC.h>
#include <IRLibCombo.h>

#include <IRLibRecv.h>


#define NUM_LEDS 28
#define DATA_PIN 4
#define CLOCK_PIN 3

#define totalModes 20
#define totalPatterns 21
#define demoDuration 12000 // duration of each pattern in demo mode

unsigned long demoClock = 0;
bool demoModeActive = true;

//Objects
//---------------------------------------------------------

CRGB leds[NUM_LEDS];
CRGB boardlight[1];
Modes mode[totalModes];
Buttons button[2];

IRdecode myDecoder;
IRrecv myReceiver(2);
IRsend mySender;

typedef struct Orbit {
  bool dataIsStored;
  uint8_t sHue[totalModes][8];
  uint8_t sSat[totalModes][8];
  uint8_t sVal[totalModes][8];
  uint8_t sNumColors[totalModes];
  uint8_t sPatternNum[totalModes];
};
FlashStorage(saveData, Orbit);

//Variable
//---------------------------------------------------------

bool dataStored = 0;
bool on, on2, on3;
int m = 0;
byte menu;
byte stage = 0;
byte frame = 0;
byte qBand;
int gap;
int patNum;
byte dot = 0;
byte k = 0;
int targetSlot;
byte currentSlot;
int targetZone;
byte colorZone;
int targetHue;
byte selectedHue;
int targetSat;
byte selectedSat;
int targetVal;
byte selectedVal;
bool buttonState, buttonState2, lastButtonState, lastButtonState2 = 0;
unsigned long mainClock, prevTime, duration, prevTime2;
int data1[8];
int data2[8];
int data3[8];
bool received1, received2, received3;
int rep = 0;

unsigned long prevTime3, prevTime4;
unsigned long pressTime, prevPressTime, holdTime, prevHoldTime;

const byte numChars = 128;
char receivedChars[numChars];
char tempChars[numChars];

boolean newData = false;

int dataNumber = 0;

//Main body
//---------------------------------------------------------

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(20);
  button[0].createButton(19);
  button[1].createButton(20);
  setDefaults();
  loadSave();
  prevPressTime = 0;
  prevTime = 0;
  duration = 0;
  mode[m].menuNum = 0;
  randomSeed(analogRead(0));

  Adafruit_DotStar strip = Adafruit_DotStar(1, 7, 8, DOTSTAR_BGR);
  strip.begin();
  strip.show();
}

void loop() {
  menu = mode[m].menuNum;
  switch (menu) {
    case 0: {
      if (demoModeActive) {
        demoMode();
      } else {
        playMode();
      }
      break;
    }
    case 1: {
      openColors();
      break;
    }
    case 2: {
      colorSet();
      break;
    }
    case 3: {
      openPatterns();
      break;
    }
    case 4: {
      patternSelect();
      break;
    }
    case 5: {
      confirmBlink();
      break;
    }
    case 6: {
      randomizerRoll();
      break;
    }
    case 7: {
      shareMode();
      break;
    }
    case 8: {
      receiveMode();
      break;
    }
  }
  checkButton();
  checkSerial();
  FastLED.show();
  //Serial.println(m);
}

int hue, sat, val;

void playMode() {
  mainClock = millis();
  patterns(mode[m].patternNum);
  catchMode();
}

void demoMode() {
  if (millis() >= demoClock + demoDuration) {
      demoClock += demoDuration;

      m = m + 1 < totalModes ? m + 1 : 0;
    }
    playMode();
}

//Patterns
//---------------------------------------------------------

void patterns(int pat) {

  int totalColors = mode[m].numColors;
  int currentColor = mode[m].currentColor;
  int next = mode[m].nextColor;

  switch (pat) {
    case 1: { // All tracer
      getColor(0);
      setLeds(0, 27);
      if (on) {
        getColor(currentColor);
        if (totalColors == 1) val = 0;
        setLeds(0, 27);
        duration = 2;
      }
      if (!on) duration = 10;
      if (mainClock - prevTime > duration) {
        if (!on)nextColor(1);
        on = !on;
        prevTime = mainClock;
      }
      break;
    }

    case 2: { // SparkleTrace
      getColor(0);
      setLeds(0, 27);
      if (on) {
        getColor(currentColor);
        if (totalColors == 1) val = 0;
        setLed(random(0, 7));
        setLed(random(0, 7));
        setLed(random(7, 14));
        setLed(random(7, 14));
        setLed(random(14, 21));
        setLed(random(14, 21));
        setLed(random(21, 28));
        setLed(random(21, 28));
      }
      if (!on) nextColor (1);
      on = !on;
      break;
    }

    case 3: { // Vortex
      getColor(currentColor);
      if (mainClock - prevTime > 50) {
        clearAll();
        for (int side = 0; side < 4; side++) {
          if (frame <= 3) {
            setLed(3 + (7 * side) + frame);
            setLed(3 + (7 * side) - frame);
          }
          if (frame >= 4) {
            setLed(3 + (7 * side) + (6 - frame));
            setLed(3 + (7 * side) - (6 - frame));
          }
        }
        nextColor (0);
        frame++;
        if (frame > 6) frame = 0;
        prevTime = mainClock;
      }
      break;
    }

    case 4: { // Dot Zip
      getColor(currentColor);
      if (mainClock - prevTime > 5) {
        clearAll();
        qBand++;
        if (qBand > 6) qBand = 0;
        setLed(qBand);
        setLed(qBand + 7);
        setLed(qBand + 14);
        setLed(qBand + 21);
        nextColor (0);
        prevTime = mainClock;
      }
      break;
    }

    case 5: { // Cross strobe
      getColor(currentColor);
      if (mainClock - prevTime > 50) {
        clearAll();
        for (int s = 0; s < 7; s++) {
          if (on) {
            setLed(s);
            setLed(s + 14);
          }
          if (!on) {
            setLed(s + 7);
            setLed(s + 21);
          }
          nextColor (0);
        }

        on = !on;
        prevTime = mainClock;
      }
      break;
    }

    case 6: { // Impact
      if (mainClock - prevTime > 1) on = !on, prevTime = mainClock;
      getColor(0);
      if (on) {
        clearAll();
        setLeds(0, 2);
        setLeds(11, 17);
        setLeds(25, 27);
      }
      getColor(1);
      if (totalColors == 1) val = 0;
      if (!on) {
        clearAll();
        setLeds(4, 9);
        setLeds(18, 23);
      }
      getColor(currentColor);
      if (totalColors <= 2) val = 0;
      setLed(3), setLed(10), setLed(17), setLed(24);
      nextColor(2);
      break;
    }

    case 7: { // Blend
      getColor(currentColor);
      int color1 = mode[m].hue[currentColor];
      int color2 = mode[m].hue[next];
      if (color1 > color2 && color1 - color2 < (255 - color1) + color2)gap--;
      if (color1 > color2 && color1 - color2 > (255 - color1) + color2)gap++;
      if (color1 < color2 && color2 - color1 < (255 - color2) + color1)gap++;
      if (color1 < color2 && color2 - color1 > (255 - color2) + color1)gap--;
      if (color1 + gap >= 255) gap -= 255;
      if (color1 + gap < 0) gap += 255;
      int finalHue = color1 + gap;
      if (finalHue == color2) gap = 0, nextColor(0);
      for (int a = 0; a < 28; a++) leds[a].setHSV(finalHue, sat, val);
      break;
    }

    case 8: { // Chroma Crush
      getColor(currentColor);
      setLeds(0, 27);
      nextColor(0);

      break;
    }

    case 9: { // Dops Crush
      if (on) {
        clearAll();
        if (mainClock - prevTime > 7) {
          on = !on;
          prevTime = mainClock;
        }
      }
      if (!on) {
        getColor(currentColor);
        setLeds(0, 27);
        if (currentColor == totalColors - 1) on = !on;
        nextColor(0);
      }

      break;
    }

    case 10: { // Meteor
      for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(30);
      if (mainClock - prevTime > 10) {
        getColor(currentColor);
        setLed(random(0, 7));
        setLed(random(0, 7));
        setLed(random(7, 14));
        setLed(random(7, 14));
        setLed(random(14, 21));
        setLed(random(14, 21));
        setLed(random(21, 28));
        setLed(random(21, 28));
        nextColor (0);
        prevTime = mainClock;
      }

      break;
    }

    case 11: { //Carnival Chroma
      if (mainClock - prevTime > 200) {
        for (int i = 0; i < NUM_LEDS; i++) {
          if (i % 2 == 0) {
            getColor(currentColor);
            setLed(i);
          }
          if (i % 2 == 1) {
            getColor(currentColor + 1);
            if (currentColor + 1 == totalColors) {
              getColor(0);
            }
            setLed(i);
          }
        }
        nextColor(0);
        prevTime = mainClock;
      }

      break;
    }

    case 12: { // Vortex Wipe
      getColor(currentColor);
      if (mainClock - prevTime > 50) {
        for (int side = 0; side < 4; side++) {
          if (frame <= 3) {
            setLed(3 + (7 * side) + frame);
            setLed(3 + (7 * side) - frame);
          }
          if (frame >= 4) {
            setLed(3 + (7 * side) + (6 - (frame - 1)));
            setLed(3 + (7 * side) - (6 - (frame - 1)));
          }
        }
        if (frame == 3 || frame == 7) nextColor (0);
        frame++;
        if (frame > 7) {
          frame = 0;
        }
        prevTime = mainClock;
      }

      break;
    }

    case 13: { //Warp Worm
      getColor(0);
      setLeds(0, 27);
      getColor(currentColor);
      for (int i = 0; i < 5; i++) {
        int chunk = i + k;
        if (chunk > 27) chunk -= 28;
        setLed(chunk);
      }
      if (mainClock - prevTime > 15) {
        if (currentColor == totalColors - 1) k++;
        prevTime = mainClock;
        if (k > 27) k = 0;
        nextColor(1);
      }

      break;
    }

    case 14: { // MegaDops
      if (on) {
        getColor(currentColor);
        setLeds(0, 27);
        duration = 1;
      }
      if (!on) {
        clearAll();
        duration = 3;
      }
      if (mainClock - prevTime > duration) {
        if (!on)nextColor(0);
        on = !on;
        prevTime = mainClock;
      }

      break;
    }

    case 15: { // Warp Wipe
      if (mainClock - prevTime > 35) {
        getColor(currentColor);
        setLed(dot);
        dot++;
        if (dot >= NUM_LEDS) {
          dot = 0;
          nextColor(0);
        }
        prevTime = mainClock;
      }

      break;
    }

    case 16: { // Double Warp
      getColor(0);
      setLeds(0, 27);
      getColor(currentColor);
      for (int i = 0; i < 5; i++) {
        int chunk = i + k;
        if (chunk > 27) chunk -= 28;
        setLed(chunk);
        int otherChunk = i + k + 14;
        if (otherChunk > 27) otherChunk -= 28;
        setLed(otherChunk);
      }
      if (mainClock - prevTime > 5) {
        if (currentColor == totalColors - 1) k++;
        prevTime = mainClock;
        if (k > 27) k = 0;
        nextColor(1);
      }

      break;
    }

    case 17: { //Warp Fade
      if (mainClock - prevTime2 > 1) {
        for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(7);
        prevTime2 = mainClock;
      }
      if (mainClock - prevTime > 5) {
        getColor(currentColor);
        setLed(dot);
        dot++;
        if (dot >= NUM_LEDS) {
          dot = 0;
          nextColor(0);
        }
        prevTime = mainClock;
      }

      break;
    }

    case 18: { // Double fade
      if (mainClock - prevTime2 > 1) {
        for (int a = 0; a < NUM_LEDS; a++)leds[a].fadeToBlackBy(1);
        prevTime2 = mainClock;
      }
      if (mainClock - prevTime > 35) {
        getColor(currentColor);
        setLed(dot);
        dot++;
        int dot2 = dot + 13;
        if (dot2 > 27) dot2 -= 28;
        setLed(dot2);
        if (dot >= NUM_LEDS) {
          dot = 0;
          nextColor(0);
        }
        prevTime = mainClock;
      }

      break;
    }

    case 19: { //Bonus 1
      int pos;
      if (!on) {
        if (rep == 0) pos = 3;//1
        if (rep == 1) pos = 2;//0
        getColor(mode[m].currentColor);
        for (int side = 0; side < 4; side++) {
          setLed(3 + (7 * side) + pos);
          setLed(3 + (7 * side) - pos);
          setLed(3 + (7 * side) + (pos - 2));
          setLed(3 + (7 * side) - (pos - 2));
        }
        nextColor(0);
      }
      if (on) {
        if (rep == 0) pos = 3;
        if (rep == 1) pos = 2;
        for (int side = 0; side < 4; side++) {
          clearLight(3 + (7 * side) + pos);
          clearLight(3 + (7 * side) - pos);
          clearLight(3 + (7 * side) + (pos - 2));
          clearLight(3 + (7 * side) - (pos - 2));
        }
      }
      on = !on;

      getColor(0);
      if (rep == 1) pos = 3;
      if (rep == 0) pos = 2;
      for (int side = 0; side < 4; side++) {
        setLed(3 + (7 * side) + pos);
        setLed(3 + (7 * side) - pos);
        setLed(3 + (7 * side) + (pos - 2));
        setLed(3 + (7 * side) - (pos - 2));
      }
      if (on2) {
        getColor(mode[m].currentColor1);
        if (mode[m].numColors == 1) val = 0;
        if (rep == 1) pos = 3;
        if (rep == 0) pos = 2;
        duration = 1;
        for (int side = 0; side < 4; side++) {
          setLed(3 + (7 * side) + pos);
          setLed(3 + (7 * side) - pos);
          setLed(3 + (7 * side) + (pos - 2));
          setLed(3 + (7 * side) - (pos - 2));
        }
      }
      if (!on2) duration = 10;
      if (mainClock - prevTime2 > duration) {
        if (!on)nextColor1(1);
        on2 = !on2;
        prevTime2 = mainClock;
      }
      if (mainClock - prevTime4 > 500) {
        rep++;
        if (rep > 1) rep = 0;
        prevTime4 = mainClock;
      }

      break;
    }

    case 20: { // Bonus 2
      int pos;
      if (!on3) {
        getColor(mode[m].currentColor);
        if (rep == 0) pos = 3;
        if (rep == 1) pos = 2;
        for (int side = 0; side < 4; side++) {
          setLed(3 + (7 * side) + pos);
          setLed(3 + (7 * side) - pos);
          setLed(3 + (7 * side) + (pos - 2));
          setLed(3 + (7 * side) - (pos - 2));
        }
      }
      if (on3) {
        if (rep == 0) pos = 3;
        if (rep == 1) pos = 2;
        for (int side = 0; side < 4; side++) {
          clearLight(3 + (7 * side) + pos);
          clearLight(3 + (7 * side) - pos);
          clearLight(3 + (7 * side) + (pos - 2));
          clearLight(3 + (7 * side) - (pos - 2));
        }
      }
      if (mainClock - prevTime3 > 100) {
        on3 = !on3;
        if (!on3) nextColor(0);
        prevTime3 = mainClock;
      }

      if (mainClock - prevTime > 5) {
        getColor(mode[m].currentColor1);
        if (rep == 0) pos = 2;
        if (rep == 1) pos = 3;
        for (int side = 0; side < 4; side++) {
          setLed(3 + (7 * side) + pos);
          setLed(3 + (7 * side) - pos);
          setLed(3 + (7 * side) + (pos - 2));
          setLed(3 + (7 * side) - (pos - 2));
        }
        nextColor1(0);
        prevTime = mainClock;
      }
      if (mainClock - prevTime4 > 500) {
        rep++;
        if (rep > 1) rep = 0;
        prevTime4 = mainClock;
      }

      break;
    }

    default: { // All Ribbon - executed if pat == 0 or out-of-range
      if (mainClock - prevTime > 10) {
        getColor(currentColor);
        setLeds(0, 27);
        nextColor(0);
        prevTime = mainClock;
      }
    }
  }
}

//Led controlls for running patterns
//-----------------------------------------------------

void getColor(int target) {
  hue = mode[m].hue[target];
  sat = mode[m].sat[target];
  val = mode[m].val[target];
}

void setLed(int target) {
  leds[target].setHSV(hue, sat, val);
}

void setLeds(int first, int last) {
  for (int a = first; a <= last; a++) setLed(a);
}

void nextColor(int start) {
  mode[m].currentColor++;
  if (mode[m].currentColor >= mode[m].numColors) mode[m].currentColor = start;
  mode[m].nextColor = mode[m].currentColor + 1;
  if (mode[m].nextColor >= mode[m].numColors) mode[m].nextColor = start;
}

void nextColor1(int start) {
  mode[m].currentColor1++;
  if (mode[m].currentColor1 >= mode[m].numColors) mode[m].currentColor1 = start;
  mode[m].nextColor1 = mode[m].currentColor1 + 1;
  if (mode[m].nextColor1 >= mode[m].numColors) mode[m].nextColor1 = start;
}

void clearAll() {
  for (int a = 0; a < 28; a++) leds[a].setHSV(0, 0, 0);
}

void clearLight(int lightNum) {
  leds[lightNum].setHSV(0, 0, 0);
}

void blinkTarget(unsigned long blinkTime) {
  mainClock = millis();
  if (mainClock - prevTime > blinkTime) {
    on = !on;
    prevTime = mainClock;
  }
}

void resetColors(){
  for(int r = 0; r < 4; r++){
   mode[m].currentColor = 0;
  }
}

//Menus and settings
//---------------------------------------------------------

void openColors() {
  mainClock = millis();
  if (mainClock - prevTime > 75) {
    clearAll();
    for (int side = 0; side < 4; side++) {
      if (frame >= 0 && frame <= 3) {
        leds[3 + (7 * side) + frame].setHSV(0, 0, 170);
        leds[3 + (7 * side) - frame].setHSV(0, 0, 170);
      }
      if (frame >= 4 && frame <= 6) {
        leds[3 + (7 * side) + (6 - frame)].setHSV(0, 0, 170);
        leds[3 + (7 * side) - (6 - frame)].setHSV(0, 0, 170);
      }
    }
    frame++;
    if (frame > 6) frame = 0;
    prevTime = mainClock;
  }
}

void colorSet() {
  if (stage == 0) {
    int setSize = mode[m].numColors;
    clearAll();
    for (int colorSlot = 0; colorSlot < 8; colorSlot++) {
      int side = colorSlot / 2;
      leds[colorSlot + (5 * side)].setHSV(0, 0, 1);
      leds[((2 + (3 * side)) * 3) - colorSlot].setHSV(0, 0, 1);
    }
    for (int colorNum = 0; colorNum < setSize; colorNum++) {
      getColor(colorNum);
      int side = colorNum / 2;
      leds[(5 * side) + colorNum].setHSV(hue, sat, val);
      leds[((2 + (3 * side)) * 3) - colorNum].setHSV(hue, sat, val);
    }
    if (targetSlot < setSize) {
      if (on) {
        int side = targetSlot / 2;
        int blinkVal = 0;
        if (mode[m].val[targetSlot] == 0)blinkVal = 5;
        leds[targetSlot + (5 * side)].setHSV(0, 0, blinkVal);
        leds[((2 + (3 * side)) * 3) - targetSlot].setHSV(0, 0, blinkVal);
      }
      blinkTarget(300);
    }
    if (targetSlot == setSize) {
      if (on) {
        int side = (setSize - 1) / 2;
        leds[(5 * side) + (setSize - 1)].setHSV(0, 0, 1);
        leds[((2 + (3 * side)) * 3) - (setSize - 1)].setHSV(0, 0, 1);
        for (int side = 0; side < 4; side++) {
          leds[2 + (7 * side)].setHSV(0, 0, 1);
          leds[4 + (7 * side)].setHSV(0, 0, 1);
        }
      }
      blinkTarget(60);
    }
    if (targetSlot > setSize) {
      if (on) {
        int targLed = targetSlot - 1;
        int side = targLed / 2;
        leds[targLed + (5 * side)].setHSV(0, 0, 0);
        leds[((2 + (3 * side)) * 3) - targLed].setHSV(0, 0, 0);
      }
      blinkTarget(300);
    }
  }
  if (stage == 1) colorWheel(0);
  if (stage == 2) colorWheel(1);
  if (stage == 3) colorWheel(2);
  if (stage == 4) colorWheel(3);
}

void colorWheel(int layer) {
  int hue = 0, sat = 255, val = 170;
  if (layer == 0) {
    for (int color = 0; color < 16; color++) {
      int side = color / 4;
      hue = color * 16;
      if (color >= 0 + 4 * side && color <= 3 + side * 4) {
        leds[(side * 3) + color].setHSV(hue, sat, val);
        leds[((side * 11) + 6) - color].setHSV(hue, sat, val);
      }
    }
  }
  if (layer == 1) {
    for (int shade = 0; shade < 4; shade++) {
      hue = (shade * 16) + (64 * colorZone);
      for (int band = 0; band < 7; band++)leds[band + (7 * shade)].setHSV(hue, sat, val);
    }
  }
  if (layer == 2) {
    for (int fade = 0; fade < 4; fade++) {
      sat = 255 - (85 * fade);
      for (int band = 0; band < 7; band++)leds[band + (7 * fade)].setHSV(selectedHue, sat, val);
    }
  }
  if (layer == 3) {
    for (int bright = 0; bright < 4; bright ++) {
      val = 255 - (85 * bright);
      if (bright == 2) val = 120;
      for (int band = 0; band < 7; band++)leds[band + (7 * bright)].setHSV(selectedHue, selectedSat, val);
    }
  }
  if (on) {
    if (layer == 0)for (int band = 0; band < 7; band++)leds[band + (7 * targetZone)].setHSV(0, 0, 0);
    if (layer == 1)for (int band = 0; band < 7; band++)leds[band + (7 * targetHue)].setHSV(0, 0, 0);
    if (layer == 2)for (int band = 0; band < 7; band++)leds[band + (7 * targetSat)].setHSV(0, 0, 0);
    if (layer == 3)for (int band = 0; band < 7; band++)leds[band + (7 * targetVal)].setHSV(0, 0, 0);
    if (layer == 3 && targetVal == 3) for (int band = 0; band < 7; band++)leds[band + (7 * targetVal)].setHSV(0, 0, 1);
  }
  blinkTarget(300);
}

void openPatterns() {
  mainClock = millis();
  if (mainClock - prevTime > 100) {
    for (int a = 0; a < 28; a++) {
      leds[a].setHSV(0, 0, 110);
    }
    if (on) {
      clearAll();
    }
    on = !on;
    prevTime = mainClock;
  }
}

void patternSelect() {
  mainClock = millis();
  patterns(patNum);
}

void confirmBlink() {
  mainClock = millis();
  if (mainClock - prevTime > 50) {
    if (frame == 0) clearAll();
    if (frame == 1) sat = 0, setLeds(0, 27);
    if (frame == 2) clearAll();
    if (frame == 3) frame = 0, mode[m].menuNum = 0;
    frame++;
    prevTime = mainClock;
  }
}

// Randomizer
//---------------------------------------------------------


void randomizerRoll() {
  clearAll();
  for (int q = 0; q < 4; q++) {
    leds[7 * (q) + 0].setHSV(0, 0, 110);
    leds[7 * (q) + 6].setHSV(0, 0, 110);
  }
  if (button[0].holdTime > 1100) {
    for (int q = 0; q < 4; q++) {
      leds[7 * (q) + 1].setHSV(0, 0, 110);
      leds[7 * (q) + 5].setHSV(0, 0, 110);
    }
  }
  if (button[0].holdTime > 1400) {
    for (int q = 0; q < 4; q++) {
      leds[7 * (q) + 2].setHSV(0, 0, 110);
      leds[7 * (q) + 4].setHSV(0, 0, 110);
    }
  }
  if (button[0].holdTime > 1800) {
    for (int q = 0; q < 4; q++) {
      leds[7 * (q) + 3].setHSV(0, 0, 110);
    }
  }
}

void progressModes(int i) {
  if (demoModeActive) {
    demoModeActive = false;
  } else {
    m += i;
  }
  if (m > totalModes) {
    m = 0;
  } else if (m < 0) {
    m = totalModes - 1;
  }
  frame = 0;
  gap = 0;
  resetColors();
}

//Buttons
//---------------------------------------------------------

void checkButton() {
  for (int b = 0; b < 2; b++) {
    button[b].buttonState = digitalRead(button[b].pinNum);
    if (button[b].buttonState == LOW && button[b].lastButtonState == HIGH && (millis() - button[b].pressTime > 200)) {
      button[b].pressTime = millis();
    }
    button[b].holdTime = (millis() - button[b].pressTime);
    if (button[b].holdTime > 50) {
      if (button[b].buttonState == LOW && button[b].holdTime > button[b].prevHoldTime) {
        if (b == 0) {
          if (button[b].holdTime > 800 && button[b].holdTime <= 2000 && menu == 0) mode[m].menuNum = 6;
          if (button[b].holdTime > 2000 && button[b].holdTime < 3000 && menu == 6) mode[m].menuNum = 1;
          if (button[b].holdTime > 3000 && menu == 1) mode[m].menuNum = 7;
        }
        if (b == 1) {
          if (button[b].holdTime > 2000 && button[b].holdTime < 3000 && menu == 0) mode[m].menuNum = 3;
          if (button[b].holdTime > 3000 && menu == 3) mode[m].menuNum = 8, mode[m].currentColor = 0;
        }
      }
      if (button[b].buttonState == HIGH && button[b].lastButtonState == LOW && millis() - button[b].prevPressTime > 200) {
        if (button[b].holdTime < 300) {
          if (b == 0) {
            if (menu == 0) progressModes(1);
            if (menu == 2) {
              if (stage == 0) targetSlot++; //next option
              if (stage == 1) targetZone++;
              if (stage == 2) targetHue++;
              if (stage == 3) targetSat++;
              if (stage == 4) targetVal++;
            }
            if (menu == 4)patNum++, frame = 0, resetColors();
          }
          if (b == 1) {
            if (menu == 0) progressModes(-1);
            if (menu == 2) {
              if (stage == 0) targetSlot--; //previous option
              if (stage == 1) targetZone--;
              if (stage == 2) targetHue--;
              if (stage == 3) targetSat--;
              if (stage == 4) targetVal--;
            }
            if (menu == 4)patNum--, frame = 0, resetColors(); //previous option
            //if (menu == 7) mode[m].menuNum = 5;
          }
        }
        if (button[b].holdTime > 400 && button[b].holdTime < 3000) {
          //medium press
          if (b == 0) {
            if (button[b].holdTime > 800 &&  button[b].holdTime <= 2000) {
              if (menu == 6) {
                mode[m] = getRandomMode();
                saveAll();
                frame = 0;
                mode[m].menuNum = 5;
              }
            }
            if (menu == 2) {
              if (stage == 0) {
                int setSize = mode[m].numColors;
                if (targetSlot < setSize)stage = 1, currentSlot = targetSlot; //confirm selection
                if (targetSlot == setSize && mode[m].numColors > 1)targetSlot--, mode[m].numColors--;
                if (targetSlot > setSize)stage = 1, currentSlot = setSize;
              }
              else if (stage == 1) stage = 2, colorZone = targetZone;
              else if (stage == 2) stage = 3, selectedHue = (targetHue * 16) + (colorZone * 64);
              else if (stage == 3) stage = 4, selectedSat = 255 - (85 * targetSat);
              else if (stage == 4) {
                selectedVal = 255 - (85 * targetVal);
                if (targetVal == 2) selectedVal = 120;
                mode[m].saveColor(currentSlot, selectedHue, selectedSat, selectedVal);
                stage = 0;
              }
            }
            if (menu == 4) {
              mode[m].patternNum = patNum, saveAll(), frame = 0, mode[m].menuNum = 5;//confirm selection
            }
          }
          if (b == 1) {
            if (menu == 2) {
              if (stage == 0)mode[m].currentColor = 0, resetColors(), saveAll(), mode[m].menuNum = 0;//cancle exit
              if (stage == 1)stage = 0;
              if (stage == 2)stage = 1;
              if (stage == 3)stage = 2;
              if (stage == 4)stage = 3;
            }
            if (menu == 4)mode[m].menuNum = 5, frame = 0;//cancle exit
            if (menu == 7)mode[m].menuNum = 5, frame = 0;
            if (menu == 8)mode[m].menuNum = 5, frame = 0;
          }
        }
        if (button[b].holdTime > 300 && Serial) exportSettings();
        if (button[b].holdTime < 3000 && menu == 1)mode[m].menuNum = 2;
        if (button[b].holdTime < 3000 && menu == 3)mode[m].menuNum = 4, mode[m].currentColor = 0;
        button[b].prevPressTime = millis();
      }
    }
    //these are the max and minimum values for each variable.
    if (patNum > totalPatterns - 1) patNum = 0;
    if (patNum < 0) patNum = totalPatterns - 1;
    int lastSlot = mode[m].numColors + 1;
    if (mode[m].numColors == 8) lastSlot = mode[m].numColors;
    if (targetSlot > lastSlot) targetSlot = 0;
    if (targetSlot < 0) targetSlot = lastSlot;
    if (targetZone > 3) targetZone = 0;
    if (targetZone < 0) targetZone = 3;
    if (targetHue > 3) targetHue = 0;
    if (targetHue < 0) targetHue = 3;
    if (targetSat > 3) targetSat = 0;
    if (targetSat < 0) targetSat = 3;
    if (targetVal > 3) targetVal = 0;
    if (targetVal < 0) targetVal = 3;
    if (m < 0)m = totalModes - 1;
    if (m > totalModes - 1)m = 0;
    button[b].lastButtonState = button[b].buttonState;
    button[b].prevHoldTime = button[b].holdTime;
  }
}

//Saving/Loading
//---------------------------------------------------------

void loadSave() {
  Orbit myOrbit;
  myOrbit = saveData.read();
  if (myOrbit.dataIsStored == true) {
    for (int modes = 0; modes < totalModes; modes ++) {
      mode[modes].patternNum = myOrbit.sPatternNum[modes];
      mode[modes].numColors = myOrbit.sNumColors[modes];
      for (int c = 0; c < mode[modes].numColors; c++) {
        mode[modes].hue[c] = myOrbit.sHue[modes][c];
        mode[modes].sat[c] = myOrbit.sSat[modes][c];
        mode[modes].val[c] = myOrbit.sVal[modes][c];
      }
    }
  }
}
void saveAll() {
  Orbit myOrbit;
  for (int modes = 0; modes < totalModes; modes ++) {
    myOrbit.sPatternNum[modes] = mode[modes].patternNum;
    myOrbit.sNumColors[modes] = mode[modes].numColors;
    for (int c = 0; c < mode[modes].numColors; c++) {
      myOrbit.sHue[modes][c] = mode[modes].hue[c];
      myOrbit.sSat[modes][c] = mode[modes].sat[c];
      myOrbit.sVal[modes][c] = mode[modes].val[c];
    }
  }
  myOrbit.dataIsStored = true;
  saveData.write(myOrbit);
}

//IR commuinication
//---------------------------------------------------------
void shareMode() {
  //confirmBlink();
  if (on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(128, 255, 100);
  }
  if (!on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(0, 0, 0);
  }
  blinkTarget(1);
  unsigned long shareBit;
  for (int s = 0; s < 8; s++) {
    Serial.print(mode[m].hue[s]);
    Serial.print(" ");
  }
  Serial.println();
  shareBit = (((unsigned long)mode[m].hue[0] / 16 ) * (unsigned long)0x10000000) +
             (((unsigned long)mode[m].hue[1] / 16 ) * (unsigned long)0x1000000) +
             (((unsigned long)mode[m].hue[2] / 16 ) * (unsigned long)0x100000) +
             (((unsigned long)mode[m].hue[3] / 16 ) * (unsigned long)0x10000) +
             (((unsigned long)mode[m].hue[4] / 16 ) * (unsigned long)0x1000) +
             (((unsigned long)mode[m].hue[5] / 16 ) * (unsigned long)0x100) +
             (((unsigned long)mode[m].hue[6] / 16 ) * (unsigned long)0x10) +
             (unsigned long)1;
  /*mode[m].hue[0]*/
  //Serial.println(shareBit);
  Serial.println(shareBit, HEX);
  mySender.send(NEC, shareBit, 0);
  int sendSat[8];
  int sendVal[8];
  for (int n = 0; n < 8; n++) {
    if (mode[m].sat[n] >= 0 && mode[m].sat[n] < 85) sendSat[n] = 0;
    if (mode[m].sat[n] >= 85 && mode[m].sat[n] < 170) sendSat[n] = 1;
    if (mode[m].sat[n] >= 170 && mode[m].sat[n] < 255)sendSat[n] = 2;
    if (mode[m].sat[n] == 255)sendSat[n] = 3;

    if (mode[m].val[n] >= 0 && mode[m].val[n] < 85) sendVal[n] = 0;
    if (mode[m].val[n] >= 85 && mode[m].val[n] < 170) sendVal[n] = 1;
    if (mode[m].val[n] >= 170 && mode[m].val[n] < 255)sendVal[n] = 2;
    if (mode[m].val[n] == 255)sendVal[n] = 3;
    //Serial.print(sendVal[n]);
    //Serial.print(" ");
  }
  //Serial.println();
  shareBit = ((unsigned long)mode[m].hue[7] * (unsigned long)0x1000000) +
             ((unsigned long)((sendSat[0] * 0x4) + sendSat[1]) * (unsigned long)0x1000000) +
             ((unsigned long)((sendSat[2] * 0x4) + sendSat[3]) * (unsigned long)0x100000) +
             ((unsigned long)((sendSat[4] * 0x4) + sendSat[5]) * (unsigned long)0x10000) +
             ((unsigned long)((sendSat[6] * 0x4) + sendSat[7]) * (unsigned long)0x1000) +
             ((unsigned long)((sendVal[0] * 0x4) + sendVal[1]) * (unsigned long)0x100) +
             ((unsigned long)((sendVal[2] * 0x4) + sendVal[3]) * (unsigned long)0x10) +
             (unsigned long)2;
  Serial.println(shareBit, HEX);
  mySender.send(NEC, shareBit, 0);
  shareBit = ((unsigned long)((sendVal[4] * 0x4) + sendVal[5]) * (unsigned long)0x10000000) +
             ((unsigned long)((sendVal[6] * 0x4) + sendVal[7]) * (unsigned long)0x1000000) +
             ((unsigned long)mode[m].patternNum * (unsigned long)0x10000) +
             ((unsigned long)mode[m].numColors * (unsigned long)0x1000) +
             (unsigned long)3;
  Serial.println(shareBit, HEX);
  mySender.send(NEC, shareBit, 0);
}

void receiveMode() {
  if (on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(128, 255, 100);
  }
  if (!on) {
    for (int d = 0; d < 28; d++) leds[d].setHSV(0, 0, 0);
  }
  blinkTarget(750);
  myReceiver.enableIRIn();
  if (myReceiver.getResults()) {
    myDecoder.decode();           //Decode it
    if (myDecoder.value != 0) {
      //myDecoder.dumpResults(false);  //Now print results. Use false for less detail
      unsigned long value = myDecoder.value;
      if (hexValue(0, value) == 1) {
        for (int f = 0; f < 8; f++) {
          data1[f] = hexValue(f, value);
          received1 = true;
        }
        Serial.println("data1");
      }
      if (hexValue(0, value) == 2) {
        for (int f = 0; f < 8; f++) {
          data2[f] = hexValue(f, value);
          received2 = true;
        }
        Serial.println("data2");
      }
      if (hexValue(0, value) == 3) {
        for (int f = 0; f < 8; f++) {
          data3[f] = hexValue(f, value);
          received3 = true;
        }
        Serial.println("data3");
      }
      //for (int f = 0; f < 8; f++) {
      //  Serial.print(hexValue(f, value), HEX);
      //  Serial.print(" ");
      //}
      //Serial.println();
      //Serial.println(value);
      Serial.println(value, HEX);
      myReceiver.enableIRIn();      //Restart receiver
    }
  }
  if (received1 && received2 && received3) {
    Serial.println("data received");
    mode[m].hue[0] = data1[7] * 16;
    mode[m].hue[1] = data1[6] * 16;
    mode[m].hue[2] = data1[5] * 16;
    mode[m].hue[3] = data1[4] * 16;
    mode[m].hue[4] = data1[3] * 16;
    mode[m].hue[5] = data1[2] * 16;
    mode[m].hue[6] = data1[1] * 16;
    mode[m].hue[7] = data2[7] * 16;
    mode[m].sat[0] = (data2[6] / 4) * 85;
    mode[m].sat[1] = (data2[6] % 4) * 85;
    mode[m].sat[2] = (data2[5] / 4) * 85;
    mode[m].sat[3] = (data2[5] % 4) * 85;
    mode[m].sat[4] = (data2[4] / 4) * 85;
    mode[m].sat[5] = (data2[4] % 4) * 85;
    mode[m].sat[6] = (data2[3] / 4) * 85;
    mode[m].sat[7] = (data2[3] % 4) * 85;
    mode[m].val[0] = (data2[2] / 4) * 85;
    mode[m].val[1] = (data2[2] % 4) * 85;
    mode[m].val[2] = (data2[1] / 4) * 85;
    mode[m].val[3] = (data2[1] % 4) * 85;
    mode[m].val[4] = (data3[7] / 4) * 85;
    mode[m].val[5] = (data3[7] % 4) * 85;
    mode[m].val[6] = (data3[6] / 4) * 85;
    mode[m].val[7] = (data3[6] % 4) * 85;
    mode[m].patternNum = (data3[5] * 16) + data3[4];
    mode[m].numColors = data3[3];
    received1 = false;
    received2 = false;
    received3 = false;
    mode[m].menuNum = 0;
    saveAll();
  }
}

unsigned long hexValue(int place, unsigned long number) {
  for (int p = 0; p < place; p++) number /= 0x10;
  return number % 0x10;
}

void throwMode() {
  unsigned long shareBit;
  shareBit = (m * (unsigned long)0x10000000) +
             (0 * (unsigned long)0x1000000) +
             (0 * (unsigned long)0x100000) +
             (0 * (unsigned long)0x10000) +
             (0 * (unsigned long)0x1000) +
             (0 * (unsigned long)0x100) +
             (0 * (unsigned long)0x10) +
             (unsigned long)4;
  mySender.send(NEC, shareBit, 0);
}

void catchMode() {
  myReceiver.enableIRIn();
  if (myReceiver.getResults()) {
    myDecoder.decode();           //Decode it
    if (myDecoder.value != 0) {
      //myDecoder.dumpResults(false);  //Now print results. Use false for less detail
      unsigned long value = myDecoder.value;
      if (hexValue(0, value) == 4) {
        m = hexValue(7, value);
        Serial.println("Mode Change Caught");
      }
      myReceiver.enableIRIn();      //Restart receiver
    }
  }
}

//Serial Mode Transfer (usb)
//---------------------------------------------------------

void exportSettings() {
  Serial.println("Each line below contains 1 mode, copy and paste them to the line above to upload it!");
  for (int mo = 0; mo < totalModes; mo++) {
    Serial.print("<");
    Serial.print(mo);
    Serial.print(", ");
    Serial.print(mode[mo].patternNum);
    Serial.print(", ");
    Serial.print(mode[mo].numColors);
    Serial.print(", ");
    for (int co = 0; co < 8; co++) {
      Serial.print(mode[mo].hue[co]);
      Serial.print(", ");
      Serial.print(mode[mo].sat[co]);
      Serial.print(", ");
      Serial.print(mode[mo].val[co]);
      if (co != 7) Serial.print(", ");
    }
    Serial.println(">");
  }
}

void checkSerial() {
  recvWithStartEndMarkers();
  importData();
}

void recvWithStartEndMarkers() {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  if (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress == true) {
      if (rc != endMarker) {
        receivedChars[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
      else {
        receivedChars[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = true;
      }
    }

    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

void importData() {
  bool dataIsValid = false;
  char * strtokIndx; // this is used by strtok() as an index
  if (newData == true) {
    newData = false;
    if (!dataIsValid) {
      strcpy(tempChars, receivedChars);
      strtokIndx = strtok(tempChars, ",");
      if (atoi(strtokIndx) >= totalModes) {
        Serial.println("Invalid input. Mode number: too high");
        return;
      }
      strtokIndx = strtok(NULL, ",");
      if (atoi(strtokIndx) >= totalPatterns) {
        Serial.println("Invalid input. Pattern number: too high");
        return;
      }
      strtokIndx = strtok(NULL, ",");
      if (atoi(strtokIndx) < 1) {
        Serial.println("Invalid input. Number of colors: too low");
        return;
      }
      if (atoi(strtokIndx) > 8) {
        Serial.println("Invalid input. Number of colors: too high");
        return;
      }
      for (int col = 0; col < 8; col++) {
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Hue " + (String)col + ": too high");
          return;
        }
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Saturation " + (String)col + ": too high");
          return;
        }
        strtokIndx = strtok(NULL, ",");
        if (atoi(strtokIndx) > 255) {
          Serial.println("Invalid input. Brightness " + (String)col + ": too high");
          return;
        }
      }
      dataIsValid = true;
    }
    if (dataIsValid) {
      strcpy(tempChars, receivedChars);
      strtokIndx = strtok(tempChars, ",");
      int mNum = atoi(strtokIndx);
      strtokIndx = strtok(NULL, ",");
      mode[mNum].patternNum = atoi(strtokIndx);
      strtokIndx = strtok(NULL, ",");
      mode[mNum].numColors = atoi(strtokIndx);
      for (int col = 0; col < 8; col++) {
        strtokIndx = strtok(NULL, ",");
        mode[mNum].hue[col] = atoi(strtokIndx);
        strtokIndx = strtok(NULL, ",");
        mode[mNum].sat[col] = atoi(strtokIndx);
        strtokIndx = strtok(NULL, ",");
        mode[mNum].val[col] = atoi(strtokIndx);
      }
      Serial.println("Data recieved");
      saveAll();
      dataIsValid = false;
    }
  }
}

Modes getRandomMode() {
  int type = random(0, 10);

  Modes newMode; // a new mode to be returned

  newMode.patternNum = random(0, totalPatterns);

  switch (type) {
    // true random
    case 0: {
      newMode.numColors = random(1, 8);

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.hue[i] = random(0, 16) * 16;
        newMode.sat[i] = random(0, 4) * 85;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // monochrome
    case 1: {
      newMode.numColors = 4;

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.hue[i] = random(0, 16) * 16;;
        newMode.sat[i] = i * 85;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // complimentary
    case 2: {
      newMode.numColors = 2;

      int tempHue = random(0, 16) * 16;
      int compHue = tempHue + 128;

      if (compHue >= 255) {
        compHue -= 256;
      }
      newMode.hue[0] = tempHue;
      newMode.hue[1] = compHue;

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // analogous
    case 3: {
      newMode.numColors = 3;
      int tempHue = random(0, 16) * 16;

      int analHue1 = tempHue - 16; // hah. "anal"
      if (analHue1 < 0) {
        analHue1 += 256;
      }

      int analHue2 = tempHue + 16;
      if (analHue2 > 255) {
        analHue2 -= 256;
      }

      newMode.hue[0] = tempHue;
      newMode.hue[1] = analHue1;
      newMode.hue[2] = analHue2;

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // triadic
    case 4: {
      newMode.numColors = 3;
      int tempHue = random(0, 16) * 16;
      int triadHue1 = tempHue + 80;
      int triadHue2 = tempHue - 80;
      if (triadHue1 > 255) {
        triadHue1 -= 256;
      }
      if (triadHue2 < 0) {
        triadHue2 += 256;
      }
      newMode.hue[0] = tempHue;
      newMode.hue[1] = triadHue1;
      newMode.hue[2] = triadHue2;
      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // split complimentary
    case 5: {
      newMode.numColors = 3;
      int tempHue = random(0, 16) * 16;
      int splitCompHue1 = tempHue + 112;
      int splitCompHue2 = tempHue - 112;
      if (splitCompHue1 > 255) splitCompHue1 += 256;
      if (splitCompHue2 < 0) splitCompHue2 += 256;
      newMode.hue[0] = tempHue;
      newMode.hue[1] = splitCompHue1;
      newMode.hue[2] = splitCompHue2;

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // tetradic
    case 6: {
      newMode.numColors = 4;
      int tempHue = random(0, 16) * 16;
      int tetradHue1 = tempHue + 48;
      int tetradHue2 = tempHue + 128;
      int tetradHue3 = tempHue + 208;
      if (tetradHue1 > 255) tetradHue1 -= 256;
      if (tetradHue2 > 255) tetradHue2 -= 256;
      if (tetradHue3 > 255) tetradHue3 -= 256;
      newMode.hue[0] = tempHue;
      newMode.hue[1] = tetradHue1;
      newMode.hue[2] = tetradHue2;
      newMode.hue[3] = tetradHue3;

      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // square
    case 7: {
      newMode.numColors = 4;
      int tempHue = random(0, 16) * 16;
      int tetradHue1 = tempHue + 64;
      int tetradHue2 = tempHue + 128;
      int tetradHue3 = tempHue + 192;
      if (tetradHue1 > 255) tetradHue1 -= 256;
      if (tetradHue2 > 255) tetradHue2 -= 256;
      if (tetradHue3 > 255) tetradHue2 -= 256;
      for (int i = 0; i < newMode.numColors; i++) {
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // full rainbow
    case 8: {
      newMode.numColors = 8;
      for (int i = 0; i < newMode.numColors; i++) {
        newMode.hue[i] = i * 32;
        newMode.sat[i] = 255;
        newMode.val[i] = random(1, 4) * 85;
      }

      break;
    }

    // Solid
    case 9: {
      newMode.numColors = 1;
      newMode.hue[0] = random(0, 16) * 16;
      newMode.sat[0] = random(0, 4) * 85;
      newMode.val[0] = random(1, 4) * 85;
    }

    break;
  }

  int blank = random(0, 3); // randomly chooses to add blanks to colorset
  if (blank == 0) {
    if (newMode.patternNum == 1 || newMode.patternNum == 2 || newMode.patternNum == 8 || newMode.patternNum == 13 || newMode.patternNum == 16) {
      if (newMode.numColors < 8) { // Blank at beginning
        newMode.hue[newMode.numColors] = newMode.hue[0];
        newMode.sat[newMode.numColors] = newMode.sat[0];
        newMode.val[newMode.numColors] = newMode.val[0];
        newMode.numColors += 1;
        newMode.val[0] = 0;
      }
      else newMode.val[0] = random(1, 3) * 85;
    }
    bool blankType = random (0, 3);
    if (newMode.numColors < 8) {
      if (blankType == 0) { // Blank at end
        newMode.val[newMode.numColors] = 0;
        newMode.numColors += 1;
      }
      if (newMode.numColors >= 1 && newMode.numColors < 7) {
        if (blankType == 1) { // 2 blanks
          int mid = newMode.numColors / 2;
          newMode.hue[newMode.numColors] = newMode.hue[mid];
          newMode.sat[newMode.numColors] = newMode.sat[mid];
          newMode.val[newMode.numColors] = newMode.val[mid];
          newMode.val[mid] = 0;
          newMode.val[newMode.numColors + 1] = 0;
          newMode.numColors += 2;
        }
      }
    }
  }

  return newMode;
}

//Default Modes
//---------------------------------------------------------

void setDefaults() {
  for (int tempMode = 0; tempMode < totalModes; tempMode++) {
    mode[tempMode].patternNum = 0;
    mode[tempMode].numColors = 1;
    int tempColor = random(0, 255);
    mode[tempMode].hue[0] = tempColor;
    mode[tempMode].sat[0] = 255;
    mode[tempMode].val[0] = 255;
  }

  mode[0].patternNum = 8;
  mode[0].numColors = 8;
  for (int c = 0; c < mode[0].numColors; c++) {
    mode[0].hue[c] = c * 32;
    mode[0].sat[c] = 255;
    mode[0].val[c] = 170;
  }

  mode[1].patternNum = 6;
  mode[1].numColors = 3;
  mode[1].hue[0] = 0;
  mode[1].sat[0] = 255;
  mode[1].val[0] = 170;
  mode[1].hue[1] = 160;
  mode[1].sat[1] = 255;
  mode[1].val[1] = 170;
  mode[1].hue[2] = 224;
  mode[1].sat[2] = 255;
  mode[1].val[2] = 120;

  mode[2].patternNum = 2;
  mode[2].numColors = 5;
  mode[2].hue[0] = 0;
  mode[2].sat[0] = 0;
  mode[2].val[0] = 0;
  mode[2].hue[1] = 0;
  mode[2].sat[1] = 0;
  mode[2].val[1] = 120;
  mode[2].hue[2] = 64;
  mode[2].sat[2] = 170;
  mode[2].val[2] = 120;
  mode[2].hue[2] = 64;
  mode[2].sat[2] = 255;
  mode[2].val[2] = 120;
  mode[2].hue[3] = 160;
  mode[2].sat[3] = 255;
  mode[2].val[3] = 120;

  mode[3].patternNum = 3;
  mode[3].numColors = 2;
  mode[3].hue[0] = 224;
  mode[3].sat[0] = 255;
  mode[3].val[0] = 170;
  mode[3].hue[1] = 192;
  mode[3].sat[1] = 255;
  mode[3].val[1] = 170;

  mode[4].patternNum = 9;
  mode[4].numColors = 3;
  mode[4].hue[0] = 0;
  mode[4].sat[0] = 255;
  mode[4].val[0] = 170;
  mode[4].hue[1] = 96;
  mode[4].sat[1] = 255;
  mode[4].val[1] = 170;
  mode[4].hue[2] = 160;
  mode[4].sat[2] = 255;
  mode[4].val[2] = 170;

  mode[5].patternNum = 5;
  mode[5].numColors = 4;
  mode[5].hue[0] = 0;
  mode[5].sat[0] = 255;
  mode[5].val[0] = 120;
  mode[5].hue[1] = 160;
  mode[5].sat[1] = 255;
  mode[5].val[1] = 170;
  mode[5].hue[2] = 64;
  mode[5].sat[2] = 255;
  mode[5].val[2] = 170;
  mode[5].hue[3] = 96;
  mode[5].sat[3] = 255;
  mode[5].val[3] = 170;

  mode[6].patternNum = 7;
  mode[6].numColors = 3;
  mode[6].hue[0] = 0;
  mode[6].sat[0] = 255;
  mode[6].val[0] = 120;
  mode[6].hue[1] = 192;
  mode[6].sat[1] = 255;
  mode[6].val[1] = 170;
  mode[6].hue[2] = 128;
  mode[6].sat[2] = 255;
  mode[6].val[2] = 170;

  mode[7].patternNum = 1;
  mode[7].numColors = 3;
  mode[7].hue[0] = 16;
  mode[7].sat[0] = 255;
  mode[7].val[0] = 170;
  mode[7].hue[1] = 96;
  mode[7].sat[1] = 255;
  mode[7].val[1] = 255;
  mode[7].hue[2] = 192;
  mode[7].sat[2] = 255;
  mode[7].val[2] = 85;

  mode[8].patternNum = 17;
  mode[8].numColors = 3;
  mode[8].hue[0] = 16;
  mode[8].sat[0] = 255;
  mode[8].val[0] = 255;
  mode[8].hue[1] = 128;
  mode[8].sat[1] = 255;
  mode[8].val[1] = 85;
  mode[8].hue[2] = 160;
  mode[8].sat[2] = 255;
  mode[8].val[2] = 255;

  mode[9].patternNum = 20;
  mode[9].numColors = 1;
  mode[9].hue[0] = 80;
  mode[9].sat[0] = 255;
  mode[9].val[0] = 85;

  mode[10].patternNum = 4;
  mode[10].numColors = 8;
  mode[10].hue[0] = 0;
  mode[10].sat[0] = 255;
  mode[10].val[0] = 85;
  mode[10].hue[1] = 32;
  mode[10].sat[1] = 255;
  mode[10].val[1] = 170;
  mode[10].hue[2] = 64;
  mode[10].sat[2] = 255;
  mode[10].val[2] = 255;
  mode[10].hue[3] = 96;
  mode[10].sat[3] = 255;
  mode[10].val[3] = 85;
  mode[10].hue[4] = 128;
  mode[10].sat[4] = 255;
  mode[10].val[4] = 85;
  mode[10].hue[5] = 160;
  mode[10].sat[5] = 255;
  mode[10].val[5] = 85;
  mode[10].hue[6] = 192;
  mode[10].sat[6] = 255;
  mode[10].val[6] = 170;
  mode[10].hue[7] = 224;
  mode[10].sat[7] = 255;
  mode[10].val[7] = 85;

  mode[11].patternNum = 19;
  mode[11].numColors = 3;
  mode[11].hue[0] = 144;
  mode[11].sat[0] = 0;
  mode[11].val[0] = 0;
  mode[11].hue[1] = 144;
  mode[11].sat[1] = 0;
  mode[11].val[1] = 255;
  mode[11].hue[2] = 96;
  mode[11].sat[2] = 255;
  mode[11].val[2] = 0;

  mode[12].patternNum = 2;
  mode[12].numColors = 4;
  mode[12].hue[0] = 192;
  mode[12].sat[0] = 255;
  mode[12].val[0] = 85;
  mode[12].hue[1] = 240;
  mode[12].sat[1] = 255;
  mode[12].val[1] = 255;
  mode[12].hue[2] = 64;
  mode[12].sat[2] = 255;
  mode[12].val[2] = 85;
  mode[12].hue[3] = 144;
  mode[12].sat[3] = 255;
  mode[12].sat[3] = 170;

  mode[13].patternNum = 8;
  mode[13].numColors = 5;
  mode[13].hue[0] = 16;
  mode[13].sat[0] = 255;
  mode[13].val[0] = 0;
  mode[13].hue[1] = 144;
  mode[13].sat[1] = 255;
  mode[13].val[1] = 0;
  mode[13].hue[2] = 16;
  mode[13].sat[2] = 255;
  mode[13].val[2] = 85;
  mode[13].hue[3] = 144;
  mode[13].sat[3] = 255;
  mode[13].val[3] = 255;
}
