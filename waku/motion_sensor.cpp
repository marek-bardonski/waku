#include "motion_sensor.h"

MotionSensor::MotionSensor(int pin, unsigned long debugInterval)
    : sensorPin(pin)
    , DEBUG_INTERVAL(debugInterval)
    , lastDebugTime(0)
{
    pinMode(sensorPin, INPUT);
    Serial.println("PIR sensor initialized on pin " + String(sensorPin));
}

bool MotionSensor::checkMotion() {
    return digitalRead(sensorPin) == HIGH;
}

void MotionSensor::printDebug(bool isWakeUpTime, bool alarmTriggered) {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastDebugTime >= DEBUG_INTERVAL) {
        lastDebugTime = currentMillis;
        
        bool motionState = digitalRead(sensorPin);
        
        Serial.print("Motion Debug - State: ");
        Serial.print(motionState ? "HIGH" : "LOW");
        Serial.print(" | Wake-up active: ");
        Serial.print(isWakeUpTime ? "YES" : "NO");
        Serial.print(" | Alarm triggered: ");
        Serial.println(alarmTriggered ? "YES" : "NO");
    }
} 