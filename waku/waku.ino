#include <SPI.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <ArduinoGraphics.h>
#include "Arduino_LED_Matrix.h"
#include <RTC.h>
#include "alarm.h"
#include "motion_sensor.h"
#include "server_client.h"
#include "display_manager.h"
#include "arduino_secrets.h"
#include "co2_sensor.h"
#include "sound_sensor.h"
#include "error_codes.h"

// Recovery settings
const unsigned long WATCHDOG_TIMEOUT = 8000;  // 8 seconds
const int MAX_RECOVERY_ATTEMPTS = 3;
const unsigned long RECOVERY_DELAY = 10000;   // 10 seconds between recovery attempts
unsigned long lastRecoveryAttempt = 0;
bool systemInitialized = false;
unsigned long lastWatchdogReset = 0;
const unsigned long WATCHDOG_RESET_INTERVAL = 4000;  // Reset every 4 seconds

// Server settings
const char* server_host = SERVER_HOST;  // Local hostname of the server
const int server_port = SERVER_PORT;

// Pin definitions
const int BUZZER_PIN = 12;
const int PIR_PIN = A2;
const int LED_PINS[] = {9, 10, 11};
const int CO2_PWM_PIN = 5;  // Using pin 5 for CO2 sensor PWM input
const int MAX9814_PIN = A0;

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
const int WAKE_HOUR = 10;      // Default wake hour, will be updated from server
const int WAKE_MINUTE = 30;    // Default wake minute, will be updated from server
const int WAKE_DURATION = 10;

// Objects
ArduinoLEDMatrix matrix;
Alarm* alarm = nullptr;
MotionSensor* motionSensor = nullptr;
DisplayManager* displayManager = nullptr;
ServerClient* serverClient = nullptr;

// Time update check
unsigned long lastServerCheck = 0;
const unsigned long SERVER_CHECK_INTERVAL = 5 * 60 * 1000;  // Check every 5 minutes

// Sensor objects
CO2Sensor* co2Sensor = nullptr;
SoundSensor* soundSensor = nullptr;

// Sensor data reporting
unsigned long lastSensorReport = 0;
const unsigned long SENSOR_REPORT_INTERVAL = 5 * 60 * 1000; // 5 minutes

// Time conversion helper
RTCTime unixTimeToRTCTime(unsigned long unixTime) {
    // Convert Unix timestamp to date/time components
    unsigned long seconds = unixTime % 60;
    unsigned long minutes = (unixTime / 60) % 60;
    unsigned long hours = (unixTime / 3600) % 24;
    unsigned long days = (unixTime / 86400) + 1; // +1 because RTCTime expects day 1-31
    
    // Simple calculation for month/year (approximate)
    unsigned long year = 1970 + (days / 365);
    unsigned long month = ((days % 365) / 30) + 1; // +1 because Month enum starts at 1
    days = (days % 365 % 30) + 1; // +1 because RTCTime expects day 1-31
    
    // Calculate day of week (0 = Sunday)
    unsigned long dayOfWeek = (days + 4) % 7; // Jan 1, 1970 was Thursday (4)
    
    return RTCTime(
        days, static_cast<Month>(month), year,
        hours, minutes, seconds,
        static_cast<DayOfWeek>((dayOfWeek % 7) + 1), // Convert to 1-7 range
        SaveLight::SAVING_TIME_ACTIVE
    );
}

void printRTCTime(const RTCTime& time) {
    Serial.print(time.getYear());
    Serial.print("-");
    Serial.print(static_cast<int>(time.getMonth()));
    Serial.print("-");
    Serial.print(time.getDayOfMonth());
    Serial.print(" ");
    Serial.print(time.getHour());
    Serial.print(":");
    Serial.print(time.getMinutes());
    Serial.print(":");
    Serial.println(time.getSeconds());
}

bool updateRTCFromUnixTime(unsigned long unixTime) {
    RTCTime timeToSet = unixTimeToRTCTime(unixTime);
    
    Serial.print("Setting RTC from Unix timestamp: ");
    Serial.println(unixTime);
    Serial.print("Calculated time: ");
    printRTCTime(timeToSet);
    
    if (!RTC.setTime(timeToSet)) {
        Serial.println("Failed to set RTC time");
        return false;
    }
    
    Serial.println("RTC time set successfully");
    return true;
}

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
            displayManager->displayError(static_cast<int>(ErrorCode::WIFI_CONNECTION_FAILED));
            
            if (attempts < maxAttempts) {
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
        displayManager->displayError(static_cast<int>(ErrorCode::WIFI_CONNECTION_FAILED));
        return false;
    }
}

bool initializeSystem() {
    bool fullInit = true;  // Track if we achieved full initialization
    
    // Initialize display first for status messages
    Serial.println("\n=== System Initialization ===");
    
    // Try to connect to WiFi
    Serial.println("1. Connecting to WiFi...");
    if (!connectToWiFi()) {
        Serial.println("WARNING: Operating without WiFi connection");
        fullInit = false;
    }
    
    // Initialize server client if WiFi is available
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("2. Initializing server client...");
        Serial.print("   Server host: ");
        Serial.println(server_host);
        Serial.print("   Server port: ");
        Serial.println(server_port);
        
        serverClient = new ServerClient(server_host, server_port, *displayManager, co2Sensor, soundSensor, alarm);
        
        // Try to setup time and get initial alarm settings
        Serial.println("3. Setting up RTC and getting initial time...");
        if (!RTC.begin()) {
            Serial.println("ERROR: RTC initialization failed");
            displayManager->displayError(static_cast<int>(ErrorCode::RTC_INIT_FAILED));
            fullInit = false;
        } else {
            Serial.println("RTC initialized successfully");
            
            // Get initial time and alarm settings from server
            int initialHour = WAKE_HOUR;
            int initialMinute = WAKE_MINUTE;
            unsigned long currentTime;
            
            Serial.println("4. Getting time and alarm settings from server...");
            Serial.println("   Making request to server...");
            
            DeviceUpdate initialUpdate;
            if (serverClient->sendDeviceUpdateAndGetTime(initialUpdate, initialHour, initialMinute, currentTime)) {
                Serial.println("   Server response received successfully");
                Serial.print("   Current time from server: ");
                Serial.println(currentTime);
                
                if (updateRTCFromUnixTime(currentTime)) {
                    Serial.println("   RTC updated successfully");
                    
                    Serial.print("   Alarm time set to: ");
                    Serial.print(initialHour);
                    Serial.print(":");
                    Serial.println(initialMinute);
                } else {
                    Serial.println("ERROR: Failed to update RTC with server time");
                    fullInit = false;
                }
            } else {
                Serial.println("ERROR: Failed to get time from server");
                Serial.println("WARNING: Using default alarm time");
                fullInit = false;
            }
            
            // Create alarm with either server time or defaults
            Serial.println("5. Creating alarm...");
            alarm = new Alarm(initialHour, initialMinute, WAKE_DURATION,
                            LED_PINS, LED_PIN_COUNT,
                            BUZZER_PIN,
                            BEEP_FREQUENCIES, BEEP_DURATIONS, FREQ_STEPS,
                            INITIAL_INTERVAL, FINAL_INTERVAL);
        }
    } else {
        Serial.println("WARNING: Operating without server connection");
        fullInit = false;
        
        // Try to initialize RTC anyway
        Serial.println("3. Setting up RTC (offline mode)...");
        if (!RTC.begin()) {
            Serial.println("ERROR: RTC initialization failed");
            displayManager->displayError(static_cast<int>(ErrorCode::RTC_INIT_FAILED));
        } else {
            Serial.println("RTC initialized successfully (but no time sync available)");
        }
        
        // Create alarm with default values
        Serial.println("4. Creating alarm with default values...");
        alarm = new Alarm(WAKE_HOUR, WAKE_MINUTE, WAKE_DURATION,
                         LED_PINS, LED_PIN_COUNT,
                         BUZZER_PIN,
                         BEEP_FREQUENCIES, BEEP_DURATIONS, FREQ_STEPS,
                         INITIAL_INTERVAL, FINAL_INTERVAL);
    }
    
    Serial.println("6. Creating motion sensor...");
    motionSensor = new MotionSensor(PIR_PIN);
    
    // Reset timers
    lastServerCheck = millis();
    lastRecoveryAttempt = millis();
    lastWatchdogReset = millis();
    
    Serial.println("\n=== Initialization Complete ===");
    Serial.print("Status: ");
    Serial.println(fullInit ? "FULL" : "DEGRADED");
    Serial.println("============================\n");
    
    if (fullInit) {
        displayManager->displayMessage("OK");
    } else {
        displayManager->displayMessage("OFFLINE");
    }
    
    return true;  // Return true even in degraded mode
}

void attemptRecovery() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastRecoveryAttempt >= RECOVERY_DELAY) {
        lastRecoveryAttempt = currentMillis;
        Serial.println("Attempting system recovery...");
        
        if (initializeSystem()) {
            systemInitialized = true;
            Serial.println("System recovery successful");
            displayManager->displayMessage("RECOVERED");
        } else {
            Serial.println("System recovery failed");
            displayManager->displayError(static_cast<int>(ErrorCode::SYSTEM_RECOVERY_FAILED));
        }
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    Serial.println("\nStarting Waku system...");
    
    matrix.begin();
    Serial.println("LED Matrix initialized");
    
    // Initialize display first for status messages
    displayManager = new DisplayManager(matrix);
    Serial.println("Display manager initialized");
    
    // Initialize sensors
    Serial.println("\nInitializing sensors...");
    
    Serial.println("Creating CO2 sensor instance...");
    co2Sensor = new CO2Sensor(CO2_PWM_PIN);
    
    Serial.println("Creating sound sensor instance...");
    soundSensor = new SoundSensor(MAX9814_PIN);
    
    Serial.println("\nStarting CO2 sensor initialization...");
    if (!co2Sensor->begin()) {
        Serial.println("ERROR: Failed to initialize CO2 sensor");
        displayManager->displayError(static_cast<int>(ErrorCode::CO2_SENSOR_INIT_FAILED));
    } else {
        Serial.println("CO2 sensor initialization successful");
    }
    
    Serial.println("\nStarting sound sensor initialization...");
    if (!soundSensor->begin()) {
        Serial.println("ERROR: Failed to initialize sound sensor");
        displayManager->displayError(static_cast<int>(ErrorCode::SOUND_SENSOR_INIT_FAILED));
    } else {
        Serial.println("Sound sensor initialization successful");
    }
    
    // Initialize the system
    Serial.println("\nInitializing system...");
    systemInitialized = initializeSystem();
}

void checkAndReportSensorData() {
    unsigned long currentMillis = millis();
    
    // Check CO2 and sound sensors every 5 seconds
    if (co2Sensor && co2Sensor->isTimeToRead()) {
        if (co2Sensor->read()) {
            Serial.print("CO2 Level: ");
            Serial.print(co2Sensor->getCO2Level());
            Serial.println(" ppm");
        }
    }
    
    if (soundSensor && soundSensor->isTimeToRead()) {
        if (soundSensor->read()) {
            Serial.print("Sound Level: ");
            Serial.println(soundSensor->getVolume());
        }
    }
    
    // Report to server every 5 minutes
    if (currentMillis - lastSensorReport >= SENSOR_REPORT_INTERVAL) {
        lastSensorReport = currentMillis;
        lastServerCheck = currentMillis; // Update server check time as well
        
        if (co2Sensor && soundSensor && serverClient && alarm) {
            Serial.println("\n=== Sending Sensor Data and Checking Updates ===");
            
            DeviceUpdate update;
            update.CO2Level = co2Sensor->getAverageCO2();  // Get 5-minute average
            update.SoundLevel = soundSensor->getMaxVolume();  // Get 5-minute maximum
            update.AlarmActive = alarm->isTriggered();
            update.AlarmActiveTime = 0;  // Not tracking exact time
            
            int newHour, newMinute;
            unsigned long currentTime;
            if (serverClient->sendDeviceUpdateAndGetTime(update, newHour, newMinute, currentTime)) {
                // Update RTC with server time
                if (updateRTCFromUnixTime(currentTime)) {
                    Serial.println("RTC updated successfully");
                } else {
                    Serial.println("Failed to update RTC time");
                }
                
                // Update alarm time if changed
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
                Serial.println("Failed to communicate with server");
                displayManager->displayError(static_cast<int>(ErrorCode::SERVER_CONNECTION_FAILED));
            }
            
            Serial.println("==============================\n");
        }
    }
}

void loop() {
    // Handle watchdog reset
    unsigned long currentMillis = millis();
    if (currentMillis - lastWatchdogReset >= WATCHDOG_RESET_INTERVAL) {
        lastWatchdogReset = currentMillis;
    }
    
    // Check if system needs recovery
    if (!systemInitialized) {
        attemptRecovery();
        return;
    }
    
    // Normal operation - continue even without WiFi
    if (WiFi.status() == WL_CONNECTED) {
        ArduinoOTA.poll();
    }
    
    // Always check and report sensor data, even without server connection
    checkAndReportSensorData();
    
    if (alarm && motionSensor) {
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