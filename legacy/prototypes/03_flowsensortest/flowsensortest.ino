#include <Arduino.h>
#include <BlynkSimpleEsp32.h>

// Pin definitions
#define SOLENOID_VALVE1_PIN 23
#define SOLENOID_VALVE2_PIN 24
#define WATER_PUMP1_PIN 22
#define WATER_PUMP2_PIN 25
#define WATER_FLOW_SENSOR1_PIN 21
#define WATER_FLOW_SENSOR2_PIN 26

volatile int flowRate1, flowRate2; // Measures flow sensor pulses
unsigned int totalFlow1, totalFlow2; // Measures total volume
unsigned long oldTime;

// Blynk auth token
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";

void IRAM_ATTR pulseCounter1()
{
  flowRate1++;
}

void IRAM_ATTR pulseCounter2()
{
  flowRate2++;
}

void setup()
{
  Blynk.begin(auth);

  // Setup pins
  pinMode(SOLENOID_VALVE1_PIN, OUTPUT);
  pinMode(SOLENOID_VALVE2_PIN, OUTPUT);
  pinMode(WATER_PUMP1_PIN, OUTPUT);
  pinMode(WATER_PUMP2_PIN, OUTPUT);
  pinMode(WATER_FLOW_SENSOR1_PIN, INPUT_PULLUP);
  pinMode(WATER_FLOW_SENSOR2_PIN, INPUT_PULLUP);
  
  // Ensure pumps and valves are off at startup
  digitalWrite(SOLENOID_VALVE1_PIN, LOW);
  digitalWrite(SOLENOID_VALVE2_PIN, LOW);
  digitalWrite(WATER_PUMP1_PIN, LOW);
  digitalWrite(WATER_PUMP2_PIN, LOW);

  // Attach interrupt to flow sensor pins
  attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR1_PIN), pulseCounter1, RISING);
  attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR2_PIN), pulseCounter2, RISING);

  oldTime = millis();
}

void loop()
{
  Blynk.run();

  if ((millis() - oldTime) > 1000)  // Only process counters once per second
  {
    detachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR1_PIN));  // Disable the interrupt while calculating flow rate and sending the value to the host
    detachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR2_PIN));  // Disable the interrupt while calculating flow rate and sending the value to the host

    // Because this loop may not complete in exactly 1 second intervals we calculate the number of milliseconds that have passed since the last execution and use that to scale the output
    flowRate1 = (flowRate1 * 1000) / (millis() - oldTime);  // (Pulse frequency x 60) / 7.5Q, = flow rate in L/min
    flowRate2 = (flowRate2 * 1000) / (millis() - oldTime);  // (Pulse frequency x 60) / 7.5Q, = flow rate in L/min

    totalFlow1 += flowRate1; // Add the flow rate to the total volume
    totalFlow2 += flowRate2; // Add the flow rate to the total volume

    oldTime = millis(); // Save the current time for the next loop

    flowRate1 = 0; // Reset the pulse counter so we start counting from 0 again
    flowRate2 = 0; // Reset the pulse counter so we start counting from 0 again

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR1_PIN), pulseCounter1, RISING);
    attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR2_PIN), pulseCounter2, RISING);
  }

  // Send the flow rate and total volume to the Blynk app
  Blynk.virtualWrite(V1, flowRate1); // Send flow rate of first sensor to Blynk app
  Blynk.virtualWrite(V2, totalFlow1); // Send total volume of first sensor to Blynk app
  Blynk.virtualWrite(V3, flowRate2); // Send flow rate of second sensor to Blynk app
  Blynk.virtualWrite(V4, totalFlow2); // Send total volume of second sensor to Blynk app

  // Your existing code to control the solenoid valves and water pumps
  // ...
}
#include <Arduino.h>

// Pin definitions
#define SOLENOID_VALVE_PIN 23
#define WATER_PUMP_PIN 22

void setup()
{
  // Setup pins
  pinMode(SOLENOID_VALVE_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  
  // Ensure pump and valve are off at startup
  digitalWrite(SOLENOID_VALVE_PIN, LOW);
  digitalWrite(WATER_PUMP_PIN, LOW);
}

void loop()
{
  // Turn on the solenoid valve and water pump
  digitalWrite(SOLENOID_VALVE_PIN, HIGH);
  digitalWrite(WATER_PUMP_PIN, HIGH);

  // Wait for 5 seconds
  delay(5000);

  // Turn off the solenoid valve and water pump
  digitalWrite(SOLENOID_VALVE_PIN, LOW);
  digitalWrite(WATER_PUMP_PIN, LOW);

  // Wait for 5 seconds
  delay(5000);
}
