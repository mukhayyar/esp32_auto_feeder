#include <HTTPClient.h>

void httpPost(int type, float ph, float temp, String state) {
  HTTPClient http;
  int httpResponseCode;
  // type 1 is for sensor log and type 2 is for event log
  String tempStr = String(temp);
  String phStr = String(ph);
  String typeHeader = "Content-Type";
  String typeHeaderValue = "application/x-www-form-urlencoded";
  if (type == 1) {
    http.begin("https://hifish.serv00.net/model/store/post_sensors.php");
    http.addHeader(typeHeader, typeHeaderValue);

    // Replace these values with your actual data
    String postData = "password=hifish12345&temp=" + tempStr + "&ph=" + phStr;  // Replace XYZ with your sensor value
    httpResponseCode = http.POST(postData);
  } else if (type == 2) {
    http.begin("https://hifish.serv00.net/model/store/post_state_event.php");
    http.addHeader(typeHeader, typeHeaderValue);

    // Replace this value with your actual data
    String postData = "password=hifish12345&state=" + state;  // Replace EVENT_STATE with your event state
    int httpEventResponseCode = http.POST(postData);
  }

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response: " + response);
  } else {
    Serial.println("HTTP POST request failed");
  }
  http.end();
}