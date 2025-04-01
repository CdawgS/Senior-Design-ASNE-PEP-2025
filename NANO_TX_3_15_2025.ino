#include <SPI.h>
#include <RF24.h>

// NANO TX 2/24/2025
// NRF24L01 setup
RF24 radio(7, 6); // CE, CSN pins (D7, D6 on RP2040)

// Communication pipe address
const uint64_t pipe = 0xF0F0F0F0E1LL;

// Analog pins for rudder and throttle
const int rudderPin = A0; // GPIO26
const int throttlePin = A1; // GPIO27
const int throttleLimiterPin = A2; // Throttle limiter potentiometer

// Data structure for transmission
struct ControlData {
    uint8_t rudder;         // 0-255 mapped value
    uint8_t throttle;       // 0-255 mapped value
    uint8_t throttleMode;   // 0-4 (Throttle limit mode)
} __attribute__((packed));  // Ensure memory alignment

void setup() {
    Serial.begin(115200);
    
    // Initialize NRF24L01
    Serial.println("Initializing Transmitter...");
    if (!radio.begin()) {
        Serial.println("RF24 initialization failed!");
        while (1);
    }
    radio.openWritingPipe(pipe);
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.stopListening();
    Serial.println("Transmitter ready.");
}

void loop() {
    // Read analog values
    uint16_t rawRudder = analogRead(rudderPin);   
    uint16_t rawThrottle = analogRead(throttlePin);
    uint16_t rawThrottleLimiter = analogRead(throttleLimiterPin);
    
    // Map values to 8-bit (0-255)
    uint8_t rudderValue = map(rawRudder, 0, 1023, 0, 255);
    uint8_t throttleValue = map(rawThrottle, 0, 1023, 0, 255);

    // Determine throttle mode based on potentiometer
    uint8_t throttleMode;
    if (rawThrottleLimiter < 204) {
        throttleMode = 0;  // Reverse slow (1200us)
    } else if (rawThrottleLimiter < 409) {
        throttleMode = 1;  // Reverse fast (1400us)
    } else if (rawThrottleLimiter < 614) {
        throttleMode = 2;  // Forward slow (1550us)
    } else if (rawThrottleLimiter < 819) {
        throttleMode = 3;  // Forward medium (1700us)
    } else {
        throttleMode = 4;  // Forward fast (1900us)
    }

    // Prepare data packet
    ControlData data = {rudderValue, throttleValue, throttleMode};
    
    // Transmit data
    bool success = radio.write(&data, sizeof(data));
    
    // Debugging: Print transmission status
    Serial.print("Sending: Rudder=");
    Serial.print(data.rudder);
    Serial.print(", Throttle=");
    Serial.print(data.throttle);
    Serial.print(", Mode=");
    Serial.println(data.throttleMode);
    
    if (success) {
        Serial.println("Message sent successfully!");
    } else {
        Serial.println("Message failed to send.");
    }
    
    delay(20);
}
