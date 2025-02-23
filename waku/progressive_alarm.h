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
    const int BUZZER_OVERDRIVE_PIN;
    
    // Velvet Horizon Theme - Approximation (in Hz)
    static const int THEME_LENGTH = 80;
    const int THEME_NOTES[THEME_LENGTH] = {
        // Opening sequence (ethereal and dreamy)
        392, 0, 440, 523,           // G4, rest, A4, C5
        440, 0, 392, 349,           // A4, rest, G4, F4
        392, 440, 523, 587,         // G4, A4, C5, D5
        523, 0, 440, 0,             // C5, rest, A4, rest
        
        // Main melody (flowing)
        523, 587, 659, 523,         // C5, D5, E5, C5
        587, 0, 523, 440,           // D5, rest, C5, A4
        392, 440, 523, 0,           // G4, A4, C5, rest
        440, 392, 349, 0,           // A4, G4, F4, rest
        
        // Bridge section (mystical)
        440, 0, 523, 587,           // A4, rest, C5, D5
        659, 0, 587, 523,           // E5, rest, D5, C5
        440, 392, 440, 0,           // A4, G4, A4, rest
        523, 587, 659, 0,           // C5, D5, E5, rest
        
        // Final sequence (ethereal return)
        523, 0, 440, 392,           // C5, rest, A4, G4
        440, 0, 523, 587,           // A4, rest, C5, D5
        523, 0, 440, 0,             // C5, rest, A4, rest
        392, 349, 392, 0            // G4, F4, G4, rest
    };

    const int NOTE_DURATIONS[THEME_LENGTH] = {
        // Opening sequence
        600, 200, 600, 600,         // Longer notes for ethereal feel
        600, 200, 600, 600,
        400, 400, 600, 600,
        600, 200, 600, 800,
        
        // Main melody
        400, 400, 600, 600,         // Flowing rhythm
        600, 200, 400, 400,
        400, 400, 600, 800,
        400, 400, 600, 800,
        
        // Bridge section
        600, 200, 600, 600,         // Mystical timing
        600, 200, 400, 400,
        400, 400, 600, 800,
        400, 400, 600, 800,
        
        // Final sequence
        600, 200, 600, 600,         // Return to ethereal timing
        600, 200, 600, 600,
        600, 200, 600, 800,
        400, 400, 600, 800
    };

    int currentNoteIndex = 0;
        
    // Timing thresholds (as percentages)
    const float BUZZER_START = 1.0;     // Start buzzer at wake-up time
    const float FLASH_START = 1.0;      // Start flashing at wake-up time
    
    // Flashing parameters
    const unsigned long FLASH_INTERVAL = 500;  // Flash every 500ms (120 bpm)
    unsigned long lastFlashTime = 0;
    bool flashState = false;
    
    // Initial ramp-up timing
    unsigned long rampStartTime = 0;
    static const unsigned long INITIAL_RAMP_DURATION = 10 * 60 * 1000; // 10 minutes
    
    // Buzzer timing control
    unsigned long lastBuzzerTime = 0;
    bool isBuzzerActive = false;
    
    // Calculate LED intensities based on progress
    void calculateLEDIntensities(float progress, int& red, int& green, int& blue) const;
    void playNextNote();
    
public:
    ProgressiveAlarm(int redPin, int greenPin, int bluePin, int buzzerPin, int buzzerOverdrivePin)
        : RED_PIN(redPin), GREEN_PIN(greenPin), BLUE_PIN(bluePin), BUZZER_PIN(buzzerPin), BUZZER_OVERDRIVE_PIN(buzzerOverdrivePin) {
        pinMode(RED_PIN, OUTPUT);
        pinMode(GREEN_PIN, OUTPUT);
        pinMode(BLUE_PIN, OUTPUT);
        pinMode(BUZZER_PIN, OUTPUT);
        pinMode(BUZZER_OVERDRIVE_PIN, OUTPUT);
        
        // Initialize all outputs to OFF
        analogWrite(RED_PIN, 0);
        analogWrite(GREEN_PIN, 0);
        analogWrite(BLUE_PIN, 0);
        noTone(BUZZER_PIN);
        noTone(BUZZER_OVERDRIVE_PIN);
        rampStartTime = millis(); // Initialize ramp start time
    }
    
    void update(float progress);
    void stop();
};

#endif 