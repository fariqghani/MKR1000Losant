/**
 * Example that connects an Arduino Zero with Arduino WiFi Shield 101 to the
 * Losant IoT platform. This example reports state to Losant whenever
 * a button is pressed. It also listens for the "toggle" command to turn the
 * LED on and off.
 *
 * This example assumes the following connections:
 * Button connected to pin 14.
 * LED connected to pin 12.
 *
 * Copyright (c) 2016 Losant. All rights reserved.
 * http://losant.com
 */

#include <WiFi101.h>
#include <Losant.h>
#include "DHT.h"

#define DHTPIN 1
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// WiFi credentials.
const char* WIFI_SSID = "ssid";
const char* WIFI_PASS = "pass";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "device id";
const char* LOSANT_ACCESS_KEY = "access key";
const char* LOSANT_ACCESS_SECRET = "access secret";

const int BUTTON_PIN = 14;
const int LED_PIN = 12;

bool ledState = false;

WiFiSSLClient wifiClient;

// For an unsecured connection to Losant.
// WiFiClient wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);


void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  // For an unsecured connection.
  //device.connect(wifiClient, ACCESS_KEY, ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
}

void setup() {
  Serial.begin(115200);
  while(!Serial) { }

  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");

  connect();
}



int timeSinceLastRead = 0;

void loop() {

  bool toReconnect = false;

  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if(!device.connected()) {
    Serial.println("Disconnected from Losant");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if(toReconnect) {
    connect();
  }

  device.loop();
  // Report every 60 seconds.
  if(timeSinceLastRead > 60000) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float humidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, humidity);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, humidity, false);

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");
    report(humidity, t, f, hic, hif);

    timeSinceLastRead = 0;
  }
  delay(100);
  timeSinceLastRead += 100;
}

void report(double humidity, double tempC, double tempF, double heatIndexC, double heatIndexF) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["tempF"] = tempF;
  root["heatIndexF"] = heatIndexF;
  JsonObject& dataPair = root.createNestedObject("data");
  dataPair["humidity"] = humidity;
  dataPair["tempC"] = tempC;
  dataPair["heatIndexC"] = heatIndexC;
  device.sendState(root);
  device.sendState(dataPair);
  Serial.println("Reported!");
}
