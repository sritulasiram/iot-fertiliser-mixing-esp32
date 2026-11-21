// Include necessary libraries
#include <Arduino.h>

// Define pin numbers for pumps, solenoid valves, and flow sensors
#define PUMP1_PIN 16
#define PUMP2_PIN 17
#define SOLENOID1_PIN 22
#define SOLENOID2_PIN 23
#define FLOW_SENSOR1_PIN 18
#define FLOW_SENSOR2_PIN 19

// Variables to keep track of dispensed volume for each side
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

// Calibration factors for flow sensors
float calibrationFactor1 = 5.0;  // Update this value based on your calibration
float calibrationFactor2 = 7.0;  // Update this value based on your calibration

// Function prototypes
void dispenseVolume(unsigned long desiredVolume);
void calibrateFlowSensors();

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  
  // Configure pins for pumps, solenoid valves, and flow sensors
  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(SOLENOID1_PIN, OUTPUT);
  pinMode(SOLENOID2_PIN, OUTPUT);
  pinMode(FLOW_SENSOR1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR2_PIN, INPUT_PULLUP);
  
  // Attach interrupt routines to flow sensors
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR1_PIN), []() { pulseCount1++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR2_PIN), []() { pulseCount2++; }, RISING);
}

void loop() {
  // Check if there is data available in the Serial Monitor
  if (Serial.available() > 0) {
    // Read the input command from the Serial Monitor
    String command = Serial.readStringUntil('\n');
    
    if (command.equalsIgnoreCase("calibrate")) {
      // Calibrate flow sensors
      calibrateFlowSensors();
    } else {
      // Parse the desired volume from the input
      unsigned long desiredVolume = command.toInt();
      Serial.print("Dispensing ");
      Serial.print(desiredVolume);
      Serial.println(" ml from each side.");
      
      // Dispense the desired volume
      dispenseVolume(desiredVolume);
    }
  }
}

void calibrateFlowSensors() {
  // Prompt user to input a known volume for calibration
  Serial.println("Calibration: Please enter the known volume (ml) and press Enter:");
  
  // Wait for user input
  while (Serial.available() == 0) { }
  unsigned long knownVolume = Serial.parseInt();
  Serial.print("Calibrating with ");
  Serial.print(knownVolume);
  Serial.println(" ml of water.");
  
  // Reset pulse counts
  pulseCount1 = 0;
  pulseCount2 = 0;
  
  // Open solenoid valves and turn on pumps
  digitalWrite(SOLENOID1_PIN, HIGH);
  digitalWrite(SOLENOID2_PIN, HIGH);
  digitalWrite(PUMP1_PIN, HIGH);
  digitalWrite(PUMP2_PIN, HIGH);
  
  // Delay to allow water to flow
  delay(5000); // Adjust delay time as needed
  
  // Turn off pumps and close solenoid valves
  digitalWrite(PUMP1_PIN, LOW);
  digitalWrite(PUMP2_PIN, LOW);
  digitalWrite(SOLENOID1_PIN, LOW);
  digitalWrite(SOLENOID2_PIN, LOW);
  
  // Calculate calibration factors
  calibrationFactor1 = float(knownVolume) / pulseCount1;
  calibrationFactor2 = float(knownVolume) / pulseCount2;
  
  Serial.print("Calibration complete. Calibration factor 1: ");
  Serial.println(calibrationFactor1);
  Serial.print("Calibration factor 2: ");
  Serial.println(calibrationFactor2);
}

void dispenseVolume(unsigned long desiredVolume) {
  // Reset pulse counts
  pulseCount1 = 0;
  pulseCount2 = 0;
  
  // Open solenoid valves
  digitalWrite(SOLENOID1_PIN, HIGH);
  digitalWrite(SOLENOID2_PIN, HIGH);
  
  // Turn on pumps
  digitalWrite(PUMP1_PIN, HIGH);
  digitalWrite(PUMP2_PIN, HIGH);
  
  // Delay to allow pumps to reach desired flow rates
  delay(1000); // Adjust delay time as needed
  
  // Dispense from pumps until desired volume is reached
  while ((pulseCount1 * calibrationFactor1 < desiredVolume) || (pulseCount2 * calibrationFactor2 < desiredVolume)) {
    // Wait for desired volume to be dispensed
  }
  
  // Turn off pumps
  digitalWrite(PUMP1_PIN, LOW);
  digitalWrite(PUMP2_PIN, LOW);
  
  // Close solenoid valves
  digitalWrite(SOLENOID1_PIN, LOW);
  digitalWrite(SOLENOID2_PIN, LOW);
  
  // Calculate and display flow rates and total volumes
  float flowRate1 = pulseCount1 * calibrationFactor1 / 1000.0; // Convert to liters
  float flowRate2 = pulseCount2 * calibrationFactor2 / 1000.0; // Convert to liters
  float totalFlow1 = flowRate1 * 60; // Convert to ml/min
  float totalFlow2 = flowRate2 * 60; // Convert to ml/min
  
  // Send data to serial monitor
  Serial.print("Flow Rate 1: ");
  Serial.print(flowRate1);
  Serial.print(" ml/min\tTotal Volume 1: ");
  Serial.print(pulseCount1 * calibrationFactor1);
  Serial.println(" ml");
  Serial.print("Flow Rate 2: ");
  Serial.print(flowRate2);
  Serial.print(" ml/min\tTotal Volume 2: ");
  Serial.print(pulseCount2 * calibrationFactor2);
  Serial.println(" ml");
}
