#include "task_manager.h"
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

// Function to check available RAM
extern "C" char *sbrk(int i);
int freeMemory() {
    char stack_dummy = 0;
    return &stack_dummy - sbrk(0);
}

// Task handles initialization
TaskHandle_t alarmTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;

// Semaphores and mutexes initialization
SemaphoreHandle_t wifiMutex = NULL;
SemaphoreHandle_t displayMutex = NULL;

// Queues initialization
QueueHandle_t sensorDataQueue = NULL;
QueueHandle_t alarmStateQueue = NULL;

// Static member initialization
bool TaskManager::tasksInitialized = false;

void vAlarmTask(void *pvParameters) {
    Serial.println("Alarm Task started");
    Alarm* alarm = (Alarm*)pvParameters;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms period
    
    for(;;) {
        if (alarm) {
            // Check and update alarm state
            bool isWakeTime = alarm->isWakeUpTime();
            bool isTriggered = alarm->isTriggered();
            
            // Check motion sensor data
            SensorData sensorData;
            if (isWakeTime && !isTriggered && xQueuePeek(sensorDataQueue, &sensorData, 0) == pdTRUE) {
                if (sensorData.motionDetected) {
                    Serial.println("Motion detected during wake-up time - stopping alarm");
                    alarm->stopAlarm();
                    isTriggered = true;
                }
            }
            
            // Update alarm
            alarm->update();
            
            // Send alarm state to queue
            AlarmState state = {isTriggered, isWakeTime};
            xQueueOverwrite(alarmStateQueue, &state);
            
            // Check for midnight reset
            alarm->checkAndResetAtMidnight();
        }
        
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vSensorTask(void *pvParameters) {
    Serial.println("Sensor Task started");
    struct {
        CO2Sensor* co2;
        SoundSensor* sound;
        MotionSensor* motion;
    } *sensors = (decltype(sensors))pvParameters;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1 second period
    static uint32_t diagnosticsCounter = 0;
    
    for(;;) {
        SensorData data = {0};
        bool readSuccess = true;
        
        // Read CO2 sensor
        if (sensors->co2 && sensors->co2->isTimeToRead()) {
            int co2_ppm = sensors->co2->readPWM();
            if (co2_ppm >= 0) {
                data.co2Level = co2_ppm;
                Serial.print("CO2 reading successful: ");
                Serial.println(co2_ppm);
            } else {
                Serial.println("CO2 reading failed");
                readSuccess = false;
            }
        }
        
        // Read sound sensor
        if (sensors->sound && sensors->sound->isTimeToRead()) {
            if (!sensors->sound->read()) {
                Serial.println("Sound sensor read failed");
                readSuccess = false;
            }
            data.soundLevel = sensors->sound->getVolume();
        }
        
        // Check motion sensor
        if (sensors->motion) {
            data.motionDetected = sensors->motion->checkMotion();
        }
        
        // Send sensor data to queue only if we have valid readings
        if (readSuccess) {
            xQueueOverwrite(sensorDataQueue, &data);
        } else {
            Serial.println("Sensor read failed");
        }
                
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vNetworkTask(void *pvParameters) {
    Serial.println("Network Task started");
    ServerClient* server = (ServerClient*)pvParameters;
    static uint32_t updateFailCount = 0;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(60 * 1000); // 1 minute period
    
    for(;;) {
        if (server && xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            SensorData sensorData;
            if (xQueuePeek(sensorDataQueue, &sensorData, 0) == pdTRUE) {
                DeviceUpdate update;
                update.CO2Level = sensorData.co2Level;
                update.SoundLevel = sensorData.soundLevel;
                
                AlarmState alarmState;
                if (xQueuePeek(alarmStateQueue, &alarmState, 0) == pdTRUE) {
                    update.AlarmActive = alarmState.isTriggered;
                }
                
                int newHour, newMinute;
                unsigned long currentTime;
                if (!server->sendDeviceUpdateAndGetTime(update, newHour, newMinute, currentTime)) {
                    updateFailCount++;
                    Serial.print("Network update failed. Total fails: ");
                    Serial.println(updateFailCount);
                } else {
                    Serial.println("Network update successful");
                    updateFailCount = 0;
                }
            }
            
            xSemaphoreGive(wifiMutex);
        } else {
            Serial.println("Failed to acquire WiFi mutex for network update");
        }
        
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void vDisplayTask(void *pvParameters) {
    Serial.println("Display Task started");
    DisplayManager* display = (DisplayManager*)pvParameters;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // 100ms period
    
    for(;;) {
        if (display && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Update display based on alarm and sensor states
            AlarmState alarmState;
            if (xQueuePeek(alarmStateQueue, &alarmState, 0) == pdTRUE) {
                if (!display->isBusy()) {  // Only update if not currently displaying
                    if (alarmState.isWakeUpTime) {
                        display->displayMessage("##");
                    } else {
                        display->displayMessage("");
                    }
                }
            }
            
            // Update the display state
            display->update();
            
            xSemaphoreGive(displayMutex);
        }
        
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

bool TaskManager::initializeTasks(
    Alarm* alarm,
    MotionSensor* motionSensor,
    ServerClient* serverClient,
    DisplayManager* displayManager,
    CO2Sensor* co2Sensor,
    SoundSensor* soundSensor
) {
    if (tasksInitialized) {
        return false;
    }
    
    Serial.println("\n=== Creating FreeRTOS Tasks ===");
    
    // Print memory info
    Serial.print("Free RAM before task creation: ");
    Serial.println(freeMemory());
    
    // Create binary semaphores
    wifiMutex = xSemaphoreCreateBinary();
    displayMutex = xSemaphoreCreateBinary();
    
    if (!wifiMutex || !displayMutex) {
        Serial.println("ERROR: Failed to create semaphores");
        return false;
    }
    Serial.println("Semaphores created successfully");
    
    // Initialize semaphores to available state
    xSemaphoreGive(wifiMutex);
    xSemaphoreGive(displayMutex);
    
    // Create queues
    sensorDataQueue = xQueueCreate(1, sizeof(SensorData));
    alarmStateQueue = xQueueCreate(1, sizeof(AlarmState));
    
    if (!sensorDataQueue || !alarmStateQueue) {
        Serial.println("ERROR: Failed to create queues");
        return false;
    }
    Serial.println("Queues created successfully");
    
    Serial.print("Free RAM after queues: ");
    Serial.println(freeMemory());
    
    // Create sensor data structure
    static struct {
        CO2Sensor* co2;
        SoundSensor* sound;
        MotionSensor* motion;
    } sensors = {co2Sensor, soundSensor, motionSensor};
    
    // Create tasks
    BaseType_t status = pdPASS;
    
    Serial.println("Creating Alarm Task...");
    status = xTaskCreate(
        vAlarmTask,
        "AlarmTask",
        ALARM_STACK_SIZE,
        (void*)alarm,
        ALARM_TASK_PRIORITY,
        &alarmTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Alarm Task");
        Serial.print("Free RAM: ");
        Serial.println(freeMemory());
        return false;
    }
    
    Serial.print("Free RAM after Alarm Task: ");
    Serial.println(freeMemory());
    
    Serial.println("Creating Sensor Task...");
    status = xTaskCreate(
        vSensorTask,
        "SensorTask",
        SENSOR_STACK_SIZE,
        (void*)&sensors,
        SENSOR_TASK_PRIORITY,
        &sensorTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Sensor Task");
        Serial.print("Free RAM: ");
        Serial.println(freeMemory());
        return false;
    }
    
    Serial.println("Creating Network Task...");
    status = xTaskCreate(
        vNetworkTask,
        "NetworkTask",
        NETWORK_STACK_SIZE,
        (void*)serverClient,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Network Task");
        return false;
    }
    
    Serial.println("Creating Display Task...");
    status = xTaskCreate(
        vDisplayTask,
        "DisplayTask",
        DISPLAY_STACK_SIZE,
        (void*)displayManager,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Display Task");
        return false;
    }
    
    Serial.println("All tasks created successfully");
    Serial.println("Stack sizes (words):");
    Serial.print("Alarm: "); Serial.println(ALARM_STACK_SIZE);
    Serial.print("Sensor: "); Serial.println(SENSOR_STACK_SIZE);
    Serial.print("Network: "); Serial.println(NETWORK_STACK_SIZE);
    Serial.print("Display: "); Serial.println(DISPLAY_STACK_SIZE);
    
    tasksInitialized = true;
    return true;
}

void TaskManager::suspendAllTasks() {
    if (alarmTaskHandle) vTaskSuspend(alarmTaskHandle);
    if (sensorTaskHandle) vTaskSuspend(sensorTaskHandle);
    if (networkTaskHandle) vTaskSuspend(networkTaskHandle);
    if (displayTaskHandle) vTaskSuspend(displayTaskHandle);
}

void TaskManager::resumeAllTasks() {
    if (alarmTaskHandle) vTaskResume(alarmTaskHandle);
    if (sensorTaskHandle) vTaskResume(sensorTaskHandle);
    if (networkTaskHandle) vTaskResume(networkTaskHandle);
    if (displayTaskHandle) vTaskResume(displayTaskHandle);
} 