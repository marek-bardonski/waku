#include <SPI.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <RTC.h>
#include "alarm.h"
#include "motion_sensor.h"
#include "server_client.h"
#include "display_manager.h"
#include "arduino_secrets.h"

// Recovery settings
const unsigned long WATCHDOG_TIMEOUT = 8000;  // 8 seconds
const int MAX_RECOVERY_ATTEMPTS = 3;
const unsigned long RECOVERY_DELAY = 10000;   // 10 seconds between recovery attempts
unsigned long lastRecoveryAttempt = 0;
bool systemInitialized = false;
unsigned long lastWatchdogReset = 0;
const unsigned long WATCHDOG_RESET_INTERVAL = 4000;  // Reset every 4 seconds

// Server settings
const char* SERVER_HOST = "192.168.0.204";  // Local IP address of the server
const int SERVER_PORT = 8080;

// Pin definitions
const int BUZZER_PIN = 12;
const int PIR_PIN = A2;
const int LED_PINS[] = {9, 10, 11};
const int LED_PIN_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// Buzzer frequencies and durations
const int BEEP_FREQUENCIES[] = {8000, 3500, 3000, 2500, 2000, 1500};
const int BEEP_DURATIONS[] = {20, 100, 200, 300, 500, 1000};
const int FREQ_STEPS = sizeof(BEEP_FREQUENCIES) / sizeof(BEEP_FREQUENCIES[0]);
const int INITIAL_INTERVAL = 10000;
const int FINAL_INTERVAL = 2000;

// WiFi settings
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// Time settings
const int TIME_OFFSET = 3600;  // UTC+1 for Poland (in seconds)
const int WAKE_HOUR = 10;      // Default wake hour, will be updated from server
const int WAKE_MINUTE = 30;    // Default wake minute, will be updated from server
const int WAKE_DURATION = 10;

// Objects
ArduinoLEDMatrix matrix;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
Alarm* alarm = nullptr;
MotionSensor* motionSensor = nullptr;
DisplayManager* displayManager = nullptr;
ServerClient* serverClient = nullptr;

// Time update check
unsigned long lastServerCheck = 0;
const unsigned long SERVER_CHECK_INTERVAL = 5 * 60 * 1000;  // Check every 5 minutes

// Error handling
enum ErrorCode {
    NO_ERROR = 0,
    RTC_INIT_FAILED = 1,
    RTC_SET_TIME_FAILED = 2,
    RTC_GET_TIME_FAILED = 3,
    RTC_ALARM_SET_FAILED = 4,
    NTP_SYNC_FAILED = 5,
    WIFI_CONNECTION_FAILED = 6
};

void printWiFiStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

bool connectToWiFi() {
    int attempts = 0;
    const int maxAttempts = 5;
    
    while (status != WL_CONNECTED && attempts < maxAttempts) {
        Serial.print("Attempt ");
        Serial.print(attempts + 1);
        Serial.print(" of ");
        Serial.print(maxAttempts);
        Serial.print(" - Connecting to SSID: ");
        Serial.println(ssid);
        
        status = WiFi.begin(ssid, pass);
        if (status != WL_CONNECTED) {
            attempts++;
            displayManager->displayError(6);  // Show error briefly between attempts
            
            // If we still have attempts left, wait and try again
            if (attempts < maxAttempts) {
                // Split delay into smaller chunks and keep system responsive
                for (int i = 0; i < 5; i++) {
                    delay(1000);
                }
            }
        }
    }
    
    if (status == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        printWiFiStatus();
        displayManager->displayMessage("WIFI OK");
        ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
        return true;
    } else {
        Serial.println("Failed to connect to WiFi");
        displayManager->displayError(WIFI_CONNECTION_FAILED);
        return false;
    }
}

bool setupTime() {
    if (!RTC.begin()) {
        Serial.println("RTC initialization failed");
        displayManager->displayError(RTC_INIT_FAILED);
        return false;
    }
    
    Serial.println("RTC initialized successfully");
    timeClient.begin();
    timeClient.setTimeOffset(TIME_OFFSET);
    
    Serial.println("Updating time from NTP...");
    if (timeClient.update()) {
        RTCTime timeToSet(
            1, Month::JANUARY, 2024,
            timeClient.getHours(),
            timeClient.getMinutes(),
            timeClient.getSeconds(),
            (DayOfWeek)(timeClient.getDay() + 1),
            SaveLight::SAVING_TIME_ACTIVE
        );
        
        Serial.print("Setting RTC time to: ");
        Serial.print(timeToSet.getHour());
        Serial.print(":");
        Serial.print(timeToSet.getMinutes());
        Serial.print(":");
        Serial.println(timeToSet.getSeconds());
        
        if (!RTC.setTime(timeToSet)) {
            Serial.println("Failed to set RTC time");
            displayManager->displayError(RTC_SET_TIME_FAILED);
            return false;
        }
        
        Serial.println("RTC time set successfully");
        displayManager->displayError(NO_ERROR);
        return true;
    }
    
    Serial.println("NTP update failed");
    displayManager->displayError(NTP_SYNC_FAILED);
    return false;
}

void checkAndUpdateAlarmTime() {
    if (!alarm || !serverClient) {
        Serial.println("Alarm or ServerClient not initialized");
        return;
    }
    
    unsigned long currentMillis = millis();
    if (currentMillis - lastServerCheck >= SERVER_CHECK_INTERVAL) {
        lastServerCheck = currentMillis;
        
        Serial.println("Checking server for alarm time update...");
        int newHour, newMinute;
        if (serverClient->validateDeviceAndGetAlarmTime(nullptr, newHour, newMinute)) {
            if (newHour != alarm->getWakeHour() || newMinute != alarm->getWakeMinute()) {
                Serial.print("Updating alarm time to ");
                Serial.print(newHour);
                Serial.print(":");
                Serial.println(newMinute);
                alarm->updateTime(newHour, newMinute);
            } else {
                Serial.println("Alarm time unchanged");
            }
        } else {
            Serial.println("Failed to get alarm time from server");
        }
    }
}

bool initializeSystem() {
    // No explicit watchdog enable needed - Arduino framework handles it
    
    // Try to connect to WiFi
    if (!connectToWiFi()) {
        return false;
    }
    
    // Try to setup time
    if (!setupTime()) {
        return false;
    }
    
    // Initialize server client
    serverClient = new ServerClient(SERVER_HOST, SERVER_PORT, *displayManager);
    
    // Get initial alarm time
    int initialHour = WAKE_HOUR;
    int initialMinute = WAKE_MINUTE;
    bool serverTimeObtained = false;
    
    Serial.println("Getting initial alarm time from server...");
    if (serverClient->validateDeviceAndGetAlarmTime(nullptr, initialHour, initialMinute)) {
        Serial.print("Initial alarm time set to ");
        Serial.print(initialHour);
        Serial.print(":");
        Serial.println(initialMinute);
        serverTimeObtained = true;
    } else {
        return false;
    }
    
    // Create alarm with either server time or defaults
    alarm = new Alarm(initialHour, initialMinute, WAKE_DURATION,
                     LED_PINS, LED_PIN_COUNT,
                     BUZZER_PIN,
                     BEEP_FREQUENCIES, BEEP_DURATIONS, FREQ_STEPS,
                     INITIAL_INTERVAL, FINAL_INTERVAL);
                     
    motionSensor = new MotionSensor(PIR_PIN);
    
    // Reset the server check timer
    lastServerCheck = millis();
    lastRecoveryAttempt = millis();
    lastWatchdogReset = millis();
    
    displayManager->displayMessage("OK");
    return true;
}

void attemptRecovery() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRecoveryAttempt >= RECOVERY_DELAY) {
        lastRecoveryAttempt = currentMillis;
        Serial.println("Attempting system recovery...");
        
        // Try to reinitialize the system
        if (initializeSystem()) {
            systemInitialized = true;
            Serial.println("System recovery successful");
            displayManager->displayMessage("RECOVERED");
        } else {
            Serial.println("System recovery failed");
            displayManager->displayError(7); // New error code for recovery failure
        }
    }
}

void setup() {
    Serial.begin(9600);
    matrix.begin();
    
    // Initialize display first for status messages
    displayManager = new DisplayManager(matrix);
    
    // Initialize the system
    systemInitialized = initializeSystem();
}

void loop() {
    // Handle watchdog reset
    unsigned long currentMillis = millis();
    if (currentMillis - lastWatchdogReset >= WATCHDOG_RESET_INTERVAL) {
        lastWatchdogReset = currentMillis;
        // The watchdog is handled by the Arduino framework
    }
    
    // Check if system needs recovery
    if (!systemInitialized) {
        attemptRecovery();
        return;
    }
    
    // Normal operation
    ArduinoOTA.poll();
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost");
        systemInitialized = false;
        return;
    }
    
    if (alarm && motionSensor) {
        // Check for alarm time updates
        checkAndUpdateAlarmTime();
        
        // Check motion and stop alarm if needed
        if (alarm->isWakeUpTime() && !alarm->isTriggered()) {
            if (motionSensor->checkMotion()) {
                alarm->stopAlarm();
            }
        }
        
        // Update alarm state
        alarm->update();
        
        // Print motion debug info
        motionSensor->printDebug(alarm->isWakeUpTime(), alarm->isTriggered());
        
        // Check for midnight reset
        alarm->checkAndResetAtMidnight();
    }
}