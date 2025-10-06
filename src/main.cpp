#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <WiFi.h>
//#include <WebServer.h>
//#include "SPIFFS.h"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <OpenTherm.h>
#include <ArduinoJson.h>
#include <FS.h>

#include "ThingSpeak.h"

WiFiClient  client;

#define SECRET_CH_ID 2018752                                 // replace 0000000 with your channel number
#define SECRET_WRITE_APIKEY "C27QA27ACABXEU12"                           // replace XYZ with your channel write API Key

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

// WiFi credentials
const char* ssid = "KEM_QTECH";
const char* password = "kemerovskaya";
//const char* ssid = "cv_home";
//const char* password = "naksitral";

//Master OpenTherm Shield pins configuration
const int OT_IN_PIN = 5;  //4 for ESP8266 (D2), 21 for ESP32
const int OT_OUT_PIN = 4; //5 for ESP8266 (D1), 22 for ESP32

//Temperature sensor pin
const int ROOM_TEMP_SENSOR_PIN = 14; //14 for ESP8266 (D5), 18 for ESP32


ESP8266WebServer server(80);
//WebServer server(80);

// Variables to display
float sp = 24, //set point
t = 0, //current temperature
t_last = 0, //prior temperature
ierr = 0, //integral error
dt = 0, //time between measurements
op = 0; //PID controller output
unsigned long ts = 0, new_ts = 0; //timestamp
int enableCentralHeating = false;
int enableHotWater = false;
int enableCooling = false;
bool enableOutsideTemperatureCompensation = false;
bool enableCentralHeating2 = true;
int  dhw = 45;

float SP_Auto = 23.0; //setpoint auto mode
float SP_Man = 47.0; //setpoint manual mode
float roomTemperature = 25.0;
float waterTemperature = 47.0;   //DHW temp
float steamTemperature = 100.0;  //Boiler temp
int sliderValue = SP_Man;
bool button1State = false;
bool button2State = false;
bool button3State = false;
bool flameState = false;
unsigned long lastUpdate = 0;
unsigned long lastUpdate2 = 0;

OneWire oneWire(ROOM_TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
OpenTherm ot(OT_IN_PIN, OT_OUT_PIN);

void ICACHE_RAM_ATTR handleInterrupt() {
    ot.handleInterrupt();
}

float getTemp() {
  return sensors.getTempCByIndex(0);
}

float pid(float sp, float pv, float pv_last, float& ierr, float dt) {    
  float KP = 10;
  float KI = 0.02;  
  // upper and lower bounds on heater level
  float ophi = 80;
  float oplo = 10;
  // calculate the error
  float error = sp - pv;
  // calculate the integral error
  ierr = ierr + KI * error * dt;  

  // calculate the PID output
  float P = KP * error; //proportional contribution
  float I = ierr; //integral contribution  
  float op = P + I;
  // implement anti-reset windup
  if ((op < oplo) || (op > ophi)) {
    I = I - KI * error * dt;
    // clip output
    op = max(oplo, min(ophi, op));
  }
  ierr = I; 
  Serial.println("sp="+String(sp) + " pv=" + String(pv) + " dt=" + String(dt) + " op=" + String(op) + " P=" + String(P) + " I=" + String(I));
  return op;
}

void updateData2()
{ 

  // set the fields with the values
  ThingSpeak.setField(1, roomTemperature);
  ThingSpeak.setField(2, flameState);
  ThingSpeak.setField(3, steamTemperature);
 
  // Write value to Field 1 of a ThingSpeak Channel
  int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if (httpCode == 200) {
    Serial.println("Channel write successful.");
  }
  else {
    Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
  }
  //httpCode = ThingSpeak.writeField(myChannelNumber, 2, flameState, myWriteAPIKey);
  //httpCode = ThingSpeak.writeField(myChannelNumber, 3, steamTemperature, myWriteAPIKey);


}
void updateData()
{ 
  unsigned long response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling,enableOutsideTemperatureCompensation,enableCentralHeating2);
  OpenThermResponseStatus responseStatus = ot.getLastResponseStatus();
  if (responseStatus != OpenThermResponseStatus::SUCCESS) {
    Serial.println("Error: Invalid boiler response " + String(response, HEX));
  }   
  
  t = sensors.getTempCByIndex(0);
  new_ts = millis();
  dt = (new_ts - ts) / 1000.0;
  ts = new_ts;
  //op = sp;
  if (responseStatus == OpenThermResponseStatus::SUCCESS) {
    Serial.println("Central Heating: " + 
            String(ot.isCentralHeatingActive(response) ? "on" : "off"));
		Serial.println("Hot Water: " + 
            String(ot.isHotWaterActive(response) ? "on" : "off"));
		Serial.println("Flame: " + 
            String(ot.isFlameOn(response) ? "on" : "off"));

    if (button3State){  //auto mode
      op = pid(sliderValue, t, t_last, ierr, dt);
    } else {    //manual mode
      op = sliderValue;
    }
    
    //Set Boiler Temperature    
    ot.setBoilerTemperature(op);
    //Set DHW Temp
    ot.setDHWSetpoint(dhw);
    //read boiler temp
    steamTemperature = ot.getBoilerTemperature();
    waterTemperature = ot.getDHWTemperature();
    flameState = ot.isFlameOn(response);
  }

  Serial.println("Set boiler to " + String(op) + " degrees C");
  t_last = t;
  roomTemperature = t;
  sensors.requestTemperatures(); //async temperature request
  
  Serial.println("Current temperature is " + String(t) + " degrees C");
  Serial.println("Current boiler temp is " + String(steamTemperature) + " degrees C");
  Serial.println("Current DHW temp is " + String(waterTemperature) + " degrees C");
}



void setup() {
  Serial.begin(115200);
  
  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  ot.begin(handleInterrupt);  

  //Init DS18B20 sensor
  sensors.begin();
  sensors.requestTemperatures();
  sensors.setWaitForConversion(false); //switch to async mode
  t_last = sensors.getTempCByIndex(0);
  Serial.println(t_last);
  ts = millis();
  

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
        if (button3State) {
          SP_Auto = sliderValue;
        } else {
          SP_Man = sliderValue;
        }
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
        enableCentralHeating = button1State;
        Serial.print("Button 1 state: ");
        Serial.println(button1State ? "ON" : "OFF");
        
        if (button1State) {
          Serial.println("Boiler heating activated!");
        } else {
          Serial.println("Boiler heating deactivated!");
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
        enableHotWater = button2State;
        Serial.print("Button 2 state: ");
        Serial.println(button2State ? "ON" : "OFF");
        
        if (button2State) {
          Serial.println("Hot water activated!");
        } else {
          Serial.println("Hot water deactivated!");
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
          sliderValue = SP_Auto;
          Serial.println("Auto mode activated!");
        } else {
          sliderValue = SP_Man;
          Serial.println("Manual mode deactivated!");
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

  ThingSpeak.begin(client);
  Serial.println("ThingSpeak started");
}

void loop() {
  server.handleClient();
  
  if (millis() - lastUpdate2 > 60000) {
    lastUpdate2 = millis();
    updateData2();
  }
  

  // Update temperatures every 2 seconds (simulated data)
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();

    updateData();

    
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