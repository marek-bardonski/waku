#include "server_client.h"

bool ServerClient::validateDeviceAndGetAlarmTime(const char* errorCode, int& hour, int& minute) {
    String params = "error=";
    params += errorCode ? errorCode : "";
    
    Serial.print("Connecting to server at ");
    Serial.print(serverHost);
    Serial.print(":");
    Serial.println(serverPort);
    
    String response = makeHttpRequest("/api/device/validate", params.c_str());
    if (response.length() == 0) {
        displayManager.displayMessage("SERVER ERR");
        return false;
    }
    
    Serial.print("Server response: ");
    Serial.println(response);
    
    // Parse JSON response
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        displayManager.displayMessage("JSON ERR");
        return false;
    }
    
    const char* timeStr = doc["time"];
    if (!timeStr) {
        Serial.println("No time field in response");
        displayManager.displayMessage("NO TIME");
        return false;
    }
    
    if (parseTimeString(timeStr, hour, minute)) {
        char message[20];
        snprintf(message, sizeof(message), "ALARM %02d:%02d", hour, minute);
        displayManager.displayMessage(message);
        return true;
    }
    
    displayManager.displayMessage("TIME ERR");
    return false;
}

bool ServerClient::parseTimeString(const char* timeStr, int& hour, int& minute) {
    // Expected format: "HH:MM"
    if (strlen(timeStr) != 5 || timeStr[2] != ':') {
        return false;
    }
    
    char hourStr[3] = {timeStr[0], timeStr[1], '\0'};
    char minStr[3] = {timeStr[3], timeStr[4], '\0'};
    
    hour = atoi(hourStr);
    minute = atoi(minStr);
    
    return hour >= 0 && hour < 24 && minute >= 0 && minute < 60;
}

String ServerClient::makeHttpRequest(const char* endpoint, const char* params) {
    if (!client.connect(serverHost, serverPort)) {
        Serial.println("Connection failed");
        return "";
    }
    
    String url = String(endpoint);
    if (params && strlen(params) > 0) {
        url += "?";
        url += params;
    }
    
    Serial.print("Making request to: ");
    Serial.println(url);
    
    // Make HTTP request
    client.print("GET ");
    client.print(url);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverHost);
    client.println("Connection: close");
    client.println();
    
    // Wait for response
    unsigned long timeout = millis();
    while (!client.available()) {
        if (millis() - timeout > 5000) {
            Serial.println("Request timeout");
            client.stop();
            return "";
        }
    }
    
    // Skip headers
    bool headersParsed = false;
    while (client.available() && !headersParsed) {
        String line = client.readStringUntil('\n');
        Serial.print("Header: ");
        Serial.println(line);
        if (line == "\r") {
            headersParsed = true;
        }
    }
    
    if (!headersParsed) {
        Serial.println("Failed to parse headers");
        return "";
    }
    
    // Read response body
    String response = client.readString();
    client.stop();
    
    return response;
} 