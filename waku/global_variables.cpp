#include "global_variables.h"

// Recovery settings
const unsigned long WATCHDOG_TIMEOUT = 8000;  // 8 seconds
const int MAX_RECOVERY_ATTEMPTS = 3;
const unsigned long RECOVERY_DELAY = 10000;   // 10 seconds between recovery attempts
bool systemInitialized = false;

// Server settings
const char* server_host = SERVER_HOST;
const int server_port = SERVER_PORT;

// Pin definitions
const int BUZZER_PIN = 12;
const int BUZZER_OVERDRIVE_PIN = 6; // For future applications. Not used for now.
const int PIR_PIN = A2; // MH-Z19B CO2 sensor
const int LED_PINS[] = {9, 10, 11};
const int CO2_PWM_PIN = 2;
const int MAX9814_PIN = A0; // Not used for now
const int BUTTON_PIN = 3;

const int LED_PIN_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);


// WiFi settings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// Time settings
const int WAKE_HOUR = 10;      // Default wake hour, will be updated from server
const int WAKE_MINUTE = 30;    // Default wake minute, will be updated from server
const int WAKE_DURATION = 10; 