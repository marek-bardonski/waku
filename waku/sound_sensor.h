#ifndef SOUND_SENSOR_H
#define SOUND_SENSOR_H

class SoundSensor {
private:
    int pin;
    int lastVolume;
    unsigned long lastReadTime;
    const unsigned long READ_INTERVAL = 5000; // 5 seconds
    const int SAMPLE_WINDOW = 250; // Sample over 250ms
    
    // Circular buffer for storing last 5 minutes of readings
    static const int BUFFER_SIZE = 60; // 5 minutes * 12 readings per minute
    int volumeBuffer[BUFFER_SIZE];
    int bufferIndex;
    int bufferCount;
    
public:
    SoundSensor(int analogPin);
    bool begin();
    bool read();
    int getVolume() const { return lastVolume; }
    int getMaxVolume() const; // Get maximum volume over last 5 minutes
    bool isTimeToRead() const;
};

#endif // SOUND_SENSOR_H 