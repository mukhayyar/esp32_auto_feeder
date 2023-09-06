#include "ApiComm.h"

#define TEMP_PIN 23      // digital
#define PH_PIN 34        // adc
#define ESPADC 4096.0    //the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300  //the esp voltage supply value

OneWire oneWire(TEMP_PIN);
DallasTemperature DS18B20(&oneWire);
float voltage, phValue, temperature = 25;
DFRobot_ESP_PH_WITH_ADC ph;
int lastSE = 0;

void tempSensor() {
  DS18B20.requestTemperatures();
  temperature = DS18B20.getTempCByIndex(0);
  Serial.print(temperature);
  Serial.println("ºC");
}

void phSensor() {
  voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE;  // read the voltage
  phValue = ph.readPH(voltage, temperature);           // convert voltage to pH with temperature compensation
  Serial.print("pH:");
  Serial.println(phValue, 4);
}

void sendSensorToServer(float ph, float temp) {
  // every 2 minutes send ph and temp data to the server
  // Make a POST request to https://hifish.serv00.net/model/store/post_sensors.php
  httpPost(1, ph, temp, "");
}

void checkTempAndPhSensor() {
  tempSensor();
  phSensor();
  if (millis() - (1 * 60 * 1000UL) > lastSE) {
    lastSE = millis();
    sendSensorToServer(phValue, temperature);
  }
}