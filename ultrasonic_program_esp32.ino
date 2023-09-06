#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <DFRobot_ESP_PH_WITH_ADC.h>
#include <Time.h>
#include "Connection.h"
#include "Temperature.h"

// Communication Pin
#define TRIGGER_PIN 18   // Pin connected to the trigger of the ultrasonic sensor (D13) / digital
#define ECHO_PIN 5       // Pin connected to the echo of the ultrasonic sensor (D12) / digital
#define LED_PIN 19       // Pin connected to the LED (D2) / digital
#define BUZZER_PIN 16    // digital
#define SERVO_PIN 17     // digital
#define BUTTON_PIN 22    // digital

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7;
const int daylightOffset_sec = 3600;

unsigned long sendDataPrevMillis = 0;

Servo servo;

int empty_food = 0, initFood = 0;
int counter = 0;
int servoBukaSatu = 45;
int servoBukaDua = 60;

// TimeKeeper
char waktuSimpan[5];  // Define a character array to store the formatted time
int indexWaktuSatu = 0;
int indexWaktuDua = 1;

// Constants for feeding schedule
const int FEED_TIMES[] = { 900, 1200, 1500, 1700, 1900, 2100 };  // Feed times in 24-hour format
const int ALREADY_EAT[] = { 0, 0, 0 };                           //morning, afternoon, evening

// Variables for tracking feeding state
int currentFeedIndex = 0;
unsigned long lastFeedTime = 0;

void checkFood() {
  // check for third time empty food state, increment the empty_food, after empty_food value reach 3 stop the increment, send data to server, and
  // turn on the led also the buzzer
  static int emptyFoodCounter = 0;  // Static variable to keep track of empty food occurrences
  static int filledFoodCounter = 0;
  static int postEmpty = 0;
  static int postFilled = 0;
  // Servo, LED, and Buzzer
  long duration, distance_cm;

  // Generate a pulse to the ultrasonic sensor
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Measure the duration of the pulse from the sensor
  duration = pulseIn(ECHO_PIN, HIGH, 10000000L);

  // Serial.println("test");

  // Calculate the distance in centimeters
  distance_cm = duration * 0.034 / 2;

  if (distance_cm > 0 && distance_cm < 15) {
    Serial.println("Food Fill");  // Object detected within 8 cm
    digitalWrite(LED_PIN, LOW);   // Turn on the LED
    digitalWrite(BUZZER_PIN, LOW);
    if (!initFood) {
      if (!postFilled) {
        sendEventToServer("MAKANAN_TERISI");
        postFilled = 1;
      }
      initFood = 1;
    }
    if (filledFoodCounter < 10) {
      // Increment the counter and reset the counter if it reaches 10
      filledFoodCounter++;
    }
    // Check if the filledFoodCounter has reached 10
    if (filledFoodCounter >= 10) {
      if (!postFilled) {
        sendEventToServer("MAKANAN_TERISI");
        postFilled = 1;
      }
      emptyFoodCounter = 0;
      postEmpty = 0;
    }
  } else {
    Serial.println("No Food");       // No object detected or object is too far
    digitalWrite(LED_PIN, HIGH);     // Turn on the LED
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer
    if (!initFood) {
      if (!postEmpty) {
        sendEventToServer("MAKANAN_HABIS");
        postEmpty = 1;
      }
      initFood = 1;
    }
    if (emptyFoodCounter < 10) {
      // Increment the counter and reset the counter if it reaches 10
      emptyFoodCounter++;
    }
    // Check if the emptyFoodCounter has reached 10
    if (emptyFoodCounter >= 10) {
      // Perform actions when food is empty for the tenth time
      // Send data to the server (you can implement this part)
      if (!postEmpty) {
        sendEventToServer("MAKANAN_HABIS");
        postEmpty = 1;
      }
      filledFoodCounter = 0;
      postFilled = 0;
    }
  }
}

void autoFeedFish() {

  int waktu = atoi(waktuSimpan);
  if (waktu >= FEED_TIMES[indexWaktuSatu] && waktu <= FEED_TIMES[indexWaktuDua]) {
    Serial.println("Auto_Feed");
    servo.write(servoBukaSatu);
    sendEventToServer("AUTO_MAKAN_IKAN");
    if (indexWaktuSatu == 4 && indexWaktuDua == 5) {
      indexWaktuSatu = 0;
      indexWaktuDua = 1;
      Serial.println("RESET_AUTO_FEED");
      sendEventToServer("RESET_AUTO_FEED");
    }
    indexWaktuSatu+=2;
    indexWaktuDua+=2;
  }
}

void manualFeedFish() {
  // when the button is pressed, the servo will open 90 degrees and after 2 seconds it will close, if the button is over and over again
  static unsigned long lastButtonPressTime = 0;  // Static variable to track last button press time

  unsigned long currentTime = millis();

  // Check if the button is pressed and enough time has passed since the last press
  if (digitalRead(BUTTON_PIN) == 0 && currentTime - lastButtonPressTime > 1000) {
    lastButtonPressTime = currentTime;

    // Toggle servo state between open and close
    servo.write(45);  // Open the servo
    Serial.println(digitalRead(BUTTON_PIN));
    // Send data to the server (you can implement this part)
    sendEventToServer("MANUAL_MAKAN_IKAN");
  }
}

void sendEventToServer(String state) {
  httpPost(2, 0.0, 0.0, state);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Adjust hours for GMT+6
  timeinfo.tm_hour = (timeinfo.tm_hour + 6) % 24;

  // Format the time and store it in waktuSimpan
  strftime(waktuSimpan, sizeof(waktuSimpan), "%H%M", &timeinfo);

  Serial.print("Time: ");
  Serial.println(&timeinfo, "%H%M");
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);       // Set LED pin as output
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  servo.attach(SERVO_PIN);
  ph.begin();
  initWifi();

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // temperature
  DS18B20.begin();
  servo.write(0);
  delay(150);
  int waktu = atoi(waktuSimpan);
  // int waktu = 1800;
  for (int i = 0; i < 3; i++) {
    if (ALREADY_EAT[i] == 0) {
      if (waktu >= FEED_TIMES[indexWaktuSatu] && waktu <= FEED_TIMES[indexWaktuDua]) {
        Serial.println("NO_TIME_SKIP");
        break;
      } else {
        if (waktu > FEED_TIMES[indexWaktuSatu] && waktu > FEED_TIMES[indexWaktuDua]) {
          break;
        }
        Serial.print("TIME_SKIP: ");
        Serial.print(FEED_TIMES[indexWaktuSatu]);
        Serial.print(" dan ");
        Serial.println(FEED_TIMES[indexWaktuDua]);
        if (indexWaktuSatu == 4 && indexWaktuDua == 5) {
          indexWaktuSatu = 0;
          indexWaktuDua = 1;
          Serial.println("RESET_AUTO_FEED");
          sendEventToServer("RESET_AUTO_FEED");
        }
        indexWaktuSatu+=2;
        indexWaktuDua+=2;
      }
    };
  }
  sendEventToServer("PERANGKAT_BERJALAN");
}


void loop() {
  if (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0) {
    printLocalTime();
    checkFood();
    manualFeedFish();
    autoFeedFish();
    checkTempAndPhSensor();
    sendDataPrevMillis = millis();
    counter++;
    if (counter == 1) {
      servo.write(0);  // Close the servo
      ph.calibration(voltage, temperature);
      counter = 0;
    }
  }
}
