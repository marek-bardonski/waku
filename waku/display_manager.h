#pragma once
#include <Arduino.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

class DisplayManager {
private:
    ArduinoLEDMatrix& matrix;
    const int scrollSpeed;
    
    // Message display state
    unsigned long displayStartTime;
    bool isDisplaying;
    const char* currentMessage;
    int currentErrorCode;
    bool isError;
    
    static const unsigned long MESSAGE_DISPLAY_TIME = 3000;  // 3 seconds
    static const unsigned long ERROR_DISPLAY_TIME = 5000;    // 5 seconds

public:
    DisplayManager(ArduinoLEDMatrix& ledMatrix, int speed = 150) 
        : matrix(ledMatrix), scrollSpeed(speed), displayStartTime(0), 
          isDisplaying(false), currentMessage(nullptr), 
          currentErrorCode(0), isError(false) {}
    
    void displayMessage(const char* message);
    void displayError(int errorCode);
    void clear();
    void update();  // Call this regularly to update the display
    bool isBusy() const { return isDisplaying; }
}; 