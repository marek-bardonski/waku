#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "display_manager.h"
#include "error_codes.h"
#include "co2_sensor.h"
#include "sound_sensor.h"
#include "alarm.h"

struct DeviceUpdate {
    ErrorCode error = ErrorCode::NO_ERROR;
    float CO2Level = 0;
    float SoundLevel = 0;
    bool AlarmActive = false;
    long AlarmActiveTime = 0;
};

struct ServerResponse {
    String alarmTime;
    bool alarmArmed;
    unsigned long currentTime;  // Unix timestamp from server
};

class ServerClient {
private:
    const char* serverHost;
    const int serverPort;
    WiFiClient client;
    DisplayManager& displayManager;
    CO2Sensor* co2Sensor;
    SoundSensor* soundSensor;
    Alarm* alarm;
    
    bool parseTimeString(const char* timeStr, int& hour, int& minute);
    String makeHttpRequest(const char* endpoint, const String& jsonBody);
    void logError(ErrorCode error, const char* message);
    bool parseServerResponse(const String& response, ServerResponse& serverResponse);
    
public:
    ServerClient(const char* host, int port, DisplayManager& display, 
                CO2Sensor* co2, SoundSensor* sound, Alarm* alm) 
        : serverHost(host), serverPort(port), displayManager(display),
          co2Sensor(co2), soundSensor(sound), alarm(alm) {}
    
    bool sendDeviceUpdateAndGetTime(const DeviceUpdate& update, int& hour, int& minute, unsigned long& currentTime);
};

#endif 