#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"

class DisplayManager {
private:
    ArduinoLEDMatrix& matrix;
    const int scrollSpeed;

public:
    DisplayManager(ArduinoLEDMatrix& ledMatrix, int speed = 150) 
        : matrix(ledMatrix), scrollSpeed(speed) {}

    void displayMessage(const char* message);
    void displayError(int errorCode);
    void clear();
};

#endif 