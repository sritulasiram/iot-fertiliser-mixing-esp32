#define BLYNK_TEMPLATE_ID "TMPL6ikkFekEo"
#define BLYNK_TEMPLATE_NAME "Fertilizer System Monitoring"
#define BLYNK_AUTH_TOKEN "h4ZNIbSEA7tfR2-2-lDxT8rPJdsPn5PI"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "Galaxy Note5";
char pass[] = "abcd1234";

// Blynk authentication token
char auth[] = "h4ZNIbSEA7tfR2-2-lDxT8rPJdsPn5PI";

// Define pin numbers for pumps, solenoid valves, flow sensors, and TDS sensor
#define PUMP1_PIN 16
#define PUMP2_PIN 17
#define SOLENOID1_PIN 22
#define SOLENOID2_PIN 23
#define FLOW_SENSOR1_PIN 18
#define FLOW_SENSOR2_PIN 19
#define TdsSensorPin 36 // GPIO36 (VP) for ADC1 on ESP32

// Define pin numbers for water level sensors
#define sensor1Pin 34 // GPIO 34 for sensor 1
#define sensor2Pin 35 // GPIO 35 for sensor 2

// Define the virtual pins for Blynk
#define VIRTUAL_PIN1 V1  // For dispensing control
#define VIRTUAL_PIN2 V2  // For total volume dispensed
#define VIRTUAL_PIN3 V3  // For TDS value
#define VIRTUAL_PIN4 V4  // For water level sensor 1 readings
#define VIRTUAL_PIN5 V5  // For water level sensor 2 readings

// Define color constants as RGB values for water level indicators
#define RED     "#FF0000"  // Red color for Low
#define YELLOW  "#FFFF00"  // Yellow color for Medium
#define GREEN   "#00FF00"  // Green color for High

// Calibration factors for flow sensors
float calibrationFactor1 = 5.25;  // Update this value based on your calibration
float calibrationFactor2 = 5.5;  // Update this value based on your calibration

// Variables for flow sensors and dispensing
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;
unsigned long totalVolume1 = 0;
unsigned long totalVolume2 = 0;
unsigned long totalVolume = 0;

// Variables for TDS sensor
#define VREF 3.3        // Analog reference voltage of the ADC
#define SCOUNT 30       // Sum of sample points
int analogBuffer[SCOUNT]; // Store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 23; // Current temperature for compensation
float tdsCalibrationFactor = 2.3; // Calibration factor for TDS calculation

// Create a BlynkTimer instance
BlynkTimer timer;

// Function prototypes
void dispenseVolume(unsigned long desiredVolume);
void calibrateFlowSensors();
int getMedianNum(int bArray[], int iFilterLen);
void readSensors();
String getLevel(int value);
String getLedColor(String level);

void setup() {
  Serial.begin(115200);

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

  // Configure pins for pumps, solenoid valves, flow sensors, and TDS sensor
  pinMode(PUMP1_PIN, OUTPUT);
  pinMode(PUMP2_PIN, OUTPUT);
  pinMode(SOLENOID1_PIN, OUTPUT);
  pinMode(SOLENOID2_PIN, OUTPUT);
  pinMode(FLOW_SENSOR1_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR2_PIN, INPUT_PULLUP);
  pinMode(TdsSensorPin, INPUT);

  // Configure pins for water level sensors
  pinMode(sensor1Pin, INPUT);
  pinMode(sensor2Pin, INPUT);

  // Attach interrupt routines to flow sensors
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR1_PIN), []() { pulseCount1++; totalVolume1++; }, RISING);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR2_PIN), []() { pulseCount2++; totalVolume2++; }, RISING);

  // Set up a timer to read the sensor values every second
  timer.setInterval(1000L, readSensors);

  // Calibrate flow sensors (if needed)
  calibrateFlowSensors();
}

void loop() {
  Blynk.run();
  timer.run();

  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) { // Every 40 milliseconds, read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); // Read the analog value and store into the buffer
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    // Read the analog value more stable by the median filtering algorithm, and convert to voltage value
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0; // Change 1024 to 4096 for ESP32 ADC

    // Temperature compensation formula: fFinalResult(25°C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0))
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    // Temperature compensation
    float compensationVoltage = averageVoltage / compensationCoefficient;

    // Convert voltage value to TDS value, applying the calibration factor
    tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                - 255.86 * compensationVoltage * compensationVoltage
                + 857.39 * compensationVoltage) * 0.5 * tdsCalibrationFactor;

    // Print only the TDS value to the Serial Monitor
    Serial.print("TDS Value: ");
    Serial.print(tdsValue, 0);
    Serial.println(" ppm");

    // Send TDS value to Blynk app on V3
    Blynk.virtualWrite(VIRTUAL_PIN3, tdsValue);
  }
}

BLYNK_WRITE(VIRTUAL_PIN1) {
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
  Blynk.virtualWrite(VIRTUAL_PIN2, totalVolume);

  // Dispensing complete status
  Serial.println("Dispensing complete.");
}

void readSensors() {
  // Read the analog value from each water level sensor
  int sensor1Value = analogRead(sensor1Pin);
  int sensor2Value = analogRead(sensor2Pin);

  // Determine the level for each sensor
  String sensor1Level = getLevel(sensor1Value);
  String sensor2Level = getLevel(sensor2Value);

  // Print the sensor values and levels to the serial monitor
  Serial.print("Sensor 1 Value: ");
  Serial.print(sensor1Value);
  Serial.print(" (");
  Serial.print(sensor1Level);
  Serial.print(")\t");  // Tab character for spacing
  Serial.print("Sensor 2 Value: ");
  Serial.print(sensor2Value);
  Serial.print(" (");
  Serial.print(sensor2Level);
  Serial.println(")");

  // Send the sensor values and levels to the Blynk app
  Blynk.virtualWrite(VIRTUAL_PIN4, sensor1Value);
  Blynk.setProperty(VIRTUAL_PIN4, "color", getLedColor(sensor1Level));

  Blynk.virtualWrite(VIRTUAL_PIN5, sensor2Value);
  Blynk.setProperty(VIRTUAL_PIN5, "color", getLedColor(sensor2Level));
}

String getLevel(int value) {
  if (value <= 700) {
    return "Low";
  } else if (value <= 1200) {
    return "Medium";
  } else {
    return "High";
  }
}

String getLedColor(String level) {
  if (level == "Low") {
    return RED;  // Red color for Low
  } else if (level == "Medium") {
    return YELLOW;  // Yellow color for Medium
  } else {
    return GREEN;  // Green color for High
  }
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0) {
    bTemp = bTab[(iFilterLen - 1) / 2];
  } else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}
