// Make sure you install the ESP8266 Board Package via the Arduino IDE Board Manager and select the correct ESP8266 board before compiling. 

//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP8266.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <TH02_dev.h>
#include "Arduino.h"

#include <EEPROM.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

// WiFi network info.
char ssid[] = "Liteart";            //Add your home wifi network ssid 
char wifiPassword[] = "Aumate@11231";   //Add wifi password

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "9ccd05e0-6584-11e6-8378-e1a125c94566";       //Add your username
char password[] = "98b38f9c1258d6c03c6c53fd7878be59ddc07586";   //Add your password
char clientID[] = "1ccd4569-1212-11e6-b0e9-e9adcff7878e";       //Add your clientID

unsigned long lastMillis = 0;

//GPIO Connections
const int trigPin = 12;
const int echoPin = 13;

const int relayPin1 = 5;  //connected to motor (water pump)
const int relayPin2 = 4;  //Connected to led grow light

//Variables for LDR light level data storage
int lightLevel = 0;
int ldr_addr = 0;

//Variables for soil moisture level data storage
int moistureLevel = 0;
int moisture_addr = 20;

//Variables for water level data storage
int waterLevelcm = 0;
int waterContainerHeight = 90;  //in cm water container's total height
int waterLevelmin = 10;
int motorONTime = 1000;

//Variables for Temperature and Humidity data storage
float temperature = 0;
float humidity = 0;

//WiFi Variables
long wifiRSSI = 0;

 //Publish data every 30 seconds (30000 milliseconds). Change this value to publish at a different interval.
int uploadInterval = 30000; //in milliseconds

int eepromReadInt(int address){
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}
 
void eepromWriteInt(int address, int value){
   EEPROM.write(address, (value >> 8) & 0xFF );
   EEPROM.write(address+1, value & 0xFF);
   EEPROM.commit();
}

void setup() {
  //configure GPIO pins as input/output
  pinMode(relayPin1, OUTPUT);     // Initialize the Relay1 pin as an output
  pinMode(relayPin2, OUTPUT);     // Initialize the Relay2 pin as an output
  digitalWrite(relayPin1, HIGH);    // Turn the Relay1 OFF (Water Pump)
  digitalWrite(relayPin2, HIGH);    // Turn the Relay2 OFF (LED Grow Light)
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  EEPROM.begin(512); // Initlize EEPROM with 512 Bytes
  Serial.begin(9600);

  Serial.println("");
  Serial.println("");
  
  Serial.println("Smart Mali Project!");
    
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV
  
  ads.begin();
  
  /* Power up,delay 150ms,until voltage is stable */
  delay(150);
  /* Reset HP20x_dev */
  TH02.begin();
  delay(100);
  
  /* Determine TH02_dev is available or not */
  Serial.println("TH02_dev is available.\n"); 
  
  delay(1000);                      // Wait for a second
  
  Cayenne.begin(username, password, clientID, ssid, wifiPassword);
}

void loop() {
  Cayenne.loop(); 
   
   if (millis() - lastMillis > uploadInterval) {
    lastMillis = millis();

    long val = 0;
    for (int number = 0; number < 10; number++) 
    {
        lightLevel = ads.readADC_SingleEnded(0);
        val = val + lightLevel;
        delay(100);
    }
    //Serial.print("Total ADC0: ");  Serial.println(val);
    lightLevel = val/10;
    //Serial.print("AIN0: ");   Serial.print(lightLevel);
    //float voltage0 = (lightLevel * 0.125)/1000;
    //Serial.print("\tVoltage0: ");  Serial.println(voltage0, 6);

    val = 0;
    for (int number = 0; number < 10; number++) 
    {
      moistureLevel = ads.readADC_SingleEnded(3);  
      val = val + moistureLevel;
      delay(100);
    }
    //Serial.print("Total ADC1: ");  Serial.println(val);
    
    moistureLevel = val/10;
    //Serial.print("AIN3: "); Serial.print(moistureLevel);
    //float voltage3 = (moistureLevel * 0.125)/1000;
    //Serial.print("\tVoltage3: ");  Serial.println(voltage3, 6);  
   
    Serial.print("Light Level: ");   
    Serial.print(lightLevel);
    Serial.print("\t Moisture Level: ");
    Serial.println(moistureLevel);

    // The ultrasonic sensor is triggered by a HIGH pulse of 10 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW); 
    // Read the signal from the sensor: a HIGH pulse whose
    // duration is the time (in microseconds) from the sending
    // of the ping to the reception of its echo off of an object.
    long duration = pulseIn(echoPin, HIGH);
    waterLevelcm = waterContainerHeight - int(microsecondsToCentimeters(duration));
    Serial.print("Water Level: ");
    Serial.println(waterLevelcm);

    temperature = TH02.ReadTemperature(); 
    humidity = TH02.ReadHumidity();  
    Serial.print("Temperature: ");   
    Serial.print(temperature);
    Serial.print("\t Humidity: ");
    Serial.println(humidity);

    //the received wifi signal strength:
    wifiRSSI = WiFi.RSSI(); 
    Serial.print("RSSI:");
    Serial.println(wifiRSSI);
    Serial.println("");

    int data = eepromReadInt(ldr_addr);
    Serial.print("LDR Data EEPROM Read: ");   Serial.println(data);
    if (lightLevel < data)
    {
      digitalWrite(relayPin2, LOW);    // Turn the Relay2 ON
      Serial.println("LED GROW LIGHT is turned ON");
      Cayenne.virtualWrite(12, 21);
    }
    else
    {
      digitalWrite(relayPin2, HIGH);    // Turn the Relay2 OFF
      Serial.println("LED GROW LIGHT is turned OFF");
      Cayenne.virtualWrite(12, 20);
    }
    
    data = eepromReadInt(moisture_addr);
    Serial.print("Moisture Data EEPROM Read: ");   Serial.println(data);
    if((moistureLevel < data) && (waterLevelcm > waterLevelmin))
    {
      digitalWrite(relayPin1, LOW);    // Turn the Relay1 ON
      Serial.println("Motor (Water Pump) is turned ON");
      
      delay(motorONTime);
      
      digitalWrite(relayPin1, HIGH);    // Turn the Relay1 OFF
      Serial.println("Motor (Water Pump) is turned OFF");
    }
    else
    {
      digitalWrite(relayPin1, HIGH);    // Turn the Relay1 OFF
      Serial.println("Motor (Water Pump) is turned OFF");
    }
       
    //Write data to Cayenne here. This example just sends the current uptime in milliseconds.
    Cayenne.virtualWrite(1, lastMillis);
    Cayenne.luxWrite(2, lightLevel);
    Cayenne.virtualWrite(3, moistureLevel);
    Cayenne.virtualWrite(4, waterLevelcm);
    Cayenne.celsiusWrite(5, temperature);
    Cayenne.virtualWrite(6, humidity);
    Cayenne.virtualWrite(7, wifiRSSI);
    Cayenne.virtualWrite(12, 0);
    
    Serial.println(" ");
    Serial.println(" ------------------------------------------------------------------------ ");
  }
  
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

CAYENNE_IN(8)
{
  CAYENNE_LOG("CAYENNE_IN_DEFAULT(%u) - %s, %s", request.channel, getValue.getId(), getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
  Serial.print("Data Received:-= ");
  Serial.println(getValue.asInt());
  int data = getValue.asInt();
  
  // The ultrasonic sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW); 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  long duration = pulseIn(echoPin, HIGH);
  waterLevelcm = waterContainerHeight - int(microsecondsToCentimeters(duration));
  Serial.print("Water Level: ");
  Serial.println(waterLevelcm);
  
  if ((data == 1) && (waterLevelcm > waterLevelmin))
  {
      digitalWrite(relayPin1, LOW);    // Turn the Relay1 ON
      Serial.println("Motor (Water Pump) is turned ON");
      Cayenne.virtualWrite(12, 11);
  }
  else
  {
      digitalWrite(relayPin1, HIGH);    // Turn the Relay1 OFF
      Serial.println("Motor (Water Pump) is turned OFF");
      Cayenne.virtualWrite(12, 10);
  }
}

CAYENNE_IN(9)
{
  CAYENNE_LOG("CAYENNE_IN_DEFAULT(%u) - %s, %s", request.channel, getValue.getId(), getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
  Serial.print("Data Received:-= ");
  Serial.println(getValue.asInt());
  int data = getValue.asInt();
  if (data == 1)
  {
      digitalWrite(relayPin2, LOW);    // Turn the Relay2 ON
      Serial.println("LED GROW LIGHT is turned ON");
      Cayenne.virtualWrite(12, 21);
  }
  else
  {
      digitalWrite(relayPin2, HIGH);    // Turn the Relay2 OFF
      Serial.println("LED GROW LIGHT is turned OFF");
      Cayenne.virtualWrite(12, 20);
  }
}

CAYENNE_IN(10)
{
  CAYENNE_LOG("CAYENNE_IN_DEFAULT(%u) - %s, %s", request.channel, getValue.getId(), getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
  Serial.print("Data Received:-= ");
  Serial.println(getValue.asInt());
  int data = 0;
  data = getValue.asInt();
  if (data == 1)
  {
      Serial.println("Starting the LDR sampling");
      Cayenne.virtualWrite(12, 31);
      
      long val = 0;
      for (int number = 0; number < 10; number++) 
      {
        lightLevel = ads.readADC_SingleEnded(0);
        val = val + lightLevel;
        delay(100);
      }
      
      //Serial.print("Total ADC0: ");  Serial.println(val);
      
      lightLevel = val/10;
      //Serial.print("AIN0: ");   Serial.print(lightLevel);
      //float voltage0 = (lightLevel * 0.125)/1000;
      //Serial.print("\tVoltage0: ");  Serial.println(voltage0, 2);
      
      // write the value to the appropriate byte of the EEPROM.
      // these values will remain there when the board is
      // turned off.
      eepromWriteInt(ldr_addr, lightLevel);     
  }
}

CAYENNE_IN(11)
{
  CAYENNE_LOG("CAYENNE_IN_DEFAULT(%u) - %s, %s", request.channel, getValue.getId(), getValue.asString());
  //Process message here. If there is an error set an error message using getValue.setError(), e.g getValue.setError("Error message");
  Serial.print("Data Received:-= ");
  Serial.println(getValue.asInt());
  int data = 0;
  data = getValue.asInt();
  if (data == 1)
  {
      Serial.println("Starting the Soil Moisture sampling");
      Cayenne.virtualWrite(12, 41);

      long val = 0;
      for (int number = 0; number < 10; number++) 
      {
        moistureLevel = ads.readADC_SingleEnded(3);  
        val = val + moistureLevel;
        delay(100);
      }
      //Serial.print("Total ADC1: ");  Serial.println(val);
      
      moistureLevel = val/10;
      //Serial.print("AIN3: "); Serial.print(moistureLevel);
      //float voltage3 = (moistureLevel * 0.125)/1000;
      //Serial.print("\tVoltage3: ");  Serial.println(voltage3, 6);
      eepromWriteInt(moisture_addr, moistureLevel);
  }
}
