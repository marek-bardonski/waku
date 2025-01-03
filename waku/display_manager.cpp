#include "display_manager.h"

void DisplayManager::displayMessage(const char* message) {
    matrix.clear();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(message);
    matrix.endText();
    matrix.endDraw();
    delay(3000);
    clear();
}

void DisplayManager::displayError(int errorCode) {
    matrix.clear();
    matrix.stroke(0xFFFFFFFF);
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    
    char errorText[10];
    if (errorCode == 0) {
        matrix.println("OK");
    } else {
        snprintf(errorText, sizeof(errorText), "E %d", errorCode);
        matrix.println(errorText);
    }
    
    matrix.endText();
    matrix.endDraw();
    delay(errorCode == 0 ? 3000 : 5000);
    clear();
}

void DisplayManager::clear() {
    matrix.clear();
    matrix.endDraw();
} 