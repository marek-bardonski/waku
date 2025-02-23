#pragma once
#include <Arduino.h>
#include "alarm.h"
#include "display_manager.h"
#include "co2_sensor.h"

class ButtonHandler {
private:
    static const unsigned long LONG_PRESS_TIME = 3000;    // 3 seconds for long press
    
    static volatile unsigned long pressStartTime;
    static volatile bool buttonPressed;
    static volatile bool stateChanged;
    static volatile bool lastState;
    static volatile unsigned long lastPressDuration;
    static ButtonHandler* instance;
    
    const int buttonPin;
    Alarm* alarm;
    DisplayManager* display;
    CO2Sensor* co2Sensor;
    
    static void buttonISR();
    void handleShortPress();
    void handleLongPress();
    
public:
    ButtonHandler(int pin, Alarm* alm, DisplayManager* disp, CO2Sensor* co2)
        : buttonPin(pin), alarm(alm), display(disp), co2Sensor(co2) {
        instance = this;
    }
    
    void begin();
    void update();  // Call this in the main loop or a task
}; 