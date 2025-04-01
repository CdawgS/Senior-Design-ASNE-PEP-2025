#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

// ARDUINO MEGA RX CODE 2/24/2025
// DESIGNED BY CANNON SHIELDS 

// NRF24L01 setup
RF24 radio(7, 8); // CE = Pin 7, CSN = Pin 8
const uint64_t pipe = 0xF0F0F0F0E1LL; // Communication pipe address

// Servo setup
Servo rudderServo;
const int rudderServoPin = 3; // Rudder servo on pin 3

Servo throttleESC1;
Servo throttleESC2;
const int throttlePwmPin1 = 5; // ESC PWM output 1
const int throttlePwmPin2 = 9; // ESC PWM output 2

// Data structure to hold received values
struct ControlData {
    uint8_t rudder;         // 0-255 mapped value
    uint8_t throttle;       // 0-255 mapped value
    uint8_t throttleMode;   // 0-4 (Throttle limit mode)
} __attribute__((packed));    

ControlData data;
int lastThrottlePwm = 1500; // Start at neutral (1500 µs)
unsigned long lastPacketTime = 0; // Last received packet time
const unsigned long failsafeTimeout = 500; // 500ms timeout before failsafe
bool failsafeActive = false;

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
    throttleESC1.attach(throttlePwmPin1);
    throttleESC2.attach(throttlePwmPin2);
    rudderServo.attach(rudderServoPin);

    // Set throttle to neutral at startup
    throttleESC1.writeMicroseconds(1500);
    throttleESC2.writeMicroseconds(1500);
}

void loop() {
    if (radio.available()) {
        // Clear data before receiving
        memset(&data, 0, sizeof(data));
        
        // Read data
        radio.read(&data, sizeof(data));
        
        // Update last packet time
        lastPacketTime = millis();
        failsafeActive = false;

        // Convert rudder from 0-255 to angle (0-180 degrees)
        int rudderAngle = map(data.rudder, 0, 255, 0, 180);
        rudderServo.write(rudderAngle);

        // Debugging Rudder Output
        Serial.print("Rudder=");
        Serial.print(data.rudder);
        Serial.print(" (Mapped to Angle ");
        Serial.print(rudderAngle);
        Serial.println(" degrees)");

        // **Fixed Throttle Mapping:**
        // - 0 (Full Reverse) -> 1100 µs
        // - 127-128 (Neutral) -> 1500 µs
        // - 255 (Full Forward) -> 2000 µs
        int throttlePwmSignal;
        if (data.throttle <= 127) {
            // Map lower half (Reverse)
            throttlePwmSignal = map(data.throttle, 0, 127, 1100, 1500);
        } else {
            // Map upper half (Forward)
            throttlePwmSignal = map(data.throttle, 128, 255, 1500, 2000);
        }

        // Ensure PWM stays within valid range
        throttlePwmSignal = constrain(throttlePwmSignal, 1100, 2000);

        // Smooth throttle changes to avoid sudden jumps
        if (abs(throttlePwmSignal - lastThrottlePwm) > 10) {
            lastThrottlePwm += (throttlePwmSignal > lastThrottlePwm) ? 10 : -10;
        } else {
            lastThrottlePwm = throttlePwmSignal;
        }

        // Apply PWM to both ESC outputs
        throttleESC1.writeMicroseconds(lastThrottlePwm);
        throttleESC2.writeMicroseconds(lastThrottlePwm);
        
        // Debugging Throttle Output
        Serial.print("Throttle=");
        Serial.print(data.throttle);
        Serial.print(" (Mapped to ");
        Serial.print(lastThrottlePwm);
        Serial.println(" us)");
    } else {
        // Check if we've lost packets for more than the timeout period
        if (millis() - lastPacketTime > failsafeTimeout) {
            if (!failsafeActive) {
                Serial.println("Failsafe activated! Gradually reducing throttle to neutral.");
                failsafeActive = true;
            }

            // Gradually decrease throttle to neutral (1500 µs)
            if (lastThrottlePwm > 1500) {
                lastThrottlePwm -= 5; // Reduce gradually
                if (lastThrottlePwm < 1500) lastThrottlePwm = 1500;
            } else if (lastThrottlePwm < 1500) {
                lastThrottlePwm += 5; // Bring it up to neutral if it's below
                if (lastThrottlePwm > 1500) lastThrottlePwm = 1500;
            }

            // Apply failsafe throttle
            throttleESC1.writeMicroseconds(lastThrottlePwm);
            throttleESC2.writeMicroseconds(lastThrottlePwm);
            
            // Debugging Failsafe
            Serial.print("Failsafe Throttle Adjusting: ");
            Serial.println(lastThrottlePwm);
        }
    }
}

