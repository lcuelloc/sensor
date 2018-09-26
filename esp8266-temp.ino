#include <ArduinoJson.h>
#include <StreamString.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <DHT.h>
#define DHTTYPE DHT11
//#define DHTTYPE DHT22
#define DHTPIN 2
#define USE_SERIAL Serial
#define MESSAGE_INTERVAL 3000

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
DHT dht(DHTPIN, DHTTYPE, 11);

long previousMillis = 0;
float humidity, temperature;
String temp="";
String hum="";
String sensor_id="";


uint64_t messageTimestamp = 0;
uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      isConnected = false;
      break;
    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);
      isConnected = true;

      // send message to server when Connected
      //webSocket.sendTXT("Connected");
    }
      break;
    case WStype_TEXT:
      USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
  }

}

void setup() {
  // USE_SERIAL.begin(921600);
  USE_SERIAL.begin(115200);

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP("ssid", "password");

  //WiFi.disconnect();
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  // server address, port and URL
  webSocket.begin("host", port, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

}

void getData() {
  
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature)){
      Serial.println("Failed to read from DHT sensor!");
      return;
      }
}

void loop() {
  webSocket.loop();
  
  if(isConnected) {

    unsigned long currentMillis = millis();
  
    if (currentMillis - previousMillis > MESSAGE_INTERVAL) {
      
        previousMillis = currentMillis;
        
        //get data
        sensor_id = String(WiFi.macAddress());
        getData();
        temp = String((int)temperature);
        hum = String((int)humidity);
        
        //Format json
        USE_SERIAL.println(WiFi.macAddress());
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["sensor_id"] = sensor_id;
        
        JsonObject& data = root.createNestedObject("data");
        data.set("temperature", temp);
        data.set("humidity", hum);

        StreamString databuf;
        root.printTo(databuf);

        webSocket.sendTXT(databuf);
      }
    }
}