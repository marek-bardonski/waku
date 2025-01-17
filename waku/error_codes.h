#ifndef ERROR_CODES_H
#define ERROR_CODES_H

enum class ErrorCode {
    NO_ERROR = 0,
    RTC_INIT_FAILED = 1,
    RTC_SET_TIME_FAILED = 2,
    RTC_GET_TIME_FAILED = 3,
    RTC_ALARM_SET_FAILED = 4,
    NTP_SYNC_FAILED = 5,
    WIFI_CONNECTION_FAILED = 6,
    SYSTEM_RECOVERY_FAILED = 7,
    CO2_SENSOR_INIT_FAILED = 8,
    SOUND_SENSOR_INIT_FAILED = 9,
    SERVER_CONNECTION_FAILED = 10,
    JSON_PARSE_ERROR = 11,
    INVALID_TIME_FORMAT = 12,
    SENSOR_READ_ERROR = 13,
    TASK_INIT_FAILED = 14
};

// Convert enum to string for display and logging
inline const char* getErrorString(ErrorCode code) {
    switch (code) {
        case ErrorCode::NO_ERROR: return "NO_ERROR";
        case ErrorCode::RTC_INIT_FAILED: return "RTC_INIT_FAILED";
        case ErrorCode::RTC_SET_TIME_FAILED: return "RTC_SET_TIME_FAILED";
        case ErrorCode::RTC_GET_TIME_FAILED: return "RTC_GET_TIME_FAILED";
        case ErrorCode::RTC_ALARM_SET_FAILED: return "RTC_ALARM_SET_FAILED";
        case ErrorCode::NTP_SYNC_FAILED: return "NTP_SYNC_FAILED";
        case ErrorCode::WIFI_CONNECTION_FAILED: return "WIFI_CONNECTION_FAILED";
        case ErrorCode::SYSTEM_RECOVERY_FAILED: return "SYSTEM_RECOVERY_FAILED";
        case ErrorCode::CO2_SENSOR_INIT_FAILED: return "CO2_SENSOR_INIT_FAILED";
        case ErrorCode::SOUND_SENSOR_INIT_FAILED: return "SOUND_SENSOR_INIT_FAILED";
        case ErrorCode::SERVER_CONNECTION_FAILED: return "SERVER_CONNECTION_FAILED";
        case ErrorCode::JSON_PARSE_ERROR: return "JSON_PARSE_ERROR";
        case ErrorCode::INVALID_TIME_FORMAT: return "INVALID_TIME_FORMAT";
        case ErrorCode::SENSOR_READ_ERROR: return "SENSOR_READ_ERROR";
        case ErrorCode::TASK_INIT_FAILED: return "TASK_INIT_FAILED";
        default: return "UNKNOWN_ERROR";
    }
}

#endif // ERROR_CODES_H 