#pragma once
#include <Arduino.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

class DisplayManager {
private:
    ArduinoLEDMatrix& matrix;
    Adafruit_SSD1306 oled;
    const int scrollSpeed;
    bool oledInitialized;
    
    // Message display state
    unsigned long displayStartTime;
    bool isDisplaying;
    const char* currentMessage;
    int currentErrorCode;
    bool isError;
    bool showingCO2;
    
    static const unsigned long MESSAGE_DISPLAY_TIME = 3000;  // 3 seconds
    static const unsigned long ERROR_DISPLAY_TIME = 5000;    // 5 seconds
    static const unsigned long CO2_DISPLAY_TIME = 10000;    // 10 seconds

public:
    DisplayManager(ArduinoLEDMatrix& ledMatrix, int speed = 150) 
        : matrix(ledMatrix), 
          oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
          scrollSpeed(speed), 
          oledInitialized(false),
          displayStartTime(0), 
          isDisplaying(false), 
          currentMessage(nullptr), 
          currentErrorCode(0), 
          isError(false),
          showingCO2(false) {
            
        // Initialize OLED with proper error handling and delays
        delay(100);  // Wait for display to power up
        if(!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            Serial.println(F("SSD1306 allocation failed"));
            return;
        }
        delay(100);  // Wait for display to initialize
        matrix.begin();

        // Verify I2C communication
        Wire.beginTransmission(SCREEN_ADDRESS);
        if (Wire.endTransmission() == 0) {
            oledInitialized = true;
            oled.clearDisplay();
            oled.display();
        } else {
            Serial.println(F("Failed to communicate with OLED"));
        }
    }
    
    void displayMessage(const char* message);
    void clearMessage();

    void displayError(int errorCode);
    void clearError();

    void displayCO2Level(int ppm);
    void displayAlarmTime(int hour, int minute);

    bool isBusy() const { return isDisplaying; }
    bool isOLEDWorking() const { return oledInitialized; }

    void update();
}; 