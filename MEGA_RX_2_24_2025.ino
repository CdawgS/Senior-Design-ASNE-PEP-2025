#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
//ARDUINO MEGA RX CODE 2/24/2025
//DESIGNED BY CANNON SHIELDS 
// NRF24L01 setup
RF24 radio(7, 8); // CE = Pin 7, CSN = Pin 8
const uint64_t pipe = 0xF0F0F0F0E1LL; // Communication pipe address
 
// Servo setup
Servo rudderServo;
const int rudderServoPin = 3; // Connect rudder servo to pin 3
Servo throttleServo;
const int throttlePwmPin = 5; // Use pin 5 for throttle PWM signal
 
// Data structure to hold received values (Reduced size)
struct ControlData {
    uint8_t rudder;         // 0-255 mapped value
    uint8_t throttle;       // 0-255 mapped value
    uint8_t throttleMode;   // 0-4 (Throttle limit mode)
} __attribute__((packed));  
 
ControlData data;
 
void setup() {
    Serial.begin(9600);
 
    // Initialize NRF24L01
    Serial.println("Initializing Receiver...");
    if (!radio.begin()) {
        Serial.println("RF24 initialization failed!");
        while (1);
    }
    radio.openReadingPipe(1, pipe);
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.startListening();
 
    // Initialize servos
    throttleServo.attach(throttlePwmPin);
    rudderServo.attach(rudderServoPin);
}
 
void loop() {
    if (radio.available()) {
        // Clear data before receiving
        memset(&data, 0, sizeof(data));
 
        // Read data
        radio.read(&data, sizeof(data));
 
        // Convert rudder from 0-255 to angle
        int rudderAngle = map(data.rudder, 0, 255, 60, 120);
        rudderServo.write(rudderAngle);
 
        // Set throttle limit based on mode
        int throttleRangeMax;
        switch (data.throttleMode) {
            case 0: throttleRangeMax = 1200; break;  // 20% range
            case 1: throttleRangeMax = 1400; break;  // 40% range
            case 2: throttleRangeMax = 1600; break;  // 60% range
            case 3: throttleRangeMax = 1800; break;  // 80% range
            case 4: default: throttleRangeMax = 2000; break;  // 100% range
        }
 
        // Convert throttle from 0-255 to PWM range
        int throttlePwmSignal = map(data.throttle, 0, 255, 1000, throttleRangeMax);
        throttleServo.writeMicroseconds(throttlePwmSignal);
 
        // Debugging Output
        Serial.print("Rudder=");
        Serial.print(data.rudder);
        Serial.print(" (");
        Serial.print(rudderAngle);
        Serial.print(" deg), Throttle=");
        Serial.print(data.throttle);
        Serial.print(" (");
        Serial.print(throttlePwmSignal);
        Serial.print(" us), Mode=");
        Serial.println(data.throttleMode);
    }
}