# WAKU - an Arduino robot that always wakes you up on time. 
## It detects missed wake-up alarms and wakes you up with gradually increasing light effects and sounds. 

## Features
* Version 0.1 - can be only programmed via API to wake-up at a specific time.
* Version 0.2 [in-progress] - it is going to use a microphone to detect missed alarms automatically.

## Prerequisites

- Arduino UNO R4 WiFi board
- Arduino IDE 2.x
- USB-C cable to initiate the first upload
- WiFi network access
- An 'Arduino' starting set with small electronic components (LEDs, resistors, capacitors, etc.)

## Assembling the kit
I've used a 15x15x5 cm wooden box as a base for the robot. The pins are connected in the following way:

### Arduino

* 5V -> breadboard 5V bus
* GND -> breadboard GND bus
* PIN 9 (PWM) - Resistor [LED #1] -> 2N2222 base [LED #1]
* PIN 10 (PWM)- Resistor [LED #2] -> 2N2222 base [LED #2]
* PIN 11 (PWM)- Resistor [LED #3] -> 2N2222 base [LED #3]

**TBC**

## Required Libraries

### Installing Libraries via Arduino IDE
1. Open Arduino IDE
2. Go to Tools → Manage Libraries
3. Search and install:
   - `ArduinoGraphics`
   - `WiFiS3`
   - `Arduino_LED_Matrix`
   - `NTPClient`
   - `ArduinoOTA`
   - `ArduinoJSON`

### Installing ArduinoOTA with Extended Timeout
To enable easy updates, this project is equipped with an OTA system. Unfortunatelly, the OTA library is known to be problematic and doesn't work out of the box on any of my computers. 

Unfortunately it doesn't work out of the box for me. The lessons I've learned are:
1. The ArduinoOTA library doesn't work on Macbook Silicon processors because there is no dedicated release version for them and by default a version for Intel processors is used even when installed on a Macbook Silicon - that cause a hard to diagnose segmentation fault error. I coldn't get this version to work using Rosetta 2. 

2. You can use Arduino IDE and Arduino OTA library under Virtual Machine (eg. VMWare), both for wired flashing [to be confirmed] and wireless OTA under MacBook. I suggest to use proper electical isolation or a backup device as there is (relatively low) risk of short circuiting the USB-C port.

### Exact steps that worked for me

1. Check if you have ArduinoOTA library installed. If not, install it from the Arduino IDE. 
2. Download the latest ArduinoOTA library release (binary) from [GitHub](https://github.com/arduino/arduinoOTA/releases). The version used for this project was 1.4.1
3. Replace the old version distributed by Arduino IDE library [**arduinoOTA** file] with the one downloaded in the previous step that supports the `-t` flag. It is located in the 
```
%LOCALAPPDATA%\Arduino15\packages\arduino\tools\arduinoOTA\<version>\bin\
```


## Platform Configuration

By default, Arduino IDE is not able to communicate proplerly with Arduino UNO R4 WiFi board via Wi-Fi. To enable this communication, you need to modify the configuration of the board by copying the platform.local.txt file from the [GitHub](https://github.com/JAndrassy/ArduinoOTA/tree/master/extras/renesas) repository to the Arduino Uno R4 board configuration directory, next to the platform.txt file 
```
%LOCALAPPDATA%\Arduino15\packages\arduino\hardware\renesas_uno\[version]
```

After adding this file, modify all 4 "patterns" by adding the `-t 60` flag to the end of each pattern, eg. 
```
tools.arduino_ota.upload.pattern="{cmd}" -address "{upload.port.address}" -port 65280 -username arduino -password "{upload.field.password}" -sketch "{build.path}/{build.project_name}.bin" -upload /sketch -b -t 60
```


## Project Setup

1. Modify `arduino_secrets.h` with your WiFi credentials:
   ```cpp
   #define SECRET_SSID "your_wifi_ssid"
   #define SECRET_PASS "your_wifi_password"
   ```

## Features

- Scrolling text display on LED matrix
- WiFi connectivity with OTA update support
- LED control on pins 9, 10, and 11
- Built-in LED status indication

## OTA Updates

1. Upload the initial sketch via USB
2. For subsequent updates:
   - Select Tools → Port → Network Ports
   - Choose your Arduino from the network ports list
   - Upload as normal

## Troubleshooting

- If OTA upload fails, ensure:
  - Arduino is powered and connected to WiFi
  - Computer and Arduino are on the same network
  - Firewall isn't blocking the connection
  - 60-second timeout is properly configured


## Contributing

Feel free to submit issues and pull requests.

## License

This project is open-source and available under the MIT License.

# TODO:
- [ ] Notify the owner of ArduinoOTA of the problems in MacBook Silicon
- [ ] Check if you can use ArduinoOTA via VMWare to flash the Arduino via cable


