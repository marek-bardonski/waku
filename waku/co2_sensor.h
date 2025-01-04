#ifndef CO2_SENSOR_H
#define CO2_SENSOR_H

class CO2Sensor {
private:
    const int pwmPin;
    float lastTemperature;
    int lastCO2;
    unsigned long lastReadTime;
    const unsigned long READ_INTERVAL = 5000; // 5 seconds
    const unsigned long PWM_CYCLE = 1004; // PWM cycle of 1004ms for MH-Z19B
    const int MAX_PPM = 5000; // Maximum CO2 PPM reading

    // Circular buffer for storing last 5 minutes of readings
    static const int BUFFER_SIZE = 60; // 5 minutes * 12 readings per minute
    int co2Buffer[BUFFER_SIZE];
    int bufferIndex;
    int bufferCount;

    int readPWM();

public:
    CO2Sensor(int pwmPin);
    
    bool begin();
    bool read();
    float getTemperature() const { return lastTemperature; }
    int getCO2Level() const { return lastCO2; }
    float getAverageCO2() const; // Get average CO2 over last 5 minutes
    bool isTimeToRead() const;
};

#endif // CO2_SENSOR_H 