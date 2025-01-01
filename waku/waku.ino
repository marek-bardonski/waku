/*

 This example connects to an WPA encrypted WiFi network.
 Then it prints the  MAC address of the Wifi shield,
 the IP address obtained, and other network details.
 It then polls for sketch updates over WiFi, sketches
 can be updated by selecting a network port from within
 the Arduino IDE: Tools -> Port -> Network Ports ...

 Circuit:
 * WiFi shield attached

 created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe
 modified 16 January 2017
 by Sandeep Mistry
 */
 
#include <SPI.h>
#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
ArduinoLEDMatrix matrix;
#include "animation.h"

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;      // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password

int status = WL_IDLE_STATUS;

void connectToWiFi() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }

  // start the WiFi OTA library with internal (flash) based storage
  ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
}

void setup() {
  //Initialize serial:
  //Serial.begin(9600);
  
  connectToWiFi();

  matrix.begin();

  //matrix.textFont(Font_4x6);


   // add the text
  const char text[] = "    I LOVE MAREK !!!!    ";
  matrix.stroke(0xFFFFFFFF);
  matrix.textScrollSpeed(50);
  matrix.textFont(Font_5x7);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(text);
  matrix.endText(SCROLL_LEFT);
  matrix.endDraw();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);

}

void loop() {
  // check for WiFi OTA updates
  ArduinoOTA.poll();

  //blinks the built-in LED every second
  //digitalWrite(LED_BUILTIN, HIGH);
 // delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);


  analogWrite(9, 255);
  analogWrite(10, 255);
  analogWrite(11, 255);
}