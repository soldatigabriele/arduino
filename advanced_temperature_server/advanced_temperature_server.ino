#include "arduino_secrets.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS D8

// Secrets
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
int deviceCount = 0;
float tempC;

ESP8266WebServer server(80);

void temperatures() {
  char temp[400];
  sensors.requestTemperatures(); 
  snprintf(temp, 400,
    "{\"pond\": \"%.1f\", \"air\":\"%.1f\"}",
    sensors.getTempCByIndex(0), sensors.getTempCByIndex(1)
  );
  server.send(200, "application/json", temp);
  digitalWrite(led, 0);
}


void setup(void) {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/temperatures", temperatures);

  server.begin();
  Serial.println("HTTP server started");
  sensors.begin(); // Start up the temperature sensors
  deviceCount = sensors.getDeviceCount();
  Serial.println("Sensors initialised");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
