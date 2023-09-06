#include "arduino_secrets.h"
#define WIFI_LED_PIN 21  // digital

const char* ssid = secret_ssid;
const char* password = secret_password;

void initWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println('.');
    digitalWrite(WIFI_LED_PIN, LOW);
    delay(250);
    digitalWrite(WIFI_LED_PIN, HIGH);
  }

  digitalWrite(WIFI_LED_PIN, HIGH);
  Serial.println(WiFi.localIP());
}