# WAKU Robot Documentation

## WAKU

Version: 0.3

A robot that always wakes you up on time using a scientifically-proven protocol of dawn-simulating light.

### Research Sources:
- [PubMed Study](https://pubmed.ncbi.nlm.nih.gov/20408928/)
- [Chronobiology Study](https://www.tandfonline.com/doi/abs/10.3109/07420528.2013.793196)

## Features
- Wake-up alarm with gradual light progression using multicolor LED and buzzer.
- Wi-Fi connectivity for wake-up time configuration (via home server). Home server is a separate repository.
- Button functions:
  - Disarm alarm during activation.
  - Check alarm time and CO2 levels when idle.
  - Long-press (3+ sec) disables next-day alarm.
- OLED screen for alarm time/CO2 level display.
- LED alert for CO2 levels exceeding 1000 ppm.
- CO2 level detection & reporting to the home server.

## Hardware
- **Microcontroller:** Arduino UNO R4 WiFi (Renesas RA4M1 processor)
- **Power:** USB-C for initial upload
- **Sensors:** 
  - MH-Z19B CO2 sensor (PWM output)
  - MAX9814 microphone
- **Display:** OLED 0.91"
- **Enclosure:** Wooden box (15x15x5 cm)
- **Components:** Basic Arduino starter kit (LEDs, resistors, capacitors, etc.)

## Software
- **Development Environment:** Arduino IDE 2.x (includes FreeRTOS for RA4M1)
- Uses Interrupts and OpenRTOS for interactive control.

### OpenRTOS Threads:
- **`vAlarmTask`** (10ms): Checks alarm activation, light/buzzer updates, button handling.
- **`vDisplayTask`** (50ms): Manages display updates via internal/external queues.
- **`vNetworkTask`** (15,000ms): Updates CO2 sensor data to the server.

### Interrupts:
- **CO2 Sensor (digitalPinToInterrupt)**: Tracks state changes, averages data, sends to server every `vNetworkTask` cycle.
- **Button (digitalPinToInterrupt)**:
  - Pressed during alarm → Stops alarm.
  - Pressed outside of alarm → Shows alarm time or disabled status.
  - Long press (>3 sec) → Disables next-day alarm.

### Displays:
- **Internal Display:** Shows error codes; remains blank if no error.
- **External Display:** 
  - CO2 levels (11:00-22:00)
  - Alarm time (3 sec upon button press)
  - Otherwise, remains off.

---

## Wake-Up Protocol
1. **Activation (T - 40 min):** Red light signals protocol start and slowly ramps up to 10% intensity
2. **Light Ramp-Up (T - 30 min to Wake-Up):** 
   - 10% dawn light starts, gradually reaching 100% at wake-up.
3. **Full Alarm (Wake-Up Time +10 min):**
   - Buzzer plays a melody at 120 bpm.
   - Light starts flashing.

```python
# Define LED intensity ramping logic
red = np.where(progress < 0.5, progress * 2 * 255, 255)
green = np.clip((progress - 0.4) / 0.6, 0, 1) * 180
blue = np.clip((progress - 0.6) / 0.4, 0, 1) * 120
```

## Assembling the Kit

A wooden box (15x15x5 cm) is used as the base. The pin connections are as follows:

### **Arduino Pin Connections**
- **5V** → Breadboard 5V bus
- **GND** → Breadboard GND bus
- **PIN 2 (ADC)** → MH-Z19B CO2 sensor PWM output
- **PIN 3 (DIGITAL)** → Button → GND (Requires PULL-UP)
  - **Note:** Setting this pin as output may destroy the chip unless connected via a resistor.
- **PIN 9 (PWM)** → Resistor → LED #1 → 2N2222 base
- **PIN 10 (PWM)** → Resistor → LED #2 → 2N2222 base
- **PIN 11 (PWM)** → Resistor → LED #3 → 2N2222 base
- **PIN 12 (PWM)** → Buzzer -> 330 OHM -> GND
- **PIN 6 (PWM)** → Buzzer -> GND
- **PIN A0 (ADC)** → MAX9814 microphone out
- **SDA/SCL** → OLED 0.91"

---

## Required Libraries

### Installing Libraries via Arduino IDE
1. Open **Arduino IDE**
2. Go to **Tools → Manage Libraries**
3. Search for and install:
   - `ArduinoGraphics`
   - `WiFiS3`
   - `Arduino_LED_Matrix`
   - `NTPClient`
   - `ArduinoOTA`
   - `ArduinoJSON`

> **Note:** FreeRTOS support is built into the Arduino IDE for the UNO R4 WiFi board.


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

## TODO

* [ ] Detecting missed wake-up alarms and triggering gradual light and sound effects.
* [ ] Using movement sensors to detect movement in specific areas indicating a missed alarm.
* [ ] Add sounds system that can replace buzzer to provide better quality of wake-up sound.
* [ ] Implement stronger LED to better simulate dawn in the room eg. https://sklep.avt.pl/pl/products/dioda-led-f5-biala-60000mcd-172143.html?rec=101002105 + https://sklep.avt.pl/pl/products/oprawka-do-diod-led-5mm-metalowa-wypukla-typ2-180856.html 




