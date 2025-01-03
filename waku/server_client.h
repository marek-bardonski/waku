#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoJson.h>
#include "display_manager.h"

class ServerClient {
private:
    const char* serverHost;
    const int serverPort;
    WiFiClient client;
    DisplayManager& displayManager;
    
public:
    ServerClient(const char* host, int port, DisplayManager& display) 
        : serverHost(host), serverPort(port), displayManager(display) {}
    
    bool validateDeviceAndGetAlarmTime(const char* errorCode, int& hour, int& minute);

private:
    bool parseTimeString(const char* timeStr, int& hour, int& minute);
    String makeHttpRequest(const char* endpoint, const char* params);
};

#endif 