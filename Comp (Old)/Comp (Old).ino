//Include the neopixel library
#include <Adafruit_NeoPixel.h>

//Used throughout the program to reference the number of LEDS on the strand
//This is completely modular -- any number works
#define lNUMLEDS 80
#define rNUMLEDS 80

//This is the wheel diameter of the robot's main drive wheels
#define WHEEL_DIAMETER 6

//Distance between two leds. Please keep this constant. Pretty please?
#define LED_PITCH 0.6875

const int lLedPin = 13; //Left side leds
const int rLedPin = 12; //Right side leds
const int statePin = 0; //Used to get state from the RoboRIO (analog)
const int analogPin1 = 1; //Used to get data from the RoboRIO (analog)
const int analogPin2 = 2; //Used to get data from the RoboRIO (analog)

//Drop the brightness if the robot starts to lose voltage
bool lowVoltage = false;
const double lowVoltageBrightnessDrop = 5.0;
int lastState;

long startTime;
long currentTime;
int timeSinceLastAction; //Used to define tick rate

const double idleStatePeriodLength = 2.0; //Period as in waves -- the time it takes one cycle to pass in seconds
const int idleStateColor[] = {0, 42, 255}; //Close to Columbia Blue
const int idleStateTick = 10; //In milliseconds
const double idleStateMaxBrightness = 0.25; //Arbitrary
long periodStart;

const int driveStateColor[] = {0, 42, 255};
const int driveModeSpacing = 4;
int lIndex;
int rIndex;

bool firstTime = true;

//2D arrays storing information about what the LED state is
//Instead of messing with commands in library, just update this array
//and call updateLEDs() to display it on the strand
//Index 1 is the LED, Index 2 is 8-bit [R, G, B]
int lLED[lNUMLEDS][3] = {};
int rLED[rNUMLEDS][3] = {};

enum states {
  IDLE_STATE,         //Do this before and after the match (detect when voltage is less than some constant)
  DRIVE_STATE,        //Do this while the robot is driving around
  SHOOTER_ON_STATE,   //Do this once the shooter gets turned on
  SHOOTER_OFF_STATE   //Do this when the shooter gets turned off
};

//Initialize the two Adafruit_NeoPixel objects, one for each side
Adafruit_NeoPixel lStrip = Adafruit_NeoPixel(lNUMLEDS, lLedPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel rStrip = Adafruit_NeoPixel(rNUMLEDS, rLedPin, NEO_GRB + NEO_KHZ800);

void updateLEDs() { //This is a general purpose function to make everything easier. Instead of
                    //having to deal with the library's functions, I can do everything throughout
                    //the arrays I defined above, and then just update when necessary.
  int i, j;
  if(lowVoltage) { //If the robot battery has low voltage, drop the brightness to save power
    for(i=0; i<lNUMLEDS; ++i) { //For each of the leds on the left side
      for(j=0; j<3; ++j) {
        lLED[i][j] = static_cast<int>(static_cast<double>(lLED[i][j])/lowVoltageBrightnessDrop); //The static_casts probably aren't necessary, but I want to make sure the types are correct
      }
      lStrip.setPixelColor(i, lLED[i][0], lLED[i][1], lLED[i][2]); //Update the left strand pixels
    }
    for(i=0; i<rNUMLEDS; ++i) { //For each of the leds on the right side
      for(j=0; j<3; ++j) {
        rLED[i][j] = static_cast<int>(static_cast<double>(rLED[i][j])/lowVoltageBrightnessDrop); //The static_casts probably aren't necessary, but I want to make sure the types are correct
      }
      rStrip.setPixelColor(i, rLED[i][0], rLED[i][1], rLED[i][2]); //Update the right strand pixels
    }
  }
  else { //If voltage isn't low, FULL BRIGHTNESS :O
    for(i=0; i<lNUMLEDS; ++i) {
      lStrip.setPixelColor(i, lLED[i][0], lLED[i][1], lLED[i][2]);
    }
    for(i=0; i<rNUMLEDS; ++i) {
      rStrip.setPixelColor(i, rLED[i][0], rLED[i][1], rLED[i][2]);
    }
  }
  lStrip.show();
  rStrip.show();
}

void clearStrips() { //Basically turn off the strips by setting all the leds to RGB [0, 0, 0]
  int i, j;
  for(j=0; j<3; ++j) { //For the R value, then the G value, then the B value
    for(i=0; i<lNUMLEDS; ++i) { //For all leds on the left
      lLED[i][j] = 0;
    }
    for(i=0; i<rNUMLEDS; ++i) { //For all leds on the right
      rLED[i][j] = 0;
    }
  } //Ordering it like this should make it slightly faster, which doesn't hurt anything
  updateLEDs();
}

void idleState(int timeSinceLastAction) {
  if(timeSinceLastAction<idleStateTick) { //Only update this every so often
    return;
  }
  int i, j;
  double brightnessConstant = ((static_cast<double>(currentTime-periodStart)/1000.0)-1>0) ? (static_cast<double>(currentTime-periodStart)/1000.0)-1 : 1-(static_cast<double>(currentTime-periodStart)/1000.0);
  if(brightnessConstant > 1) {
    brightnessConstant = 1;
  }
  else if(brightnessConstant < 0) {
    brightnessConstant = 0;
  }
  for(j=0; j<3; ++j) {
    for(i=0; i<lNUMLEDS; ++i) {
      lLED[i][j] = static_cast<int>(static_cast<double>(idleStateColor[j]*brightnessConstant*idleStateMaxBrightness));
    }
    for(i=0; i<rNUMLEDS; ++i) {
      rLED[i][j] = static_cast<int>(static_cast<double>(idleStateColor[j]*brightnessConstant*idleStateMaxBrightness));
    }
  }
  if(currentTime-periodStart >= idleStatePeriodLength*1000) {
    periodStart += idleStatePeriodLength*1000;
  }
}
void driveState(int timeSinceLastAction, double rawAnalog1, double rawAnalog2) { //Analog1 is left, analog2 is right
  int i, j;
  if(firstTime) {
    lIndex = 0;
    rIndex = 0;
    for(j=0; j<3; ++j) {
      for(i=0; i<lNUMLEDS; ++i) {
        if(i % driveModeSpacing == 0) {
          lLED[i][j] = driveStateColor[j];
        }
        else {
          lLED[i][j] = 0;
        }
      }
      for(i=0; i<rNUMLEDS; ++i) {
        if(i % driveModeSpacing == 0) {
          rLED[i][j] = driveStateColor[j];
        }
        else {
          rLED[i][j] = 0;
        }
      }
    }
  }

  double lRPM = 60; //Include a function to get from rawAnalog1 soontm
  double rRPM = 60; //These are in RPM
  double lIPS = 2 * 3.14159265358979 * WHEEL_DIAMETER * lRPM; //These are in inches per second
  double rIPS = 2 * 3.14159265358979 * WHEEL_DIAMETER * rRPM; //.6975 in between each led
  double lLPS = lIPS/LED_PITCH; //These are in leds per second
  double rLPS = rIPS/LED_PITCH;
  double lShiftFactor = lLPS * timeSinceLastAction; //These are in leds
  double rShiftFactor = lLPS * timeSinceLastAction;
  if((lShiftFactor > 1 || lShiftFactor < -1) && (rShiftFactor > 1 || rShiftFactor < -1)) {
    periodStart = currentTime;
    lIndex += static_cast<int>(lShiftFactor);
    rIndex += static_cast<int>(rShiftFactor);
    for(j=0; j<3; ++j) {
      for(i=0; i<lNUMLEDS; ++i) {
        if((i % driveModeSpacing)+lIndex == 0) {
          lLED[i][j] = driveStateColor[j];
        }
        else {
          lLED[i][j] = 0;
        }
      }
      for(i=0; i<rNUMLEDS; ++i) {
        if((i % driveModeSpacing)+rIndex == 0) {
          rLED[i][j] = driveStateColor[j];
        }
        else {
          rLED[i][j] = 0;
        }
      }
    }
  }
}
void shooterOnState(int timeSinceLastAction, double rawAnalog1, double rawAnalog2) {}
void shooterOffState(int timeSinceLastAction, double rawAnalog1, double rawAnalog2) {}


void setup() {
  //Start the strips, and then clear them
  lStrip.begin();
  rStrip.begin();
  clearStrips();
  startTime = millis(); //Get the start time (used later)
  currentTime = startTime;
  periodStart = startTime;
}
void loop() {
  int state, stateRead, analog1, analog2;

  currentTime = millis();

  stateRead = analogRead(statePin);
  if(200<stateRead && stateRead<=400) { //Shooter On State
    state = SHOOTER_ON_STATE;
  }
  else if(400<stateRead && stateRead<=600) { //Shooter Off State
    state = SHOOTER_OFF_STATE;
  }
  else if(600<stateRead && stateRead<=800) { //Drive State
    state = DRIVE_STATE;
    timeSinceLastAction = static_cast<int>(currentTime-periodStart);
  }
  else if(800<stateRead && stateRead<=1023) { //Flag Low Voltage
    state = lastState; //The low voltage alert doesn't explain what state it should be, so revert back to the previous state
    lowVoltage = true;
  }
  else { //Idle State
         //This will run if the input is 0 (i.e. the RoboRIO isn't sending any signal, like before and after a match)
    state = IDLE_STATE;
    timeSinceLastAction = static_cast<int>(currentTime - periodStart);
  }
  firstTime = false;
  if(state != lastState)
  {
    periodStart = currentTime;
    firstTime = true;
    clearStrips();
  }
  lastState = state; //Update last state (in case there's a low voltage message)

  analog1 = analogRead(analogPin1); //Get analog values from RoboRIO
  analog2 = analogRead(analogPin2);

  switch(state) { //Run the appropriate state display function
    case IDLE_STATE:
      idleState(timeSinceLastAction);
      break;
    case DRIVE_STATE:
      driveState(timeSinceLastAction, analog1, analog2);
      break;
    case SHOOTER_ON_STATE:
      shooterOnState(timeSinceLastAction, analog1, analog2);
      break;
    case SHOOTER_OFF_STATE:
      shooterOffState(timeSinceLastAction, analog1, analog2);
      break;
    default:
      idleState(timeSinceLastAction);
      break;
  }

  updateLEDs();
}
