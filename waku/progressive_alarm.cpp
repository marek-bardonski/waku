#include "progressive_alarm.h"

void ProgressiveAlarm::calculateLEDIntensities(float progress, int& red, int& green, int& blue) const {
    // Pre-wake phase (progress = 0): Red light ramps up to 10% intensity
    if (progress <= 0.0) {
        // Calculate time since start for initial ramp-up
        unsigned long currentMillis = millis();
        unsigned long timeSinceStart = currentMillis - rampStartTime;
        
        // Ramp from 0% to 10% over 10 minutes
        red = map(constrain(timeSinceStart, 0, INITIAL_RAMP_DURATION), 0, INITIAL_RAMP_DURATION, 0, 25);
        green = 0;
        blue = 0;
        return;
    }
    
    // Full alarm phase (progress = 1.0): Flash all LEDs
    if (progress >= 1.0) {
        if (flashState) {
            red = 255;
            green = 180;
            blue = 120;
        } else {
            red = 0;
            green = 0;
            blue = 0;
        }
        return;
    }
    
    // Dawn simulation phase (0.0 < progress < 1.0)
    // Start from 10% (25) and implement the exact formula from README
    red = progress < 0.5 ? 
          map(progress * 100, 0, 50, 25, 255) : // Ramp from 10% to 100%
          255;
    green = constrain((progress - 0.4) / 0.6 * 180, 0, 180);
    blue = constrain((progress - 0.6) / 0.4 * 120, 0, 120);
}

void ProgressiveAlarm::playNextNote() {
    if (currentNoteIndex >= THEME_LENGTH) {
        currentNoteIndex = 0;  // Loop back to start
    }
    
    // Play the current note
    tone(BUZZER_PIN, THEME_NOTES[currentNoteIndex], NOTE_DURATIONS[currentNoteIndex]);
    lastBuzzerTime = millis();
    isBuzzerActive = true;
    currentNoteIndex++;
}

void ProgressiveAlarm::update(float progress) {
    unsigned long currentMillis = millis();
    
    // Handle flashing timing for full alarm phase
    if (progress >= FLASH_START) {
        if (currentMillis - lastFlashTime >= FLASH_INTERVAL) {
            lastFlashTime = currentMillis;
            flashState = !flashState;
        }
    }
    
    // Calculate and set LED intensities
    int red, green, blue;
    calculateLEDIntensities(progress, red, green, blue);
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
    
    // Buzzer control
    if (progress >= BUZZER_START) {
        if (!isBuzzerActive) {
            // Start next note
            playNextNote();
        } else if (currentMillis - lastBuzzerTime >= NOTE_DURATIONS[currentNoteIndex > 0 ? currentNoteIndex - 1 : THEME_LENGTH - 1]) {
            // Previous note finished, turn off buzzer and prepare for next note
            noTone(BUZZER_PIN);
            noTone(BUZZER_OVERDRIVE_PIN);
            isBuzzerActive = false;
        }
    } else {
        noTone(BUZZER_PIN);
        noTone(BUZZER_OVERDRIVE_PIN);
        isBuzzerActive = false;
        currentNoteIndex = 0;  // Reset to start of theme
    }   
}

void ProgressiveAlarm::stop() {
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    noTone(BUZZER_PIN);
    noTone(BUZZER_OVERDRIVE_PIN);
    flashState = false;
    isBuzzerActive = false;
    currentNoteIndex = 0;  // Reset theme position
    rampStartTime = millis(); // Reset ramp start time when stopping
} 