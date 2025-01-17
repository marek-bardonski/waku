#include "display_manager.h"

void DisplayManager::displayMessage(const char* message) {
    currentMessage = message;
    isError = false;
    isDisplaying = true;
    displayStartTime = millis();
    
    matrix.clear();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(message);
    matrix.endText();
    matrix.endDraw();
}

void DisplayManager::displayError(int errorCode) {
    currentErrorCode = errorCode;
    isError = true;
    isDisplaying = true;
    displayStartTime = millis();
    
    matrix.clear();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    
    char errorText[3];
    if (errorCode == 0) {
        matrix.println("OK");
    } else {
        snprintf(errorText, sizeof(errorText), "E%d", errorCode);
        matrix.println(errorText);
    }
    
    matrix.endText();
    matrix.endDraw();
}

void DisplayManager::clear() {
    matrix.clear();
    matrix.endDraw();
    isDisplaying = false;
}

void DisplayManager::update() {
    if (!isDisplaying) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long displayTime = isError ? ERROR_DISPLAY_TIME : MESSAGE_DISPLAY_TIME;
    
    if (currentTime - displayStartTime >= displayTime) {
        clear();
    }
} 