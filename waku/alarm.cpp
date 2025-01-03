#include "alarm.h"

Alarm::Alarm(int wakeHour, int wakeMinute, int wakeDuration,
             const int* ledPins, int ledPinCount,
             int buzzerPin,
             const int* frequencies, const int* durations, int freqSteps,
             int initialInterval, int finalInterval)
    : WAKE_HOUR(wakeHour)
    , WAKE_MINUTE(wakeMinute)
    , WAKE_DURATION(wakeDuration)
    , BEEP_FREQUENCIES(frequencies)
    , BEEP_DURATIONS(durations)
    , FREQ_STEPS(freqSteps)
    , alarmTriggeredToday(false)
    , progressiveAlarm(ledPins[1], ledPins[2], ledPins[0], buzzerPin)  // Red=10, Green=11, Blue=9
{
    // Constructor body is empty as initialization is done in initializer list
}

bool Alarm::isWakeUpTime() {
    if (alarmTriggeredToday) {
        return false;
    }
    
    int currentMinutes = getCurrentTimeMinutes();
    int wakeUpStart = getWakeUpStartMinutes();
    int wakeUpEnd = getWakeUpEndMinutes();
    
    // Handle case when wake-up time spans across midnight
    if (wakeUpEnd >= 24 * 60) {
        wakeUpEnd %= (24 * 60);  // Normalize to 24-hour format
        // If current time is either after start or before end (spanning midnight)
        return currentMinutes >= wakeUpStart || currentMinutes < wakeUpEnd;
    } else {
        // Normal case within same day
        return currentMinutes >= wakeUpStart && currentMinutes < wakeUpEnd;
    }
}

void Alarm::checkAndResetAtMidnight() {
    RTCTime currentTime;
    RTC.getTime(currentTime);
    
    if (currentTime.getHour() == 0 && currentTime.getMinutes() == 0) {
        if (alarmTriggeredToday) {
            alarmTriggeredToday = false;
            Serial.println("Midnight reached - Reset alarm trigger status for new day");
        }
    }
}

void Alarm::stopAlarm() {
    alarmTriggeredToday = true;
    progressiveAlarm.stop();
    Serial.println("ALARM STOPPED!");
}

void Alarm::update() {
    if (isWakeUpTime() && !alarmTriggeredToday) {
        float progress = calculateProgress();
        progressiveAlarm.update(progress, BEEP_FREQUENCIES, BEEP_DURATIONS, FREQ_STEPS);
    } else {
        progressiveAlarm.stop();
    }
}

bool Alarm::updateTime(int newHour, int newMinute) {
    if (newHour < 0 || newHour >= 24 || newMinute < 0 || newMinute >= 60) {
        return false;
    }
    
    WAKE_HOUR = newHour;
    WAKE_MINUTE = newMinute;
    return true;
} 