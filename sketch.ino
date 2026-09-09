/*
  Exoskeleton Joint Sensor & Actuator Control
  ESP32 + Potentiometer + TB6612FNG

  Joint:
  Knee flexion / extension

  Angle:
  0°   = full extension
  120° = deep flexion

  Safety zones:
  < 5°       FAULT LOW
  5–15°      LIMIT LOW
  15–105°    SAFE
  105–115°   LIMIT HIGH
  > 115°     FAULT HIGH

  Serial command:
  r = reset latched fault
*/

// ============================================================
// TB6612FNG PINS
// ============================================================

const int AI1pin = 22;
const int AI2pin = 23;
const int APWMpin = 21;
const int StandbyPin = 16;

// Original encoder pins
const int Apin = 18;
const int Bpin = 19;

// ============================================================
// JOINT SENSOR
// ============================================================

const int POT_PIN = 34;

// ============================================================
// ANGLE LIMITS
// ============================================================

const float FAULT_LOW = 5.0;
const float LIMIT_LOW = 15.0;

const float SAFE_HIGH = 105.0;
const float LIMIT_HIGH = 115.0;

// ============================================================
// VARIABLES
// ============================================================

long count = 0;

int potRaw = 0;
float jointAngle = 0.0;

String zone = "SAFE";

bool faultLatched = false;

// Current motor command for reporting
String motorDirection = "STOP";
int motorPWM = 0;

// ============================================================
// MOTOR CONTROL
// ============================================================

void motorStop() {

  analogWrite(APWMpin, 0);

  digitalWrite(AI1pin, LOW);
  digitalWrite(AI2pin, LOW);

  digitalWrite(StandbyPin, HIGH);

  motorDirection = "STOP";
  motorPWM = 0;
}

void motorBrake() {

  analogWrite(APWMpin, 255);

  digitalWrite(AI1pin, HIGH);
  digitalWrite(AI2pin, HIGH);

  digitalWrite(StandbyPin, HIGH);

  motorDirection = "BRAKE";
  motorPWM = 255;
}

void motorForward(int speed) {

  speed = constrain(speed, 0, 255);

  digitalWrite(StandbyPin, HIGH);

  digitalWrite(AI1pin, LOW);
  digitalWrite(AI2pin, HIGH);

  analogWrite(APWMpin, speed);

  motorDirection = "FORWARD";
  motorPWM = speed;
}

void motorBackward(int speed) {

  speed = constrain(speed, 0, 255);

  digitalWrite(StandbyPin, HIGH);

  digitalWrite(AI1pin, HIGH);
  digitalWrite(AI2pin, LOW);

  analogWrite(APWMpin, speed);

  motorDirection = "REVERSE";
  motorPWM = speed;
}

// ============================================================
// ZONE DETECTION
// ============================================================

void determineZone() {

  // Fault remains latched until reset
  if (faultLatched) {
    zone = "FAULT_LATCHED";
    return;
  }

  // Low hard limit
  if (jointAngle < FAULT_LOW) {

    zone = "FAULT_LOW";
    faultLatched = true;

    Serial.println("!!! LOW LIMIT FAULT LATCHED !!!");

    return;
  }

  // Low soft limit
  if (jointAngle < LIMIT_LOW) {

    zone = "LIMIT_LOW";
    return;
  }

  // Safe region
  if (jointAngle <= SAFE_HIGH) {

    zone = "SAFE";
    return;
  }

  // High soft limit
  if (jointAngle <= LIMIT_HIGH) {

    zone = "LIMIT_HIGH";
    return;
  }

  // High hard limit
  zone = "FAULT_HIGH";
  faultLatched = true;

  Serial.println("!!! HIGH LIMIT FAULT LATCHED !!!");
}

// ============================================================
// MOTOR CONTROL FROM ANGLE
// ============================================================

void controlMotor() {

  // ----------------------------------------------------------
  // FAULT
  // ----------------------------------------------------------

  if (faultLatched) {

    motorBrake();

    return;
  }

  // ----------------------------------------------------------
  // SAFE
  // ----------------------------------------------------------

  if (zone == "SAFE") {

    motorStop();

    return;
  }

  // ----------------------------------------------------------
  // LOW LIMIT
  // Drive toward increasing angle
  // ----------------------------------------------------------

  if (zone == "LIMIT_LOW") {

    int speed = map(
      (int)jointAngle,
      (int)FAULT_LOW,
      (int)LIMIT_LOW,
      180,
      80
    );

    speed = constrain(speed, 80, 180);

    motorForward(speed);

    return;
  }

  // ----------------------------------------------------------
  // HIGH LIMIT
  // Drive toward decreasing angle
  // ----------------------------------------------------------

  if (zone == "LIMIT_HIGH") {

    int speed = map(
      (int)jointAngle,
      (int)SAFE_HIGH,
      (int)LIMIT_HIGH,
      80,
      180
    );

    speed = constrain(speed, 80, 180);

    motorBackward(speed);

    return;
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  // TB6612
  pinMode(AI1pin, OUTPUT);
  pinMode(AI2pin, OUTPUT);
  pinMode(APWMpin, OUTPUT);
  pinMode(StandbyPin, OUTPUT);

  // Original encoder inputs
  pinMode(Apin, INPUT_PULLUP);
  pinMode(Bpin, INPUT_PULLUP);

  // Potentiometer
  pinMode(POT_PIN, INPUT);

  // Enable driver
  digitalWrite(StandbyPin, HIGH);

  // Safe startup state
  motorStop();

  Serial.println();
  Serial.println("========================================");
  Serial.println("EXOSKELETON CONTROL SYSTEM");
  Serial.println("ESP32 + POTENTIOMETER + TB6612FNG");
  Serial.println("========================================");
  Serial.println();
  Serial.println("Type 'r' in Serial Monitor to reset a fault.");
  Serial.println();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // MANUAL FAULT RESET
  // ----------------------------------------------------------

  if (Serial.available()) {

    char command = Serial.read();

    if (command == 'r' || command == 'R') {

      faultLatched = false;
      zone = "SAFE";

      motorStop();

      Serial.println();
      Serial.println(">>> FAULT RESET");
      Serial.println(">>> SYSTEM RETURNED TO SAFE STATE");
      Serial.println();
    }
  }

  // ----------------------------------------------------------
  // READ POTENTIOMETER
  // ----------------------------------------------------------

  potRaw = analogRead(POT_PIN);

  // Convert 0–4095 ADC into 0–120°
  jointAngle =
      ((float)potRaw / 4095.0) * 120.0;

  jointAngle = constrain(jointAngle, 0.0, 120.0);

  // ----------------------------------------------------------
  // DETERMINE ZONE
  // ----------------------------------------------------------

  determineZone();

  // ----------------------------------------------------------
  // CONTROL MOTOR
  // ----------------------------------------------------------

  controlMotor();

  // ----------------------------------------------------------
  // STATUS REPORT
  // ----------------------------------------------------------

  static unsigned long lastReport = 0;

  if (millis() - lastReport >= 200) {

    lastReport = millis();

    Serial.print("ADC=");
    Serial.print(potRaw);

    Serial.print("  Angle=");
    Serial.print(jointAngle, 1);

    Serial.print(" deg");

    Serial.print("  Zone=");
    Serial.print(zone);

    Serial.print("  Fault=");
    Serial.print(faultLatched ? "YES" : "NO");

    Serial.print("  Motor=");
    Serial.print(motorDirection);

    Serial.print("  PWM=");
    Serial.print(motorPWM);

    Serial.print("  AIN1=");
    Serial.print(digitalRead(AI1pin));

    Serial.print("  AIN2=");
    Serial.println(digitalRead(AI2pin));
  }

  delay(20);
}