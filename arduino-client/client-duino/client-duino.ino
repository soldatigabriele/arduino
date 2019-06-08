// Wifi stuff
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
ESP8266WiFiMulti WiFiMulti;

// Humidity sensor
#include "DHT.h"
#define DHTPIN D8
#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

const int buttonPin = D2;

int buttonState = 0;
bool trigger = false;
int oldStatus = 0;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

    Serial.begin(9600);
    pinMode(buttonPin, INPUT);

    dht.begin();

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("SSID", "Password");

    Serial.println("READY");
}

void loop()
{
    // wait for WiFi connection
    if ((WiFiMulti.run() == WL_CONNECTED))
    {
        buttonState = digitalRead(buttonPin);
        if (buttonState == HIGH && oldStatus == LOW)
        {
            // Send the HTTP Request
            sendRequest();
        }
        oldStatus = buttonState;
    } else {
        Serial.println("Waiting for the WiFi to connect...");
    }
}

String getTemperature()
{
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    return "?hum=" + String(h) + "&temp=" + String(t);
}

void sendRequest()
{
    WiFiClient client;
    HTTPClient http;

    // Read the temperature and humidity from the DHT11 sensor
    String temp = getTemperature();
    String url = "http://192.168.1.96:81/api/temperature" + temp;

    if (http.begin(client, url))
    {
        Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            Serial.printf("[HTTP] Response: %d\n", httpCode);
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                Serial.println(payload);
            }
        }
        else
        {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
    }
}
