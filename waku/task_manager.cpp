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
TaskHandle_t networkTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;

// Semaphores and mutexes initialization
SemaphoreHandle_t wifiMutex = NULL;
SemaphoreHandle_t displayMutex = NULL;

// Queues initialization
QueueHandle_t alarmStateQueue = NULL;

// Static member initialization
bool TaskManager::tasksInitialized = false;

// Struct for display task parameters
struct DisplayTaskParams {
    DisplayManager* display;
    CO2Sensor* co2Sensor;
};

struct NetworkTaskParams {
    ServerClient* server;
    CO2Sensor* co2Sensor;
};

void vAlarmTask(void *pvParameters) {
    AlarmTaskParams* params = (AlarmTaskParams*)pvParameters;
    Alarm* alarm = params->alarm;
    ButtonHandler* button = params->button;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10); // 10ms period as per README
    
    for(;;) {
        if (alarm) {
            // Check and update alarm state
            bool isWakeTime = alarm->isWakeUpTime();
            bool isTriggered = alarm->isTriggered();
            
            // Update alarm
            alarm->update();
            
            // Send alarm state to queue
            AlarmState state = {isTriggered, isWakeTime};
            xQueueOverwrite(alarmStateQueue, &state);
            
            // Check for midnight reset
            alarm->checkAndResetAtMidnight();
        }

        // Update button state
        if (button) {
            button->update();
        }
        
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


void vNetworkTask(void *pvParameters) {
    NetworkTaskParams* params = (NetworkTaskParams*)pvParameters;
    ServerClient* server = params->server;
    CO2Sensor* co2Sensor = params->co2Sensor;

    static uint32_t updateFailCount = 0;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(15000); // 15 seconds as per README
    
    for(;;) {
        if (server && xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {

            DeviceUpdate update;
            update.CO2Level = co2Sensor->readPWM();
                
            AlarmState alarmState;
            if (xQueuePeek(alarmStateQueue, &alarmState, 0) == pdTRUE) {
                update.AlarmActive = !alarmState.isTriggered;
            }
                
            int newHour, newMinute;
            unsigned long currentTime;
            if (!server->sendDeviceUpdateAndGetTime(update, newHour, newMinute, currentTime)) {
                updateFailCount++;
                Serial.print("Network update failed. Total fails: ");
                Serial.println(updateFailCount);
            } else {
                //Serial.println("Network update successful");
                updateFailCount = 0;
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
    DisplayTaskParams* params = (DisplayTaskParams*)pvParameters;
    DisplayManager* display = params->display;
    CO2Sensor* co2Sensor = params->co2Sensor;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // 50ms as per README
    
    for(;;) {
        if (display && xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            RTCTime currentTime;
            RTC.getTime(currentTime);
            int currentHour = currentTime.getHour();

            /*
            Not needed now.
            if (currentHour >= 11 && currentHour <= 22 && co2Sensor) {
                int co2Level = co2Sensor->readPWM();
                display->displayCO2Level(co2Level);
            }
            */

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
    ServerClient* serverClient,
    DisplayManager* displayManager,
    CO2Sensor* co2Sensor,
    ButtonHandler* buttonHandler
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
    
    // Initialize semaphores to available state
    xSemaphoreGive(wifiMutex);
    xSemaphoreGive(displayMutex);
    
    // Create queues
    alarmStateQueue = xQueueCreate(1, sizeof(AlarmState));
    
    if (!alarmStateQueue) {
        Serial.println("ERROR: Failed to create queues");
        return false;
    }
    
    Serial.print("Free RAM after queues: ");
    Serial.println(freeMemory());
    
    // Create task parameters on heap to ensure they remain valid
    // These will not be freed as they need to persist for the lifetime of the tasks
    NetworkTaskParams* networkParams = new NetworkTaskParams();
    if (!networkParams) {
        Serial.println("ERROR: Failed to allocate network parameters");
        return false;
    }
    networkParams->server = serverClient;
    networkParams->co2Sensor = co2Sensor;
    
    AlarmTaskParams* alarmParams = new AlarmTaskParams();
    if (!alarmParams) {
        Serial.println("ERROR: Failed to allocate alarm parameters");
        delete networkParams;
        return false;
    }
    alarmParams->alarm = alarm;
    alarmParams->button = buttonHandler;
    
    DisplayTaskParams* displayParams = new DisplayTaskParams();
    if (!displayParams) {
        Serial.println("ERROR: Failed to allocate display parameters");
        delete networkParams;
        delete alarmParams;
        return false;
    }
    displayParams->display = displayManager;
    displayParams->co2Sensor = co2Sensor;
    
    // Create tasks
    BaseType_t status = pdPASS;
    
    status = xTaskCreate(
        vAlarmTask,
        "AlarmTask",
        ALARM_STACK_SIZE,
        (void*)alarmParams,
        ALARM_TASK_PRIORITY,
        &alarmTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Alarm Task");
        Serial.print("Free RAM: ");
        Serial.println(freeMemory());
        delete networkParams;
        delete alarmParams;
        delete displayParams;
        return false;
    }
    
    Serial.print("Free RAM after Alarm Task: ");
    Serial.println(freeMemory());
    
    status = xTaskCreate(
        vNetworkTask,
        "NetworkTask",
        NETWORK_STACK_SIZE,
        (void*)networkParams,
        NETWORK_TASK_PRIORITY,
        &networkTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Network Task");
        delete networkParams;
        delete displayParams;
        vTaskDelete(alarmTaskHandle);
        return false;
    }
    
    status = xTaskCreate(
        vDisplayTask,
        "DisplayTask",
        DISPLAY_STACK_SIZE,
        (void*)displayParams,
        DISPLAY_TASK_PRIORITY,
        &displayTaskHandle
    );
    if (status != pdPASS) {
        Serial.println("ERROR: Failed to create Display Task");
        delete networkParams;
        delete displayParams;
        vTaskDelete(alarmTaskHandle);
        vTaskDelete(networkTaskHandle);
        return false;
    }
    
    Serial.println("All tasks created successfully");
    Serial.println("Stack sizes (words):");
    Serial.print("Alarm: "); Serial.println(ALARM_STACK_SIZE);
    Serial.print("Network: "); Serial.println(NETWORK_STACK_SIZE);
    Serial.print("Display: "); Serial.println(DISPLAY_STACK_SIZE);
    
    tasksInitialized = true;
    return true;
}

void TaskManager::suspendAllTasks() {
    if (alarmTaskHandle) vTaskSuspend(alarmTaskHandle);
    if (networkTaskHandle) vTaskSuspend(networkTaskHandle);
    if (displayTaskHandle) vTaskSuspend(displayTaskHandle);
}

void TaskManager::resumeAllTasks() {
    if (alarmTaskHandle) vTaskResume(alarmTaskHandle);
    if (networkTaskHandle) vTaskResume(networkTaskHandle);
    if (displayTaskHandle) vTaskResume(displayTaskHandle);
} 