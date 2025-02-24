#include <SPI.h>  

#include <RF24.h>  

#include <Servo.h> 

//MEGA RX Range Testing code 

// NRF24L01 setup RF24 radio(7, 8); // CE = Pin 7, CSN = Pin 8  

const uint64_t pipe = 0xF0F0F0F0E1LL; 

// Servo setup  

Servo rudderServo; const int rudderServoPin = 3; 

 Servo throttleServo; const int throttlePwmPin = 5; 

// Data structure  

struct ControlData { uint8_t rudder; uint8_t throttle; uint8_t throttleMode; } attribute((packed)); 

ControlData data; int packetCount = 0; 

void setup() {Serial.begin(9600); 

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

void loop() { if (radio.available()) { memset(&data, 0, sizeof(data)); radio.read(&data, sizeof(data)); 

   // Convert rudder from 0-255 to angle 
    int rudderAngle = map(data.rudder, 0, 255, 60, 120); 
    rudderServo.write(rudderAngle); 
 
    // Set throttle range 
    int throttleRangeMax; 
    switch (data.throttleMode) { 
        case 0: throttleRangeMax = 1200; break; 
        case 1: throttleRangeMax = 1400; break; 
        case 2: throttleRangeMax = 1600; break; 
        case 3: throttleRangeMax = 1800; break; 
        case 4: default: throttleRangeMax = 2000; break; 
    } 
 
    // Convert throttle to PWM 
    int throttlePwmSignal = map(data.throttle, 0, 255, 1000, throttleRangeMax); 
    throttleServo.writeMicroseconds(throttlePwmSignal); 
 
    // Count packets 
    packetCount++; 
 
    // Debugging Output 
    Serial.print("Packet "); 
    Serial.print(packetCount); 
    Serial.print(": Rudder="); 
    Serial.print(data.rudder); 
    Serial.print(" ("); 
    Serial.print(rudderAngle); 
    Serial.print(" deg), Throttle="); 
    Serial.print(data.throttle); 
    Serial.print(" ("); 
    Serial.print(throttlePwmSignal); 
    Serial.print(" us), Mode="); 
    Serial.println(data.throttleMode); 
 
    Serial.print("Total Packets Received: "); 
    Serial.println(packetCount); 
 
    // Stop after 100 packets 
    if (packetCount >= 100) { 
        Serial.println("Receiving Complete."); 
        while (1); // Halt after receiving 100 packets 
    } 
} 
  

} 