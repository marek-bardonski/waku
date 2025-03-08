#include "button_handler.h"

// Static member initialization
volatile unsigned long ButtonHandler::pressStartTime = 0;
volatile bool ButtonHandler::buttonPressed = false;
ButtonHandler* ButtonHandler::instance = nullptr;
static const unsigned long DEBOUNCE_TIME = 25; // 25ms debounce time
static volatile unsigned long lastInterruptTime = 0;
volatile bool ButtonHandler::stateChanged = false;
volatile bool ButtonHandler::lastState = true;
volatile unsigned long ButtonHandler::lastPressDuration = 0;

void ButtonHandler::buttonISR() {
    if (!instance) return; // Prevent null pointer dereference
    
    unsigned long interruptTime = millis();
    bool currentState = digitalRead(instance->buttonPin);
    
    // If interrupts come faster than the debounce time, assume it's a bounce and ignore
    if (interruptTime - lastInterruptTime < DEBOUNCE_TIME) {
        return;
    }
    lastInterruptTime = interruptTime;

    if (currentState == LOW) { // FALLING edge (button press)
        instance->pressStartTime = interruptTime;
        instance->buttonPressed = true;
        instance->lastState = false;
        instance->stateChanged = true;
    } else { // RISING edge (button release)
        instance->buttonPressed = false;
        instance->lastPressDuration = interruptTime - instance->pressStartTime;
        instance->lastState = true;
        instance->stateChanged = true;
    }
}

void ButtonHandler::begin() {
    pinMode(buttonPin, INPUT_PULLUP);
    lastState = true;  // Initialize to pulled-up state
    stateChanged = false;
    
    // Attach interrupt for both RISING and FALLING edges
    attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, CHANGE);
    Serial.println("Button handler initialized on pin " + String(buttonPin) + " with debouncing");
}

void ButtonHandler::handleShortPress() {
    Serial.println("\nShort press.");

    // If alarm is active, stop it
    if (alarm->isWakeUpTime() && !alarm->isTriggered()) {
        alarm->stopAlarm();
        // TODO: display->displayMessage("STOP");
        return;
    } else {
        display->displayAlarmTime(alarm->getWakeHour(), alarm->getWakeMinute());
        Serial.println("Displaying alarm time " + String(alarm->getWakeHour()) + ":" + String(alarm->getWakeMinute()));
    }
}

void ButtonHandler::handleLongPress() {
    Serial.println("\nLong press - not implemented yet");
}

void ButtonHandler::update() {
    // Move Serial and processing logic outside of interrupt
    if (stateChanged) {
        if (lastState) { // Button was released
            Serial.println("Button released (RISING edge), duration: " + String(lastPressDuration) + "ms");
            if (lastPressDuration >= LONG_PRESS_TIME) {
                handleLongPress();
            } else if (lastPressDuration > DEBOUNCE_TIME) {
                handleShortPress();
            }
        } else { // Button was pressed
            Serial.println("Button pressed (FALLING edge)");
        }
        stateChanged = false;
    }
}

/*
void ButtonHandler::handlePress() {
    if (_displayManager) {
        _displayManager->displayMessage("BTN");
        Serial.println("Button pressed!");  // Add debug output
        
        // Show CO2 value
        if (_co2Sensor) {
            int co2Value = _co2Sensor->readCO2();
            _displayManager->displayNumber(co2Value);
            // Use vTaskDelay instead of delay
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
    
    if (_alarm) {
        _alarm->stop();
    }
} */