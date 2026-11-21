#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Fertilizer System Monitoring"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// Blynk authentication token
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";

// Define pin numbers for pumps, solenoid valves, and flow sensors
#define PUMP1_PIN 16
#define PUMP2_PIN 17
#define SOLENOID1_PIN 22
#define SOLENOID2_PIN 23
#define FLOW_SENSOR1_PIN 18
#define FLOW_SENSOR2_PIN 19

// Variables to keep track of dispensed volume and total volume
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;
unsigned long totalVolume1 = 0;
unsigned long totalVolume2 = 0;
unsigned long totalVolume = 0;

// Calibration factors for flow sensors
float calibrationFactor1 = 4.5;  // Update this value based on your calibration
float calibrationFactor2 = 4.5;  // Update this value based on your calibration

// Function prototypes
void dispenseVolume(unsigned long desiredVolume);
void calibrateFlowSensors();

void setup() {
  Serial.begin(9600);

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize Blynk
  Serial.print("Connecting to Blynk...");
  Blynk.begin(auth, ssid, pass);
  while (!Blynk.connected()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Blynk");

  // Configure pins for pumps, solenoid valves, and flow sensors
  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(SOLENOID1_PIN, OUTPUT);
  pinMode(SOLENOID2_PIN, OUTPUT);
  pinMode(FLOW_SENSOR1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR2_PIN, INPUT_PULLUP);

  // Attach interrupt routines to flow sensors
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR1_PIN), []() { pulseCount1++; totalVolume1++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR2_PIN), []() { pulseCount2++; totalVolume2++; }, RISING);

  // Calibrate flow sensors
  calibrateFlowSensors();
}

void loop() {
  Blynk.run();
}

BLYNK_WRITE(V1) {
  unsigned long desiredVolume = param.asInt();
  Serial.print("Dispensing ");
  Serial.print(desiredVolume);
  Serial.println(" ml from each side.");

  // Dispense the desired volume
  dispenseVolume(desiredVolume);
}

void calibrateFlowSensors() {
  // Calibrate flow sensors here (optional)
}

void dispenseVolume(unsigned long desiredVolume) {
  // Reset pulse counts and total volume
  pulseCount1 = 0;
  pulseCount2 = 0;
  totalVolume1 = 0;
  totalVolume2 = 0;

  // Open solenoid valves
  digitalWrite(SOLENOID1_PIN, HIGH);
  digitalWrite(SOLENOID2_PIN, HIGH);

  // Turn on pumps
  digitalWrite(PUMP1_PIN, HIGH);
  digitalWrite(PUMP2_PIN, HIGH);

  // Delay to allow pumps to reach desired flow rates
  delay(1000); // Adjust delay time as needed

  // Variables to keep track of dispensed volume for each side
  unsigned long dispensedVolume1 = 0;
  unsigned long dispensedVolume2 = 0;

  // Dispense from pumps until desired volume is reached for both sides
  while (dispensedVolume1 < desiredVolume && dispensedVolume2 < desiredVolume) {
    // Update dispensed volume based on pulse counts
    dispensedVolume1 = pulseCount1 * calibrationFactor1;
    dispensedVolume2 = pulseCount2 * calibrationFactor2;

    // Update total volume
    totalVolume = dispensedVolume1 + dispensedVolume2;
  }

  // Turn off pumps
  digitalWrite(PUMP1_PIN, LOW);
  digitalWrite(PUMP2_PIN, LOW);

  // Close solenoid valves
  digitalWrite(SOLENOID1_PIN, LOW);
  digitalWrite(SOLENOID2_PIN, LOW);

  // Print total volume to Serial Monitor
  Serial.print("Total Volume Dispensed: ");
  Serial.print(totalVolume);
  Serial.println(" ml");

  // Send total volume to Blynk app
  Blynk.virtualWrite(V2, totalVolume);

  // Dispensing complete status
  Serial.println("Dispensing complete.");
}