#include "sound_sensor.h"
#include <Arduino.h>

SoundSensor::SoundSensor(int analogPin) 
    : pin(analogPin), lastVolume(0), lastReadTime(0), bufferIndex(0), bufferCount(0) {
    // Initialize buffer
    for (int i = 0; i < BUFFER_SIZE; i++) {
        volumeBuffer[i] = 0;
    }
}

bool SoundSensor::begin() {
    pinMode(pin, INPUT);
    return true;
}

bool SoundSensor::read() {
    unsigned long startMillis = millis();
    int signalMax = 0;
    int signalMin = 1024;

    // Collect data for SAMPLE_WINDOW milliseconds
    while (millis() - startMillis < SAMPLE_WINDOW) {
        int sample = analogRead(pin);
        if (sample < 1024) { // Toss out spurious readings
            if (sample > signalMax) {
                signalMax = sample;
            } else if (sample < signalMin) {
                signalMin = sample;
            }
        }
    }

    lastVolume = signalMax - signalMin;
    lastReadTime = millis();
    
    // Store in circular buffer
    volumeBuffer[bufferIndex] = lastVolume;
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
    if (bufferCount < BUFFER_SIZE) {
        bufferCount++;
    }
    
    return true;
}

int SoundSensor::getMaxVolume() const {
    if (bufferCount == 0) {
        return 0;
    }
    
    int maxVolume = volumeBuffer[0];
    for (int i = 1; i < bufferCount; i++) {
        if (volumeBuffer[i] > maxVolume) {
            maxVolume = volumeBuffer[i];
        }
    }
    return maxVolume;
}

bool SoundSensor::isTimeToRead() const {
    return (millis() - lastReadTime) >= READ_INTERVAL;
} 