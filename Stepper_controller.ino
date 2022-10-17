#include "WingStepper.h"

const int DIR_PIN    = 3;
const int STEP_PIN   = 4; // Step pin for turning movement
const int SENSOR_PIN = 2;
const int ROTATE_PIN = 23;

const int PWR_LED = 21;
const int ROT_LED = 22;

const int SLEEP_PIN = 5;

unsigned long timer = 0;
int blinkDelay = 500;
unsigned long nextTimeout = 0;
bool previousOn = false;

WingStepper stepper(STEP_PIN, DIR_PIN, SENSOR_PIN);

bool cal_status = false; // Calibration status
bool hasRotated = false;
bool rotate_right_mem;

float target_angle = 0; //Wing positions

//Switch case modes
const int CALIBRATE_STATE = 0;
const int STANDBY_STATE = 1;
const int TURNING_RIGHT_STATE = 2;
const int TURNING_LEFT_STATE = 3;
const int TURNING_DEFAULT_STATE = 4;
int state = CALIBRATE_STATE;

// LIMITS
const float right_wing_angle = 75.0;
const float left_wing_angle = -right_wing_angle;
const float default_wing_angle = 0.0;

// SKRIVE CURRENT ANGLE TIL SERIAL
// GÅ I ERRORMODE ETTER X ANTALL SEKUNDER MED TURNING
// KALIBRERE HVER ROTASJON
// SAMMENLIGNE POSISJON TIL ROTASJON MED FORRIGE ROTASJON

void setup() {
  stepper.begin();
  stepper.flip_ref_direction();
  pinMode (ROTATE_PIN, INPUT_PULLUP);
  pinMode (SENSOR_PIN, INPUT);
  pinMode (DIR_PIN, OUTPUT);
  pinMode (STEP_PIN, OUTPUT);
  pinMode(PWR_LED, OUTPUT);
  pinMode(ROT_LED, OUTPUT);
  pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite (SLEEP_PIN, HIGH);
  digitalWrite (PWR_LED, HIGH);
}

void loop() {
  //printSensor(); //debug for the sensor
  
  switch (state) {
    case CALIBRATE_STATE: {
        blinking(ROT_LED);
        // pratically, it places the sensor in the middle of the two magnets
        bool cal_status = false;
        cal_status = stepper.calibrate(); // calibrate returns true when finished
        //change state when finished
        if (cal_status) {
          Serial.println("Finished calibrating!");
          state = STANDBY_STATE;
          digitalWrite(ROT_LED, LOW);
        }
      }
      break;

    case STANDBY_STATE: {
        stepper.rotate(default_wing_angle);
        bool rotateBtn = digitalRead (ROTATE_PIN);
        if (rotateBtn == HIGH) {
          state = TURNING_RIGHT_STATE; // SET TO LAST TURNING DIRECTION
          digitalWrite(ROT_LED, HIGH);
        }
      }
      break;

    // Right and left seen from top of axis side of motor
    case TURNING_RIGHT_STATE: {
        hasRotated = stepper.rotate(right_wing_angle);
        if (hasRotated) {
          state = TURNING_LEFT_STATE;
        }
      }
      break;

    case TURNING_LEFT_STATE: {
        hasRotated = stepper.rotate(left_wing_angle);
        if (hasRotated) {
          state = TURNING_DEFAULT_STATE;
        }
      }
      break;

    case TURNING_DEFAULT_STATE: {
        hasRotated = stepper.rotate(default_wing_angle);
        if (hasRotated) {
          state = STANDBY_STATE;
          digitalWrite(ROT_LED, LOW);
        }
      }
      break;
  }
}

/*
void printSensor() {
  if (millis() > timer) {
    Serial.println(digitalRead(SENSOR_PIN));
    timer = millis() + blinkDelay;
  }
}
*/

void startTimer (unsigned long timeout)
{
  nextTimeout = millis() + timeout;
}


bool timerHasExpired ()
{
  bool expired = false;
  if (millis() >= nextTimeout)
  {
    expired = true;
  }
  return expired;
}


void blinking(int led) {
  if (timerHasExpired() && !previousOn) {
    startTimer(blinkDelay);
    digitalWrite(led, HIGH);
    previousOn = true;
  }
  else if (timerHasExpired() && previousOn) {
    startTimer(blinkDelay);
    digitalWrite(led, LOW);
    previousOn = false;
  }
}
