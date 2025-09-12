//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <OpenTherm.h>
#include <ArduinoJson.h>
//#include <FS.h>

// WiFi credentials
//const char* ssid = "KEM_QTECH";
//const char* password = "kemerovskaya";
const char* ssid = "cv_home";
const char* password = "naksitral";

//Master OpenTherm Shield pins configuration
const int OT_IN_PIN = 5;  //4 for ESP8266 (D2), 21 for ESP32
const int OT_OUT_PIN = 4; //5 for ESP8266 (D1), 22 for ESP32

//Temperature sensor pin
const int ROOM_TEMP_SENSOR_PIN = 14; //14 for ESP8266 (D5), 18 for ESP32


//ESP8266WebServer server(80);
WebServer server(80);

// Variables to display
float roomTemperature = 25.0;
float waterTemperature = 47.0;
float steamTemperature = 100.0;
int sliderValue = 20;
bool button1State = false;
bool button2State = false;
bool button3State = false;
bool flameState = false;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Setup server routes
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  });
  
  server.on("/style.css", HTTP_GET, []() {
    File file = SPIFFS.open("/style.css", "r");
    server.streamFile(file, "text/css");
    file.close();
  });
  
  server.on("/data", HTTP_GET, []() {
    StaticJsonDocument<200> doc;
    doc["roomTemp"] = roomTemperature;
    doc["waterTemp"] = waterTemperature;
    doc["steamTemp"] = steamTemperature;
    doc["sliderValue"] = sliderValue;
    doc["button1"] = button1State;
    doc["button2"] = button2State;
    doc["button3"] = button3State;
    doc["flame"] = flameState;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  server.on("/update", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      
      if (doc.containsKey("slider")) {
        sliderValue = doc["slider"];
        Serial.print("Slider updated to: ");
        Serial.println(sliderValue);
      }
      
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
  });
  
  server.on("/button1", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      
      if (doc.containsKey("state")) {
        button1State = doc["state"];
        Serial.print("Button 1 state: ");
        Serial.println(button1State ? "ON" : "OFF");
        
        if (button1State) {
          Serial.println("Button 1 activated!");
        } else {
          Serial.println("Button 1 deactivated!");
        }
      }
      
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
  });
  
  server.on("/button2", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      
      if (doc.containsKey("state")) {
        button2State = doc["state"];
        Serial.print("Button 2 state: ");
        Serial.println(button2State ? "ON" : "OFF");
        
        if (button2State) {
          Serial.println("Button 2 activated!");
        } else {
          Serial.println("Button 2 deactivated!");
        }
      }
      
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
  });

  server.on("/button3", HTTP_POST, []() {
    if (server.hasArg("plain")) {
      StaticJsonDocument<200> doc;
      deserializeJson(doc, server.arg("plain"));
      
      if (doc.containsKey("state")) {
        button3State = doc["state"];
        Serial.print("Button 3 state: ");
        Serial.println(button2State ? "AUTO" : "MAN");
        
        if (button3State) {
          Serial.println("Button 3 activated!");
        } else {
          Serial.println("Button 3 deactivated!");
        }
      }
      
      server.send(200, "application/json", "{\"status\":\"ok\"}");
    }
  });


  // Endpoint to get only button states
  server.on("/buttonStates", HTTP_GET, []() {
    StaticJsonDocument<100> doc;
    doc["button1"] = button1State;
    doc["button2"] = button2State;
    doc["button3"] = button3State;
    doc["flame"] = flameState;
    doc["sliderValue"] = sliderValue;    
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });
  
  // Handle not found
  server.onNotFound([]() {
    server.send(404, "text/plain", "File Not Found");
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  // Update temperatures every 2 seconds (simulated data)
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    
    // Simulate temperature changes
    roomTemperature = 40.0 + (random(0, 150) / 10.0);  // 20-35°C range
    waterTemperature = 40.0 + (random(0, 150) / 10.0);  // 20-35°C range
    steamTemperature = 95.0 + (random(0, 100) / 10.0);  // 95-105°C range
    flameState = true;
    
    Serial.print("Room Temp: ");
    Serial.print(roomTemperature);
    Serial.print(", Water Temp: ");
    Serial.print(waterTemperature);
    Serial.print("°C, Steam Temp: ");
    Serial.print(steamTemperature);
    Serial.print("°C, Slider: ");
    Serial.print(sliderValue);
    Serial.print(", Btn1: ");
    Serial.print(button1State ? "ON" : "OFF");
    Serial.print(", Btn2: ");
    Serial.print(button2State ? "ON" : "OFF");
    Serial.print(", Btn3: ");
    Serial.print(button3State ? "AUTO" : "MAN");
    Serial.print(", Flame: ");
    Serial.println(flameState ? "ON" : "OFF");  
  }
  
  delay(10);
}