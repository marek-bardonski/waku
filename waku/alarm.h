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
    
    // Buzzer parameters
    const int* BEEP_FREQUENCIES;
    const int* BEEP_DURATIONS;
    const int FREQ_STEPS;
    
    // Progressive alarm handler
    ProgressiveAlarm progressiveAlarm;
    
    // Helper methods for time comparison
    int timeToMinutes(int hours, int minutes) const {
        return hours * 60 + minutes;
    }
    
    int getWakeUpStartMinutes() const {
        return timeToMinutes(WAKE_HOUR, WAKE_MINUTE);
    }
    
    int getWakeUpEndMinutes() const {
        return timeToMinutes(WAKE_HOUR, WAKE_MINUTE) + WAKE_DURATION;
    }
    
    int getCurrentTimeMinutes() const {
        RTCTime currentTime;
        RTC.getTime(currentTime);
        return timeToMinutes(currentTime.getHour(), currentTime.getMinutes());
    }
    
    float calculateProgress() const {
        int currentMinutes = getCurrentTimeMinutes();
        int wakeUpStart = getWakeUpStartMinutes();
        int minutesElapsed = currentMinutes - wakeUpStart;
        
        // Handle case when we're after midnight
        if (currentMinutes < wakeUpStart) {
            minutesElapsed = (24 * 60 - wakeUpStart) + currentMinutes;
        }
        
        return float(minutesElapsed) / float(WAKE_DURATION);
    }

public:
    Alarm(int wakeHour, int wakeMinute, int wakeDuration,
          const int* ledPins, int ledPinCount,
          int buzzerPin,
          const int* frequencies, const int* durations, int freqSteps,
          int initialInterval, int finalInterval);
    
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