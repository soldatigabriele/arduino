// Wifi stuff
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

ESP8266WiFiMulti WiFiMulti;

const int buttonPin = D2;
const int ledPin =  D13;      // the number of the LED pin

int buttonState = 0;
bool trigger = false;
int oldStatus = 0;

void setup()
{
    // Initialize the LED_BUILTIN pin as an output and turn them OFF
    pinMode(LED_BUILTIN, OUTPUT); 
    pinMode(ledPin, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(ledPin, LOW);
    
    Serial.begin(9600);
    pinMode(buttonPin, INPUT);

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("ParlaParla", "passwordwifi");

    Serial.println("READY");
}

void loop()
{
    // wait for WiFi connection
    if ((WiFiMulti.run() == WL_CONNECTED))
    {
          Serial.println("connected");

          sendRequest();

//        buttonState = digitalRead(buttonPin);
//        if (buttonState == HIGH && oldStatus == LOW)
//        {
//            // Send the HTTP Request
//            sendRequest();
//        }
//        oldStatus = buttonState;
    } else {
        Serial.println("Waiting for the WiFi to connect...");
    }
}

void sendRequest()
{
    WiFiClient client;
    HTTPClient http;

    // Read the temperature and humidity from the DHT11 sensor
    String url = "http://192.168.1.104:8123/api/states/sensor.verisure_house";

    if (http.begin(client, url))
    {
      
        http.addHeader("Authorization", "Bearer abc.def.ghi");
        http.addHeader("Content-Type", "application/json");
        
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
    delay(1000000);
}
