#include "server_client.h"

void ServerClient::logError(ErrorCode error, const char* message) {
    Serial.print("Error: ");
    Serial.print(getErrorString(error));
    Serial.print(" - ");
    Serial.println(message);
    displayManager.displayError(static_cast<int>(error));
}

bool ServerClient::sendDeviceUpdateAndGetTime(const DeviceUpdate& update, int& hour, int& minute, unsigned long& currentTime) {
    // Create JSON document
    StaticJsonDocument<512> doc;
    doc["error_code"] = getErrorString(update.error);
    doc["co2_level"] = update.CO2Level;
    doc["sound_level"] = 0; // for legacy reasons now.
    doc["alarm_active"] = update.AlarmActive;
    doc["alarm_active_time"] = update.AlarmActiveTime;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    String response = makeHttpRequest("/api/device/update", jsonString);
    if (response.length() == 0) {
        logError(ErrorCode::SERVER_CONNECTION_FAILED, "Failed to send device update");
        return false;
    }
    
    ServerResponse serverResponse;
    if (!parseServerResponse(response, serverResponse)) {
        return false;
    }
    
    currentTime = serverResponse.currentTime + 3600; // Add one hour (3600 seconds) to UTC time
    
    if (parseTimeString(serverResponse.alarmTime.c_str(), hour, minute)) {
        // Update the alarm time if we have a valid alarm object (first time we don't have it.)
        if (alarm && alarm->updateTime(hour, minute)) {
            return true;
        } else if (alarm == nullptr) {
            // No alarm object yet, but time parsing succeeded
            return true;
        }
    }
    
    logError(ErrorCode::INVALID_TIME_FORMAT, "Invalid time format");
    return false;
}

bool ServerClient::parseServerResponse(const String& response, ServerResponse& serverResponse) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
        logError(ErrorCode::JSON_PARSE_ERROR, error.c_str());
        return false;
    }
    
    serverResponse.alarmTime = doc["time"].as<const char*>();
    serverResponse.alarmArmed = doc["armed"] | true;
    serverResponse.currentTime = doc["current_time"] | 0;
    
    return true;
}

bool ServerClient::parseTimeString(const char* timeStr, int& hour, int& minute) {
    // Validate input pointer
    if (!timeStr) {
        logError(ErrorCode::INVALID_TIME_FORMAT, "Time string is null");
        return false;
    }
    
    // Validate string length
    size_t len = strlen(timeStr);
    if (len < 5) {
        logError(ErrorCode::INVALID_TIME_FORMAT, "Time string too short");
        return false;
    }
    
    // Expected format: "HH:MM"
    if (strlen(timeStr) != 5 || timeStr[2] != ':') {
        logError(ErrorCode::INVALID_TIME_FORMAT, "Time string must be in HH:MM format");
        return false;
    }
    
    // Validate numeric characters
    if (!isdigit(timeStr[0]) || !isdigit(timeStr[1]) || !isdigit(timeStr[3]) || !isdigit(timeStr[4])) {
        logError(ErrorCode::INVALID_TIME_FORMAT, "Time string contains non-numeric characters");
        return false;
    }
    
    char hourStr[3] = {timeStr[0], timeStr[1], '\0'};
    char minStr[3] = {timeStr[3], timeStr[4], '\0'};
    
    hour = atoi(hourStr);
    minute = atoi(minStr);
    
    if (hour < 0 || hour >= 24 || minute < 0 || minute >= 60) {
        logError(ErrorCode::INVALID_TIME_FORMAT, "Time values out of range");
        return false;
    }
    
    return true;
}

String ServerClient::makeHttpRequest(const char* endpoint, const String& jsonBody) {
    if (!client.connect(serverHost, serverPort)) {
        logError(ErrorCode::SERVER_CONNECTION_FAILED, "Failed to connect to server");
        return "";
    }
    /* Debugging
    Serial.print("Making request to: ");
    Serial.println(endpoint);
    */
    
    // Make HTTP request
    client.print("POST ");
    client.print(endpoint);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverHost);
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonBody.length());
    client.println("Connection: close");
    client.println();
    client.println(jsonBody);
    
    /* Debugging
    Serial.print("Request body: ");
    Serial.println(jsonBody);
    */
    
    // Wait for response
    unsigned long timeout = millis();
    while (!client.available()) {
        if (millis() - timeout > 5000) {
            logError(ErrorCode::SERVER_CONNECTION_FAILED, "Request timeout");
            client.stop();
            return "";
        }
    }
    
    // Skip headers
    bool headersParsed = false;
    while (client.available() && !headersParsed) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            headersParsed = true;
        }
    }
    
    if (!headersParsed) {
        logError(ErrorCode::SERVER_CONNECTION_FAILED, "Failed to parse headers");
        return "";
    }
    
    // Read response body
    String response = client.readString();
    client.stop();
    /* Debugging
    Serial.print("Response: ");
    Serial.println(response);
    */
    return response;
} 