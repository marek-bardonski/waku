#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

#include "alarm.h"
#include "motion_sensor.h"
#include "server_client.h"
#include "display_manager.h"
#include "co2_sensor.h"
#include "sound_sensor.h"

// Task handles
extern TaskHandle_t alarmTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern TaskHandle_t displayTaskHandle;

// Semaphores and mutexes
extern SemaphoreHandle_t wifiMutex;
extern SemaphoreHandle_t i2cMutex;
extern SemaphoreHandle_t displayMutex;

// Message queues for inter-task communication
struct SensorData {
    float co2Level;
    float soundLevel;
    bool motionDetected;
};

struct AlarmState {
    bool isTriggered;
    bool isWakeUpTime;
};

extern QueueHandle_t sensorDataQueue;
extern QueueHandle_t alarmStateQueue;

// Task priorities (1-24, where 24 is highest)
#define ALARM_TASK_PRIORITY    2
#define SENSOR_TASK_PRIORITY   2
#define NETWORK_TASK_PRIORITY  1
#define DISPLAY_TASK_PRIORITY  1

// Task stack sizes (in words)
#define ALARM_STACK_SIZE      256
#define SENSOR_STACK_SIZE     256
#define NETWORK_STACK_SIZE    512
#define DISPLAY_STACK_SIZE    256

// Function declarations for tasks
void vAlarmTask(void *pvParameters);
void vSensorTask(void *pvParameters);
void vNetworkTask(void *pvParameters);
void vDisplayTask(void *pvParameters);

// Task manager class
class TaskManager {
private:
    static bool tasksInitialized;

public:
    static bool initializeTasks(
        Alarm* alarm,
        MotionSensor* motionSensor,
        ServerClient* serverClient,
        DisplayManager* displayManager,
        CO2Sensor* co2Sensor,
        SoundSensor* soundSensor
    );
    
    static void suspendAllTasks();
    static void resumeAllTasks();
};

#endif 