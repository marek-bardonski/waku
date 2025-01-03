#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <Arduino.h>

class MotionSensor {
private:
    const int sensorPin;
    const unsigned long DEBUG_INTERVAL;
    unsigned long lastDebugTime;

public:
    MotionSensor(int pin, unsigned long debugInterval = 2000);
    bool checkMotion();
    void printDebug(bool isWakeUpTime, bool alarmTriggered);
};

#endif 