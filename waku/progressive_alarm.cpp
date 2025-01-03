#include "progressive_alarm.h"

int ProgressiveAlarm::calculateBrightness(float progress, bool shouldFlash) const {
    if (!shouldFlash) {
        return map(progress * 100, 0, 100, 1, 255);
    }
    
    // If flashing is active, return either full brightness or off based on flash state
    return flashState ? 255 : 0;
}

int ProgressiveAlarm::calculateBuzzerInterval(float progress) const {
    // Map progress to interval, but only after BUZZER_THRESHOLD
    float adjustedProgress = (progress - BUZZER_THRESHOLD) / (1.0 - BUZZER_THRESHOLD);
    adjustedProgress = constrain(adjustedProgress, 0, 1);
    
    return map(adjustedProgress * 100, 0, 100, INITIAL_INTERVAL, FINAL_INTERVAL);
}

void ProgressiveAlarm::update(float progress, const int* frequencies, const int* durations, int freqSteps) {
    unsigned long currentMillis = millis();
    
    // Handle flashing timing if past flash threshold
    bool shouldFlash = progress >= FLASH_THRESHOLD;
    if (shouldFlash) {
        if (currentMillis - lastFlashTime >= FLASH_INTERVAL) {
            lastFlashTime = currentMillis;
            flashState = !flashState;
        }
    }
    
    // Calculate base brightness
    int brightness = calculateBrightness(progress, shouldFlash);
    
    // Red LED is always on after start
    analogWrite(RED_PIN, brightness);
    
    // Green LED turns on after GREEN_THRESHOLD
    if (progress >= GREEN_THRESHOLD) {
        analogWrite(GREEN_PIN, brightness);
    } else {
        analogWrite(GREEN_PIN, 0);
    }
    
    // Blue LED turns on after BLUE_THRESHOLD
    if (progress >= BLUE_THRESHOLD) {
        analogWrite(BLUE_PIN, brightness);
    } else {
        analogWrite(BLUE_PIN, 0);
    }
    
    // Buzzer control
    if (progress >= BUZZER_THRESHOLD) {
        int currentInterval = calculateBuzzerInterval(progress);
        
        // Check if it's time for a new buzzer cycle
        if (!isBuzzerActive && currentMillis - lastBuzzerTime >= currentInterval) {
            // Calculate buzzer parameters based on progress
            int freqIndex = map(progress * 100, 
                              BUZZER_THRESHOLD * 100, 100, 
                              0, freqSteps - 1);
            freqIndex = constrain(freqIndex, 0, freqSteps - 1);
            
            // Start new buzzer cycle
            if (!shouldFlash || flashState) {
                tone(BUZZER_PIN, frequencies[freqIndex], durations[freqIndex]);
                isBuzzerActive = true;
                lastBuzzerTime = currentMillis;
                
                // Debug output
                Serial.print("Buzzer: Interval=");
                Serial.print(currentInterval);
                Serial.print("ms, Freq=");
                Serial.print(frequencies[freqIndex]);
                Serial.print("Hz, Duration=");
                Serial.print(durations[freqIndex]);
                Serial.println("ms");
            }
        } else if (isBuzzerActive && currentMillis - lastBuzzerTime >= durations[0]) {
            // Turn off buzzer after duration
            noTone(BUZZER_PIN);
            isBuzzerActive = false;
        }
    } else {
        noTone(BUZZER_PIN);
        isBuzzerActive = false;
    }
}

void ProgressiveAlarm::stop() {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    noTone(BUZZER_PIN);
    flashState = false;
    isBuzzerActive = false;
} 