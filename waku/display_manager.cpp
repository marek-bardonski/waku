#include "display_manager.h"

// Display message on OLED
void DisplayManager::displayMessage(const char* message) {
    currentMessage = message;
    isDisplaying = true;
    showingCO2 = false;
    displayStartTime = millis();
    
    // Update OLED if available
    if (oledInitialized) {
        oled.clearDisplay();
        oled.setTextSize(3);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0,0);
        oled.println(message);
        oled.display();
    }
}

// Display error on Matrix
void DisplayManager::displayError(int errorCode) {
    currentErrorCode = errorCode;
    isError = true;
    isDisplaying = true;
    displayStartTime = millis();
    
    // Update LED matrix
    matrix.clear();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    
    char errorText[3];
    snprintf(errorText, sizeof(errorText), "E%d", errorCode);
    matrix.println(errorText);
    
    matrix.endText();
    matrix.endDraw();
}

void DisplayManager::clearError() {
    matrix.clear();
    matrix.endDraw();
}

void DisplayManager::displayCO2Level(int ppm) {
    if (isDisplaying && !showingCO2) {
        return;
    }
    showingCO2 = true;
    isDisplaying = true;
    displayStartTime = millis();

    char co2Str[7];
    snprintf(co2Str, sizeof(co2Str), "%02d PPM", ppm);

    displayMessage(co2Str);
}

void DisplayManager::displayAlarmTime(int hour, int minute) {
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hour, minute);
    
    showingCO2 = false;
    isDisplaying = true;
    displayStartTime = millis();
    
    displayMessage(timeStr);
}

void DisplayManager::clearMessage() {
    isDisplaying = false;
    showingCO2 = false;

    if (oledInitialized) {
        oled.clearDisplay();
        oled.display();
    }
}

// Update the display
void DisplayManager::update() {
    if (!isDisplaying) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long displayTime;
    
    if (showingCO2) {
        displayTime = CO2_DISPLAY_TIME;
    } else {
        displayTime = MESSAGE_DISPLAY_TIME;
    }
    
    if (currentTime - displayStartTime >= displayTime) {
        clearMessage();
      }
} 