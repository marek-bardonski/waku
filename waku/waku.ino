#include <SPI.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>
#include <RTC.h>
#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "alarm.h"

#include "global_variables.h"
#include "server_client.h"
#include "display_manager.h"
#include "arduino_secrets.h"
#include "co2_sensor.h"
#include "error_codes.h"
#include "task_manager.h"
#include "button_handler.h"

// Objects
ArduinoLEDMatrix matrix;
Alarm* alarm = nullptr;
DisplayManager* displayManager = nullptr;
ServerClient* serverClient = nullptr;
CO2Sensor* co2Sensor = nullptr;
ButtonHandler* buttonHandler = nullptr;

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
        /* Debugging
        Serial.print("Attempt ");
        Serial.print(attempts + 1);
        Serial.print(" of ");
        Serial.print(maxAttempts);
        Serial.print(" - Connecting to SSID: ");
        Serial.println(ssid);
        */

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
            displayManager->displayMessage("WAKU");
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

    displayManager = new DisplayManager(matrix);
    
    co2Sensor = new CO2Sensor(CO2_PWM_PIN);
    if (!co2Sensor->begin()) {
        Serial.println("ERROR: Failed to initialize CO2 sensor");
        displayManager->displayError(static_cast<int>(ErrorCode::CO2_SENSOR_INIT_FAILED));
        fullInit = false;
    }

    if (!connectToWiFi()) {
        Serial.println("WARNING: Operating without WiFi connection");
        fullInit = false;
    }
    
    // Initialize server client if WiFi is available
    if (WiFi.status() == WL_CONNECTED) {
        alarm = new Alarm(1, 1, WAKE_DURATION,
                        LED_PINS, LED_PIN_COUNT,
                        BUZZER_PIN, BUZZER_OVERDRIVE_PIN);
        
        serverClient = new ServerClient(server_host, server_port, *displayManager, co2Sensor, alarm);
        
        if (!RTC.begin()) {
            Serial.println("ERROR: RTC initialization failed");
            displayManager->displayError(static_cast<int>(ErrorCode::RTC_INIT_FAILED));
            fullInit = false;
        } else {
            int initialHour = WAKE_HOUR;
            int initialMinute = WAKE_MINUTE;
            unsigned long currentTime;
            
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
                         BUZZER_PIN, BUZZER_OVERDRIVE_PIN);
    }
    
    // Initialize button with proper pin mode
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    buttonHandler = new ButtonHandler(BUTTON_PIN, alarm, displayManager, co2Sensor);
    buttonHandler->begin();

    Serial.println("\n=== Initialization Complete ===");
    Serial.print("Status: ");
    Serial.println(fullInit ? "FULL" : "DEGRADED");
    Serial.println("============================\n");
    
    if (!fullInit) 
        displayManager->displayMessage("SAD WAKU");
    
    return fullInit;
}

// Add cleanup function after object declarations
void cleanupResources() {
    if (alarm) { delete alarm; alarm = nullptr; }
    if (displayManager) { delete displayManager; displayManager = nullptr; }
    if (serverClient) { delete serverClient; serverClient = nullptr; }
    if (co2Sensor) { delete co2Sensor; co2Sensor = nullptr; }
    if (buttonHandler) { delete buttonHandler; buttonHandler = nullptr; }
}

void setup() {
    Wire.begin();
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port to connect
    }

    delay(2000);
    Serial.println("\n\nSerial initialized. Waku waking up...");

    // Initialize the system
    systemInitialized = initializeSystem();
    
    // Initialize FreeRTOS tasks
    if (!TaskManager::initializeTasks(
        alarm,
        serverClient,
        displayManager,
        co2Sensor,
        buttonHandler
    )) {
        Serial.println("ERROR: Failed to initialize tasks");
        displayManager->displayError(static_cast<int>(ErrorCode::TASK_INIT_FAILED));
        cleanupResources();  // Clean up before halting
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