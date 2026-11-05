/*
  Line follower robot - MA7
  Luca Bailo

  The Arduino reads two FC-51 IR sensors and drives two DC motors through
  two FQP30N06L logic-level N-channel MOSFETs (low-side switches).
  Arduino pin -> 1 kOhm resistor -> gate. Source -> GND. Drain -> motor -> +V.
  A 1N4001 flyback diode is in parallel with each motor (stripe towards +V).

  FC-51 output: LOW  = reflection detected (white surface)
                HIGH = no reflection     (black surface)

  Steering: when a sensor touches the line the robot first makes a SOFT
  correction (inner wheel slowed down). If the sensor stays on the line
  longer than HARD_AFTER_MS, it is a real curve and the correction becomes
  HARD (inner wheel stopped). Small drifts on the straight are therefore
  corrected gently, while sharp bends still get a strong turn.
*/

// ---------------- Pins ----------------
const int SENSOR_LEFT  = 2;   // OUT of the left FC-51
const int SENSOR_RIGHT = 3;   // OUT of the right FC-51
const int MOTOR_LEFT   = 5;   // PWM pin -> resistor -> gate of left MOSFET
const int MOTOR_RIGHT  = 6;   // PWM pin -> resistor -> gate of right MOSFET

// ---------------- Settings ----------------
// true  = white line on a black floor (sensors on both sides of the line)
// false = black line on a white floor (sensors on both sides of the line)
const bool WHITE_LINE = true;

// Speeds (0-255, PWM), before the trim is applied
const int SPEED_FORWARD    = 100;  // both motors, going straight
const int SPEED_TURN       = 120;  // outer motor while correcting/turning
const int SPEED_INNER_SOFT = 40;   // inner motor, small correction (straight)
const int SPEED_INNER_HARD = 0;    // inner motor, real curve

// If a sensor stays on the line longer than this (ms), switch to HARD turn
const unsigned long HARD_AFTER_MS = 150;

// Motor balance: the two motors are never identical.
// If one wheel is faster, lower its value (100 = full speed, 90 = 90 %...).
const int LEFT_TRIM  = 80;    // percent (left motor is faster)
const int RIGHT_TRIM = 100;   // percent

// Soft start: max PWM increase per loop. Avoids current peaks that make the
// battery voltage drop and reset the Arduino.
const int RAMP_STEP = 15;

// Serial debug. Keep it false when the robot runs on the track:
// printing slows down the loop and makes the robot react late.
const bool DEBUG = false;

// ---------------- State ----------------
int currentLeft  = 0;
int currentRight = 0;
int turning = 0;                 // -1 = turning left, +1 = turning right, 0 = straight
unsigned long turnStart = 0;     // when the current turn started

// Returns true if the sensor is on the line
bool onLine(int pin) {
  int value = digitalRead(pin);
  bool seesWhite = (value == LOW);         // FC-51 is active-low
  return WHITE_LINE ? seesWhite : !seesWhite;
}

// Moves 'current' towards 'target' by at most RAMP_STEP when speeding up
int ramp(int current, int target) {
  if (target > current) return min(current + RAMP_STEP, target);
  return target;   // slowing down / stopping is immediate
}

void drive(int leftTarget, int rightTarget) {
  currentLeft  = ramp(currentLeft,  leftTarget  * LEFT_TRIM  / 100);
  currentRight = ramp(currentRight, rightTarget * RIGHT_TRIM / 100);
  analogWrite(MOTOR_LEFT,  currentLeft);
  analogWrite(MOTOR_RIGHT, currentRight);
}

void setup() {
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);
  pinMode(MOTOR_LEFT, OUTPUT);
  pinMode(MOTOR_RIGHT, OUTPUT);
  analogWrite(MOTOR_LEFT, 0);
  analogWrite(MOTOR_RIGHT, 0);

  if (DEBUG) {
    Serial.begin(115200);              // fast baud rate: less delay per print
    Serial.println("=== START ===");   // if this appears often, the Arduino is resetting
  }
  delay(500);
}

void loop() {
  bool left  = onLine(SENSOR_LEFT);
  bool right = onLine(SENSOR_RIGHT);

  // Decide the direction
  int newTurning;
  if (left && !right)      newTurning = -1;  // line on the left  -> turn left
  else if (!left && right) newTurning = +1;  // line on the right -> turn right
  else                     newTurning = 0;   // line in the middle or crossing -> straight

  if (newTurning != turning) {
    turning = newTurning;
    turnStart = millis();                    // a new turn starts now
  }

  // Soft correction first, hard turn if the sensor stays on the line
  int inner = (millis() - turnStart > HARD_AFTER_MS) ? SPEED_INNER_HARD
                                                     : SPEED_INNER_SOFT;

  if (turning == -1)      drive(inner, SPEED_TURN);
  else if (turning == +1) drive(SPEED_TURN, inner);
  else                    drive(SPEED_FORWARD, SPEED_FORWARD);

  if (DEBUG) {
    Serial.print(left ? "L:LINE " : "L:---- ");
    Serial.print(right ? "R:LINE " : "R:---- ");
    Serial.print(currentLeft);
    Serial.print("/");
    Serial.println(currentRight);
  }

  delay(2);
}
