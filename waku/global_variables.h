#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <Arduino.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"

// Recovery settings
extern const unsigned long WATCHDOG_TIMEOUT;  // 8 seconds
extern const int MAX_RECOVERY_ATTEMPTS;
extern const unsigned long RECOVERY_DELAY;   // 10 seconds between recovery attempts
extern bool systemInitialized;

// Server settings
extern const char* server_host;
extern const int server_port;

// Pin definitions
extern const int BUZZER_PIN;
extern const int BUZZER_OVERDRIVE_PIN;
extern const int PIR_PIN;
extern const int LED_PINS[];
extern const int CO2_PWM_PIN;
extern const int MAX9814_PIN;
extern const int BUTTON_PIN;

extern const int LED_PIN_COUNT;

// Buzzer frequencies and durations
extern const int BEEP_FREQUENCIES[];
extern const int BEEP_DURATIONS[];
extern const int FREQ_STEPS;
extern const int INITIAL_INTERVAL;
extern const int FINAL_INTERVAL;

// WiFi settings
extern char ssid[];
extern char pass[];
extern int status;

// Time settings
extern const int WAKE_HOUR;
extern const int WAKE_MINUTE;
extern const int WAKE_DURATION;

#endif // GLOBAL_VARIABLES_H
