#include <Adafruit_NeoPixel.h>

#define NUMLEDS 16

const int ledPin = 13;
const int statePin = 0;
const int leftPin = 1;
const int rightPin = 2;

bool lowVoltage = false;
const double lowVoltageBrightnessDrop = 5.0;

long startTime;
long currentTime;
long elapsedTime;
bool flash = false;

const int idleStateCycleTime = 2; //seconds for fade in/out

int leds[NUMLEDS][3] = {};

enum states {
  IDLE_STATE,         //Before and after match, pattern TBD
  DRIVE_STATE,        //Driving around, analog line 2, 3 represent left and right tread speed. Tank tread pattern?
  SHOOTER_ON_STATE,   //Shooter just got activated, pattern TBD. Analog line 2 could represent shooter wheel speed?
  SHOOTER_OFF_STATE   //Shooter just got deactivated, pattern TBD. Analog line 2 could represent shooter wheel speed?
};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMLEDS, ledPin, NEO_GRB + NEO_KHZ800);

void updateLEDs()
{
  int i, j;

  if(lowVoltage)
  {
    for(i=0; i<NUMLEDS; ++i)
    {
      for(j=0; j<3; ++j)
      {
        leds[i][j] = static_cast<int>(static_cast<double>(leds[i][j])/lowVoltageBrightnessDrop);
      }
    }
  }

  for (i=0; i<NUMLEDS; ++i)
  {
    strip.setPixelColor(i, leds[i][0], leds[i][1], leds[i][2]);
  }
  strip.show();
}

void stripClear()
{
  int i, j;
  for(int i=0; i<NUMLEDS; ++i)
  {
    for(j=0; j<3; ++j)
    {
      leds[i][j] = 0;
    }
  }
  updateLEDs();
}

void explode()
{
  int i, j;
  if(elapsedTime >= 70)
  {
    flash = !flash;
    startTime = currentTime;
  }
  if (flash)
  {
    for(i=0; i<NUMLEDS; ++i)
    {
      if(i < NUMLEDS*3.0/4.0)
      {
        leds[i%NUMLEDS][0] = 255;
        leds[i%NUMLEDS][1] = 0;
        leds[i%NUMLEDS][2] = 0;
      }
      else if (i <= NUMLEDS * 3.0/4.0 < NUMLEDS)
      {
        leds[i%NUMLEDS][0] = 80;
        leds[i%NUMLEDS][1] = 80;
        leds[i%NUMLEDS][2] = 80;
      }
    }
    updateLEDs();
  }
  else if (!flash)
  {
    stripClear();
    for (int i = 0; i < NUMLEDS; i++)
    {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
    strip.show();
  }
}

void idleState()
{
  int i;
  for(i=0; i<NUMLEDS; ++i)
  {
    leds[i][0] = 0;
    leds[i][1] = 0;
    leds[i][2] = 255 //Later make this an actual function to fade and fade out.
  }
}
void driveState(double analog1, double analog2)
{
  //
}
void shooterOnState(double analog1)
{
  //
}
void shooterOffState(double analog1)
{
  //
}

void setup()
{
  int i, j;
  for(i=0; i<NUMLEDS; ++i)
  {
    for(j=0; j<3; ++j)
    {
      leds[i][j] = 0;
    }
  }
  strip.begin();
  strip.show();
  updateLEDs();

  pinMode(statePin, INPUT);
  pinMode(leftPin, INPUT);
  pinMode(rightPin, INPUT);

}

void loop()
{
  int state;
  int stateRead;

  currentTime = millis();
  elapsedTime = currentTime - startTime;

  stateRead = analogRead(statePin);

  //Data read from RoboRIO goes here
  if(0 <= stateRead < 255)
  {
    state = SHOOTER_ON_STATE;
  }
  else if (255 <= stateRead < 510)
  {
    state = SHOOTER_OFF_STATE;
  }
  else if (510 <= stateRead < 765)
  {
    state = DRIVE_STATE;
  }
  else if (765 <= stateRead <= 1023)
  {
    state = IDLE_STATE;
  }

  //state = _______ (for testing and demonstrations)

  //State machine from here on out...
  if(state == DRIVE_STATE)
  {
    //analogRead from leftPin and rightPin for wheel speeds
  }
  else if(state == SHOOTER_ON_STATE)
  {
    //analogRead from leftPin for shooter speeds
  }
  else if(state == SHOOTER_OFF_STATE)
  {
    //analogRead from leftPin for shooter speeds
  }
  else if(state == IDLE_STATE)
  {
    //do preset pattern
  }
}
