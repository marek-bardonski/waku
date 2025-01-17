#ifndef PROGRESSIVE_ALARM_H
#define PROGRESSIVE_ALARM_H

#include <Arduino.h>

class ProgressiveAlarm {
private:
    // LED pins
    const int RED_PIN;    // Pin 10
    const int GREEN_PIN;  // Pin 11
    const int BLUE_PIN;   // Pin 9
    const int BUZZER_PIN;
    
    // Timing thresholds (as percentages)
    const float BUZZER_THRESHOLD = 0.10;  // 10%
    const float GREEN_THRESHOLD = 0.25;   // 25%
    const float BLUE_THRESHOLD = 0.50;    // 50%
    const float FLASH_THRESHOLD = 0.75;   // 75%
    
    // Flashing parameters
    const unsigned long FLASH_INTERVAL = 500;  // Flash every 500ms
    unsigned long lastFlashTime = 0;
    bool flashState = false;
    
    // Buzzer timing control
    unsigned long lastBuzzerTime = 0;
    bool isBuzzerActive = false;
    const int INITIAL_INTERVAL = 5000;  // 5 seconds
    const int FINAL_INTERVAL = 200;     // 200ms
    
    // Calculate brightness based on progress
    int calculateBrightness(float progress, bool shouldFlash) const;
    int calculateBuzzerInterval(float progress) const;
    
public:
    ProgressiveAlarm(int redPin, int greenPin, int bluePin, int buzzerPin)
        : RED_PIN(redPin), GREEN_PIN(greenPin), BLUE_PIN(bluePin), BUZZER_PIN(buzzerPin) {
        pinMode(RED_PIN, OUTPUT);
        pinMode(GREEN_PIN, OUTPUT);
        pinMode(BLUE_PIN, OUTPUT);
        pinMode(BUZZER_PIN, OUTPUT);
        
        // Initialize all outputs to OFF
        analogWrite(RED_PIN, 0);
        analogWrite(GREEN_PIN, 0);
        analogWrite(BLUE_PIN, 0);
        noTone(BUZZER_PIN);
    }
    
    void update(float progress, const int* frequencies, const int* durations, int freqSteps);
    void stop();
};

#endif 