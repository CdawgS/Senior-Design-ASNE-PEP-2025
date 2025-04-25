#include <SPI.h>
#include <RF24.h>

RF24 radio(7, 6); // CE, CSN
const uint64_t pipe = 0xF0F0F0F0E1LL;

const int rudderPin = A0; 
const int throttlePin = A1; 
const int throttleLimiterPin = A2;

const int greenLED = 10;
const int redLED = 5;

struct ControlData {
    uint8_t rudder;
    uint8_t throttle;
    uint8_t throttleMode;
} __attribute__((packed));

int consecutiveFails = 0;
const int failThreshold = 3;

void setup() {
    Serial.begin(115200);

    pinMode(greenLED, OUTPUT);
    pinMode(redLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, LOW);

    Serial.println("Initializing Transmitter...");

    if (!radio.begin()) {
        Serial.println("RF24 initialization failed!");
        digitalWrite(redLED, HIGH); // Stay ON if failed
        while (1);
    }

    radio.openWritingPipe(pipe);
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    radio.stopListening();
    Serial.println("Transmitter ready.");

    // Flash green LED for 10 sec
    for (int i = 0; i < 10; i++) {
        digitalWrite(greenLED, HIGH);
        delay(250);
        digitalWrite(greenLED, LOW);
        delay(250);
    }
}

void loop() {
    uint16_t rawRudder = analogRead(rudderPin);   
    uint16_t rawThrottle = analogRead(throttlePin);
    uint16_t rawLimiter = analogRead(throttleLimiterPin);

    uint8_t rudderValue = map(rawRudder, 0, 1023, 0, 255);
    uint8_t throttleValue = map(rawThrottle, 0, 1023, 0, 255);

    uint8_t throttleMode;
    if (rawLimiter < 204) throttleMode = 0;
    else if (rawLimiter < 409) throttleMode = 1;
    else if (rawLimiter < 614) throttleMode = 2;
    else if (rawLimiter < 819) throttleMode = 3;
    else throttleMode = 4;

    ControlData data = {rudderValue, throttleValue, throttleMode};

    bool success = radio.write(&data, sizeof(data));

    Serial.print("Sending: R=");
    Serial.print(data.rudder);
    Serial.print(", T=");
    Serial.print(data.throttle);
    Serial.print(", M=");
    Serial.print(data.throttleMode);

    if (success) {
        Serial.println(" ✅");
        consecutiveFails = 0;
        digitalWrite(redLED, LOW);  // Clear error state
    } else {
        Serial.println(" ❌");
        consecutiveFails++;
        if (consecutiveFails >= failThreshold) {
            digitalWrite(redLED, HIGH);  // Error detected
        }
    }

    delay(20);
}
