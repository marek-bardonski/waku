#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>
#include <RTC.h>
#include "progressive_alarm.h"

class Alarm {
private:
    int WAKE_HOUR;
    int WAKE_MINUTE;
    const int WAKE_DURATION;
    bool alarmTriggeredToday;
    
    // Progressive alarm handler
    ProgressiveAlarm progressiveAlarm;
    
    // Wake-up protocol timing (in minutes)
    static const int PRE_WAKE_TIME = 40;    // Start red light 40 min before
    static const int DAWN_START_TIME = 30;  // Start dawn simulation 30 min before
    static const int FULL_ALARM_TIME = 10;  // Full alarm 10 min after wake time
    
    // Helper methods for time comparison
    int timeToMinutes(int hours, int minutes) const {
        return hours * 60 + minutes;
    }
    
    int getWakeUpStartMinutes() const {
        return timeToMinutes(WAKE_HOUR, WAKE_MINUTE) - PRE_WAKE_TIME;
    }
    
    int getWakeUpEndMinutes() const {
        return timeToMinutes(WAKE_HOUR, WAKE_MINUTE) + FULL_ALARM_TIME;
    }
    
    int getCurrentTimeMinutes() const {
        RTCTime currentTime;
        RTC.getTime(currentTime);
        return timeToMinutes(currentTime.getHour(), currentTime.getMinutes());
    }
    
    float calculateProgress() const {
        int currentMinutes = getCurrentTimeMinutes();
        int wakeUpStart = getWakeUpStartMinutes();
        int wakeUpTime = timeToMinutes(WAKE_HOUR, WAKE_MINUTE);
        int minutesElapsed = currentMinutes - wakeUpStart;
        
        // Handle case when we're after midnight
        if (currentMinutes < wakeUpStart) {
            minutesElapsed = (24 * 60 - wakeUpStart) + currentMinutes;
        }
        
        // Calculate progress based on wake-up protocol phases
        if (currentMinutes < (wakeUpTime - DAWN_START_TIME)) {
            // Pre-wake phase (red light only)
            return 0.0;
        } else if (currentMinutes < wakeUpTime) {
            // Dawn simulation phase
            return float(currentMinutes - (wakeUpTime - DAWN_START_TIME)) / float(DAWN_START_TIME);
        } else if (currentMinutes <= (wakeUpTime + FULL_ALARM_TIME)) {
            // Full alarm phase
            return 1.0;
        }
        
        return 0.0;
    }

public:
    Alarm(int wakeHour, int wakeMinute, int wakeDuration,
          const int* ledPins, int ledPinCount,
          int buzzerPin, int buzzerOverdrivePin);
    
    bool isWakeUpTime();
    void checkAndResetAtMidnight();
    void stopAlarm();
    void update();
    bool isTriggered() const { return alarmTriggeredToday; }
    
    bool updateTime(int newHour, int newMinute);
    int getWakeHour() const { return WAKE_HOUR; }
    int getWakeMinute() const { return WAKE_MINUTE; }
};

#endif 