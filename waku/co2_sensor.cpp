#include "co2_sensor.h"
#include <Arduino.h>

CO2Sensor::CO2Sensor(int pwmPin) 
    : pwmPin(pwmPin), lastTemperature(0.0f), lastCO2(0), lastReadTime(0), bufferIndex(0), bufferCount(0) {
    Serial.print("CO2 Sensor created with PWM pin: ");
    Serial.println(pwmPin);
    
    // Initialize buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        co2Buffer[i] = 0;
    }
}

bool CO2Sensor::begin() {
    Serial.println("\n=== CO2 Sensor Initialization ===");
    Serial.println("Configuring PWM pin...");
    
    pinMode(pwmPin, INPUT);
    
    Serial.println("Starting warm-up sequence...");
    Serial.println("Waiting 10 seconds for sensor to stabilize...");
    // The sensor needs time to warm up and stabilize
    for (int i = 0; i < 10; i++) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nWarm-up complete");
    
    // Try reading multiple times
    Serial.println("Attempting to read sensor (3 attempts)...");
    for (int attempt = 0; attempt < 3; attempt++) {
        Serial.print("\nAttempt ");
        Serial.print(attempt + 1);
        Serial.println(":");
        
        if (read()) {
            Serial.println("CO2 sensor initialized successfully!");
            Serial.println("================================");
            return true;
        }
        
        delay(2000); // Wait between attempts
    }
    
    Serial.println("\nFailed to initialize sensor");
    return false;
}

int CO2Sensor::readPWM() {
    // MH-Z19B PWM cycle is 1004ms:
    // - High level = 2ms (fixed) + variable time based on CO2 concentration
    // - Low level = remaining time to complete 1004ms cycle
    // PPM = 5000 * (Th - 2ms) / (1004ms - 4ms)
    
    // First, wait for a complete cycle to ensure we start at the right phase
    unsigned long startTime = millis();
    unsigned long timeout = startTime + PWM_CYCLE * 2; // Give it 2 cycles to catch a full pulse
    
    // Wait for LOW (in case we're in the middle of a HIGH pulse)
    while (digitalRead(pwmPin) == HIGH) {
        if (millis() > timeout) {
            Serial.println("Timeout waiting for initial LOW");
            return -1;
        }
    }
    
    // Wait for rising edge
    while (digitalRead(pwmPin) == LOW) {
        if (millis() > timeout) {
            Serial.println("Timeout waiting for rising edge");
            return -1;
        }
    }
    
    // Now we're at the start of a HIGH pulse
    unsigned long highStart = micros();
    
    // Wait for falling edge
    while (digitalRead(pwmPin) == HIGH) {
        if (millis() > timeout) {
            Serial.println("Timeout measuring HIGH pulse");
            return -1;
        }
    }
    unsigned long highTime = micros() - highStart;
    
    // Wait for next rising edge to get the complete cycle time
    unsigned long lowStart = micros();
    while (digitalRead(pwmPin) == LOW) {
        if (millis() > timeout) {
            Serial.println("Timeout measuring LOW pulse");
            return -1;
        }
    }
    unsigned long lowTime = micros() - lowStart;
    unsigned long cycleTime = highTime + lowTime;
    
    // Validate cycle time (should be close to 1004ms = 1004000µs)
    if (cycleTime < 900000 || cycleTime > 1100000) {
        Serial.print("Invalid cycle time: ");
        Serial.print(cycleTime);
        Serial.println("µs");
        return -1;
    }
    
    // Calculate CO2 concentration
    // The formula from the datasheet is: ppm = 5000 * (Th - 2ms) / (1004ms - 4ms)
    // where Th is the high level time in milliseconds
    // We're working in microseconds, so we need to adjust the formula
    const unsigned long TH_OFFSET = 2000;  // 2ms in microseconds
    const unsigned long CYCLE_TIME = 1000000;  // 1000ms in microseconds
    
    if (highTime <= TH_OFFSET) {
        Serial.println("High time too short");
        return -1;
    }
    
    unsigned long Th = highTime - TH_OFFSET;
    int ppm = (5000L * Th) / (CYCLE_TIME - 4000);  // 4000 = 4ms in microseconds
    
    // Validate the reading
    if (ppm < 0 || ppm > MAX_PPM) {
        Serial.println("PPM out of valid range");
        return -1;
    }
    
    return ppm;
}

bool CO2Sensor::read() {
    Serial.println("\n--- Reading CO2 sensor via PWM ---");
    
    // Take a single reading
    int ppm = readPWM();
    if (ppm < 0 || ppm > MAX_PPM) {
        Serial.println("Failed to get valid reading from CO2 sensor");
        return false;
    }
    
    lastCO2 = ppm;
    lastReadTime = millis();
    
    // Store in circular buffer
    co2Buffer[bufferIndex] = lastCO2;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
    if (bufferCount < BUFFER_SIZE) {
        bufferCount++;
    }
    
    // Note: Temperature is not available in PWM mode
    lastTemperature = 0;
    
    return true;
}

float CO2Sensor::getAverageCO2() const {
    if (bufferCount == 0) {
        return 0;
    }
    
    long sum = 0;
    for (int i = 0; i < bufferCount; i++) {
        sum += co2Buffer[i];
    }
    return static_cast<float>(sum) / bufferCount;
}

bool CO2Sensor::isTimeToRead() const {
    return (millis() - lastReadTime) >= READ_INTERVAL;
} 