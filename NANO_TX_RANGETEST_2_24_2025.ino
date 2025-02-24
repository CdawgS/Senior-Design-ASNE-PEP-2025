#include <SPI.h>
#include <RF24.h>
//NANO RANGE TEST CODE
// NRF24L01 setup
RF24 radio(7, 6); // CE, CSN pins
//transmission testing TX
// Communication pipe address
const uint64_t pipe = 0xF0F0F0F0E1LL;
 
// Analog pins for rudder and throttle
const int rudderPin = A0;
const int throttlePin = A1;
const int throttleLimiterPin = A2;
 
// Data structure for transmission
struct ControlData {
    uint8_t rudder;
    uint8_t throttle;
    uint8_t throttleMode;
} __attribute__((packed));
 
void setup() {
    Serial.begin(115200);
    // Initialize NRF24L01
    Serial.println("Initializing Transmitter...");
    if (!radio.begin()) {
        Serial.println("RF24 initialization failed!");
        while (1); // Halt if radio fails
    }
    radio.openWritingPipe(pipe);
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.stopListening(); // Transmit mode
 
    Serial.println("Transmitter ready.");
}
 
void loop() {
    Serial.println("Starting transmission of 100 packets...");
    for (int i = 0; i < 100; i++) {
        uint16_t rawRudder = analogRead(rudderPin);   
        uint16_t rawThrottle = analogRead(throttlePin);
        uint16_t rawThrottleLimiter = analogRead(throttleLimiterPin);
 
        uint8_t rudderValue = map(rawRudder, 0, 1023, 0, 255);
        uint8_t throttleValue = map(rawThrottle, 0, 1023, 0, 255);
 
        uint8_t throttleMode;
        if (rawThrottleLimiter < 204) {
            throttleMode = 0;
        } else if (rawThrottleLimiter < 409) {
            throttleMode = 1;
        } else if (rawThrottleLimiter < 614) {
            throttleMode = 2;
        } else if (rawThrottleLimiter < 819) {
            throttleMode = 3;
        } else {
            throttleMode = 4;
        }
 
        ControlData data = {rudderValue, throttleValue, throttleMode};
        bool success = radio.write(&data, sizeof(data));
 
        Serial.print("Packet ");
        Serial.print(i + 1);
        Serial.print(": Rudder=");
        Serial.print(data.rudder);
        Serial.print(", Throttle=");
        Serial.print(data.throttle);
        Serial.print(", Mode=");
        Serial.println(data.throttleMode);
 
        if (!success) {
            Serial.println("Message failed to send.");
        }
 
        delay(150); // 100 packets in ~15 sec
    }
 
    Serial.println("Transmission Complete.");
    while (1); // Halt after sending 100 packets
}
