const int relayPinSolenoid1 = 22; // Pin connected to the relay controlling solenoid valve 1
const int relayPinSolenoid2 = 23; // Pin connected to the relay controlling solenoid valve 2
const int relayPinMotor1 = 16;    // Pin connected to the relay controlling motor 1
const int relayPinMotor2 = 17;    // Pin connected to the relay controlling motor 2
const unsigned long onInterval = 5000;  // Interval for turning on (5 seconds)
const unsigned long offInterval = 5000; // Interval for turning off (5 seconds)

unsigned long previousMillis = 0;
bool solenoidState1 = false;
bool solenoidState2 = false;
bool motorState1 = false;
bool motorState2 = false;

void setup() {
  pinMode(relayPinSolenoid1, OUTPUT);
  pinMode(relayPinSolenoid2, OUTPUT);
  pinMode(relayPinMotor1, OUTPUT);
  pinMode(relayPinMotor2, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if it's time to turn on
  if (!solenoidState1 && !solenoidState2 && !motorState1 && !motorState2 && (currentMillis - previousMillis >= offInterval)) {
    previousMillis = currentMillis;
    solenoidState1 = true;
    solenoidState2 = true;
    motorState1 = true;
    motorState2 = true;
  }

  // Check if it's time to turn off
  if ((solenoidState1 || solenoidState2 || motorState1 || motorState2) && (currentMillis - previousMillis >= onInterval)) {
    previousMillis = currentMillis;
    solenoidState1 = false;
    solenoidState2 = false;
    motorState1 = false;
    motorState2 = false;
  }

  // Set relay states
  digitalWrite(relayPinSolenoid1, solenoidState1 ? HIGH : LOW);
  digitalWrite(relayPinSolenoid2, solenoidState2 ? HIGH : LOW);
  digitalWrite(relayPinMotor1, motorState1 ? HIGH : LOW);
  digitalWrite(relayPinMotor2, motorState2 ? HIGH : LOW);
}
