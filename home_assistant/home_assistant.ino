// Wifi stuff
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

// Secrets
const char *Ssid = "VirginMedia";
const char *Password = "password-wifi";
const String baseUrl = "http://192.168.1.104:8123/api/states";
const String token = "home.assistant.token";

// Leds and buttons
const int resetParcelStateButton = D5;
const int syncStates = D6;

const int garage_on = D8;
const int alarm_on = D10;
const int parcel_on = D11;
// Variable to wait during the loop without blocking
unsigned long oldMillis = 0;
const int DelayBetweenRequests = 30000; // 30 seconds

// Status of the old sync and parcel box buttons
int oldStatus = 1;
int oldSyncButtonStatus = 1;

unsigned long currentMillis = 0;

int allLeds[3] = {garage_on, alarm_on, parcel_on};
int parcelLed[1] = {parcel_on};

// If set to True the program will enter a panic mode
bool fatalError = false;

void setup()
{
  Serial.begin(9600);

  // Initialize the LED_BUILTIN pin as an output and turn them OFF
  pinMode(alarm_on, OUTPUT);
  pinMode(garage_on, OUTPUT);
  pinMode(parcel_on, OUTPUT);
  digitalWrite(alarm_on, HIGH);
  digitalWrite(garage_on, HIGH);
  digitalWrite(parcel_on, HIGH);

  pinMode(resetParcelStateButton, INPUT_PULLUP);
  pinMode(syncStates, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(Ssid, Password);

  Serial.println("--------------------");
  Serial.println("Ready");

  connectToWifi();

  syncAllStatuses();
}

void loop()
{
  // wait for WiFi connection, ignore if already connected

  if (WiFiMulti.run() == WL_CONNECTED)
  {
    currentMillis = millis();

    if (requestSyncState() || ((unsigned long)(currentMillis - oldMillis) >= DelayBetweenRequests))
    {
      syncAllStatuses();
      // Blink the led to show the value was successfully updated
      blinkLed(allLeds);
      oldMillis = millis();
    }

    // Listen for the button that resets the parcel state
    handleResetParcelState();

    // If there is an error, let's blink the led indefinitely until manual reset
    while (fatalError)
    {
      blinkLed(allLeds);
    }
  }
}

/**
 * Sync all the statuses with HA
 */
void syncAllStatuses()
{
  sendRequest("/sensor.verisure_house", "Off", alarm_on);
  sendRequest("/sensor.verisure_garage", "Off", garage_on);
  sendRequest("/input_boolean.parcel_box", "Empty", parcel_on);
  Serial.println("Statuses synched");
}

bool requestSyncState()
{
  int syncStatesButtonState = digitalRead(syncStates);
  if (syncStatesButtonState == LOW && oldSyncButtonStatus == HIGH)
  {
    Serial.println("Sync state button pressed");

    oldSyncButtonStatus = syncStatesButtonState;
    return true;
  }
  oldSyncButtonStatus = syncStatesButtonState;
  return false;
}

/**
   Handle the reset of the parcel box status and led
*/
void handleResetParcelState()
{
  int buttonState = digitalRead(resetParcelStateButton);
  if (buttonState == LOW && oldStatus == HIGH)
  {
    Serial.println("Reset parcel button pressed");
    // Give the user a feedback
    blinkLed(parcelLed);

    WiFiClient client;
    HTTPClient http;

    if (http.begin(client, baseUrl + "/input_boolean.parcel_box"))
    {
      http.addHeader("Authorization", "Bearer " + token);
      http.addHeader("Content-Type", "application/json");

      // start connection and send HTTP header
      int httpCode = http.POST("{\"state\": \"Empty\"}");
      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        // Serial.printf("[HTTP] Response: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          // Refresh the status of the parcel box
          sendRequest("/input_boolean.parcel_box", "Empty", parcel_on);
        }
        else
        {
          fatalError = true;
        }
      }
      else
      {
        Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
        fatalError = true;
      }
      http.end();
    }
    else
    {
      Serial.printf("[HTTP} Unable to connect\n");
      fatalError = true;
    }
  }
  oldStatus = buttonState;
}

/**
   Blink the leds in the passed arary of leds
*/
void blinkLed(int leds[])
{
  int x, i = 0;
  // Note: we MUST use an even number of blinks, so we keep
  // the original status of the LED
  for (x = 0; x < 6; x++)
  {
    for (i = 0; i <= arrayLength(leds); i++)
    {
      // Flip the led status
      digitalWrite(leds[i], !digitalRead(leds[i]));
    }
    delay(50);
  }
}

/**
   Calculate the number of items in an array
*/
int arrayLength(int arr[])
{
  return (sizeof(arr) / sizeof(arr[0]));
}

void connectToWifi()
{
  while ((WiFiMulti.run() != WL_CONNECTED))
  {
    Serial.println("Waiting for the WiFi to connect...");
    delay(2000);
  }
}

/**
   Send an HTTP DelayBetweenRequests to the given endpoint.
   If the {state: value} returned by the endpoint is different
   than the condition, light up the specified LED.
*/
void sendRequest(String endpoint, String condition, int led)
{
  WiFiClient client;
  HTTPClient http;
  String url = baseUrl + endpoint;

  if (http.begin(client, url))
  {
    http.addHeader("Authorization", "Bearer " + token);
    http.addHeader("Content-Type", "application/json");

    // start connection and send HTTP header
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      // Serial.printf("[HTTP] Response: %d\n", httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = http.getString();
        // Allocate some memory for the JSON decodification
        StaticJsonDocument<500> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err)
        {
          String sensorState = doc["state"];
          digitalWrite(led, LOW);

          if (sensorState != condition)
          {
            digitalWrite(led, HIGH);
          }
        }
        else
        {
          Serial.printf("[HTTP] GET... failed, error: %s\n", err.c_str());
          fatalError = true;
        }
      }
    }
    else
    {
      fatalError = true;
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  else
  {
    fatalError = true;
    Serial.printf("[HTTP} Unable to connect\n");
  }
}
