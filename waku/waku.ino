#include <SPI.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>
#include <RTC.h>
#include <Arduino_FreeRTOS.h>
#include "alarm.h"
#include "motion_sensor.h"
#include "server_client.h"
#include "display_manager.h"
#include "arduino_secrets.h"
#include "co2_sensor.h"
#include "sound_sensor.h"
#include "error_codes.h"
#include "task_manager.h"

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
const int PIR_PIN = A2;
const int LED_PINS[] = {9, 10, 11};
const int CO2_PWM_PIN = 2;
const int MAX9814_PIN = A0;

const int LED_PIN_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

// Buzzer frequencies and durations
const int BEEP_FREQUENCIES[] = {8000, 3500, 3000, 2500, 2000, 1500};
const int BEEP_DURATIONS[] = {20, 100, 200, 300, 500, 1000};
const int FREQ_STEPS = sizeof(BEEP_FREQUENCIES) / sizeof(BEEP_FREQUENCIES[0]);
const int INITIAL_INTERVAL = 5000;
const int FINAL_INTERVAL = 200;

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
CO2Sensor* co2Sensor = nullptr;
SoundSensor* soundSensor = nullptr;

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
            if (displayManager) {
                displayManager->displayError(static_cast<int>(ErrorCode::WIFI_CONNECTION_FAILED));
            }
            
            if (attempts < maxAttempts) {
                vTaskDelay(pdMS_TO_TICKS(5000));  // Use vTaskDelay instead of Thread::sleep
            }
        }
    }
    
    if (status == WL_CONNECTED) {
        Serial.println("Connected to WiFi");
        if (displayManager) {
            displayManager->displayMessage("W+");
        }
        ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
        return true;
    } else {
        Serial.println("Failed to connect to WiFi");
        if (displayManager) {
            displayManager->displayError(static_cast<int>(ErrorCode::WIFI_CONNECTION_FAILED));
        }
        return false;
    }
}

bool initializeSystem() {
    bool fullInit = true;

    // Try to connect to WiFi
    Serial.println("Connecting to WiFi...");
    if (!connectToWiFi()) {
        Serial.println("WARNING: Operating without WiFi connection");
        fullInit = false;
    }
    
    // Initialize server client if WiFi is available
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Initializing server client...");
        serverClient = new ServerClient(server_host, server_port, *displayManager, co2Sensor, soundSensor, alarm);
        
        // Try to setup time and get initial alarm settings
        Serial.println("Setting up RTC and getting initial time...");
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
            
            Serial.println("Getting time and alarm settings from server...");
            DeviceUpdate initialUpdate;
            if (serverClient->sendDeviceUpdateAndGetTime(initialUpdate, initialHour, initialMinute, currentTime)) {
                RTCTime timeToSet = unixTimeToRTCTime(currentTime);
                if (RTC.setTime(timeToSet)) {
                    Serial.println("RTC updated successfully");
                } else {
                    Serial.println("ERROR: Failed to update RTC time");
                    fullInit = false;
                }
            } else {
                Serial.println("ERROR: Failed to get time from server");
                Serial.println("WARNING: Using default alarm time");
                fullInit = false;
            }
            
            // Create alarm with either server time or defaults
            Serial.println("Creating alarm...");
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
        if (!RTC.begin()) {
            Serial.println("ERROR: RTC initialization failed");
            displayManager->displayError(static_cast<int>(ErrorCode::RTC_INIT_FAILED));
        }
        
        // Create alarm with default values
        alarm = new Alarm(WAKE_HOUR, WAKE_MINUTE, WAKE_DURATION,
                         LED_PINS, LED_PIN_COUNT,
                         BUZZER_PIN,
                         BEEP_FREQUENCIES, BEEP_DURATIONS, FREQ_STEPS,
                         INITIAL_INTERVAL, FINAL_INTERVAL);
    }
    
    Serial.println("Creating motion sensor...");
    motionSensor = new MotionSensor(PIR_PIN);
    
    Serial.println("\n=== Initialization Complete ===");
    Serial.print("Status: ");
    Serial.println(fullInit ? "FULL" : "DEGRADED");
    Serial.println("============================\n");
    
    if (fullInit) {
        displayManager->displayMessage("OK");
    } else {
        displayManager->displayMessage("---");
    }
    
    return fullInit;
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    
    //Wait for serial initialization
    delay(1000);

    Serial.println("\nStarting Waku system...");
    
    matrix.begin();
    displayManager = new DisplayManager(matrix);
    co2Sensor = new CO2Sensor(CO2_PWM_PIN);
    soundSensor = new SoundSensor(MAX9814_PIN);
    
    if (!co2Sensor->begin()) {
        Serial.println("ERROR: Failed to initialize CO2 sensor");
        displayManager->displayError(static_cast<int>(ErrorCode::CO2_SENSOR_INIT_FAILED));
    }
    
    if (!soundSensor->begin()) {
        Serial.println("ERROR: Failed to initialize sound sensor");
        displayManager->displayError(static_cast<int>(ErrorCode::SOUND_SENSOR_INIT_FAILED));
    }
    
    // Initialize the system
    systemInitialized = initializeSystem();
    
    // Initialize FreeRTOS tasks
    if (!TaskManager::initializeTasks(
        alarm,
        motionSensor,
        serverClient,
        displayManager,
        co2Sensor,
        soundSensor
    )) {
        Serial.println("ERROR: Failed to initialize tasks");
        displayManager->displayError(static_cast<int>(ErrorCode::TASK_INIT_FAILED));
        while(1); // Critical error - halt system
    }
    
    // Start the FreeRTOS scheduler
    vTaskStartScheduler();
    Serial.println("Scheduler started. All systems operational.");

    // If we get here, something went wrong with the scheduler
    Serial.println("ERROR: Scheduler failed to start!");
    displayManager->displayError(static_cast<int>(ErrorCode::TASK_INIT_FAILED));
    while(1); // Critical error - halt system
}

void loop() {
    // Empty - FreeRTOS scheduler is running the tasks
    // If we get here, something is wrong with the scheduler
    Serial.println("ERROR: Main loop reached - scheduler not running!");
    delay(1000);
}