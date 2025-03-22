// Include necessary libraries
#include <WiFi.h>
#include <ESP32Servo.h>

// Wi-Fi configuration
const char* ssid = "SAIKOT-YUU";
const char* password = "1S1S1S1S";
WiFiServer server(8080);

// Motor driver configuration
const int motorPin1 = 25;
const int motorPin2 = 26;
const int motorPin3 = 27;
const int motorPin4 = 14;

// Speed sensor configuration
#define SENSOR_PIN 18
volatile unsigned long pulseCount = 0;
unsigned long lastTime = 0;
float speed = 0;

// Ultrasonic sensor configuration
#define TRIG_PIN 5
#define ECHO_PIN 4
const int stopDistance = 10;  // Stop if obstacle is closer than 10cm

// Interrupt function for speed sensor
void IRAM_ATTR onPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), onPulse, RISING);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

// Improved distance function with error handling
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 300000);  // Timeout at 30ms (~5m max range)

  if (duration == 0) {
    return 1000.0;  // Assume far away to prevent false stops
  }

  float distance = (duration * 0.034) / 2; // Convert to cm

  if (distance < 2 || distance > 400) {
    return 1000.0;  // Ignore values outside sensor range
  }

  return distance;
}

//Function to check for speed
void checkSpeed() {
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 1000) {  // Update every 1 second
    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float wheelDiameter = 0.68;  // 68mm = 0.68cm
    float circumference = wheelDiameter * 3.1416;

    int pulsesPerRevolution = 20;  // Adjust based on your encoder specs
    float revolutionsPerSecond = (float)pulses / pulsesPerRevolution;
    
    speed = circumference * revolutionsPerSecond;  // Speed in cm/s

    lastTime = currentTime;
  }
}

// Function to stop the car
void stopCar() {
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(motorPin3, LOW);
  digitalWrite(motorPin4, LOW);
}

// Function to handle commands
void handleCommand(const String& command, WiFiClient& client) {
  Serial.print("Command received: ");
  Serial.println(command);

  if (command == "forward") {
    float distance = getDistance();
    Serial.print("Measured Distance: ");
    Serial.println(distance);

    if (distance > stopDistance) {
      Serial.println("No obstacle detected, moving forward...");
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);
      digitalWrite(motorPin3, HIGH);
      digitalWrite(motorPin4, LOW);
    } else {
      Serial.println("Obstacle detected! Stopping...");
      stopCar();
    }
  } 
  else if (command == "backward") {
    Serial.println("Moving backward...");
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, HIGH);
  } 
  else if (command == "left") {
    Serial.println("Turning left...");
    digitalWrite(motorPin1, HIGH);
    digitalWrite(motorPin2, LOW);
    digitalWrite(motorPin3, HIGH);
    digitalWrite(motorPin4, LOW);
    delay(500);
  } 
  else if (command == "right") {
    Serial.println("Turning right...");
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, HIGH);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, HIGH);
    delay(500);
  } 
  else if (command == "stop") {
    Serial.println("Stopping the car...");
    stopCar();
  } 
  else if (command == "get_speed") {
    Serial.print("Speed: ");
    Serial.println(speed);
    client.println(speed);
  } 
  else if (command == "get_distance") {
    float distance = getDistance();
    Serial.println(distance);
    client.println(distance);
  } 
  else {
    Serial.println("Unknown command.");
    client.println("Unknown command.");
  }
}

// Main loop
void loop() {
  checkSpeed();
  
  // Automatically stop if obstacle is too close
  float distance = getDistance();
  if (distance <= stopDistance) {
    if (digitalRead(motorPin2) == HIGH && digitalRead(motorPin3) == HIGH) {
      Serial.println("Obstacle detected in auto mode, stopping...");
      stopCar();
    }
  }

  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        String command = client.readStringUntil('\n');
        command.trim();
        handleCommand(command, client);
      }
    }
    client.stop();
  }
}
