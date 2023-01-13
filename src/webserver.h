// Import required libraries
#include <Arduino.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <EEPROM.h>
#include <connectionManager.h>

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

bool pressRed = false;
bool pressBlue = false;
bool pressBlack = false;

unsigned long lastActivity = 0;

String message = "";
typedef struct {
  bool valveState = 0;
  bool force = 0;
  String workdayStart = "00:00";
  String workdayEnd = "00:00";
  String weekendStart = "00:00";
  String weekendEnd = "00:00";
  int warmphaseDuration = 10;
  int impulsDuration = 800;
  String workdayHeattimes[10];
  String weekendHeattimes[10];
} Settings;

Settings settings;

//Json Variable to Hold Slider Values
JSONVar states;

void initEEPROM() {
  EEPROM.begin(sizeof(Settings));
  // EEPROM.put(0, settings);
  // EEPROM.commit();
  EEPROM.get(0, settings);
  EEPROM.end();

  Serial.println("Filesystem started");
  Serial.println();
}

void storeEEPROM() {
  EEPROM.begin(sizeof(Settings));
  EEPROM.put(0, settings);
  EEPROM.commit();
  EEPROM.end();
}

//Get state values
String getStateValues(){
  states["valveState"] = settings.valveState;
  states["force"] = settings.force;
  states["workdayStart"] = String(settings.workdayStart);
  states["workdayEnd"] = String(settings.workdayEnd);
  states["weekendStart"] = String(settings.weekendStart);
  states["weekendEnd"] = String(settings.weekendEnd);
  states["warmphaseDuration"] = String(settings.warmphaseDuration);
  states["impulsDuration"] = String(settings.impulsDuration);
  for(byte num = 0; num < max(settings.workdayHeattimes->length(), settings.weekendHeattimes->length()); num++) {
    states["workdayHeattime"+String(num)] = settings.workdayHeattimes[num];
    states["weekendHeattime"+String(num)] = settings.weekendHeattimes[num];
  }

  String jsonString = JSON.stringify(states);
  return jsonString;
}

void notifyClients(String states) {
  ws.textAll(states);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;
    Serial.println(message);
    if(message.indexOf("valveState") >= 0) {
      if(settings.force) return;
      if(message.substring(10)=="true") pressRed = true; else pressBlue = true;
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("force") >= 0) {
      if(message.substring(5)=="true" != settings.force) pressBlack = true;
      // settings.force = message.substring(5)=="true";
      // if(settings.force && settings.valveState) closeValve();
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("workdayStart") >= 0) {
      settings.workdayStart = message.substring(12);
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("workdayEnd") >= 0) {
      settings.workdayEnd = message.substring(10);
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }    
    if (message.indexOf("weekendStart") >= 0) {
      settings.weekendStart = message.substring(12);
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("weekendEnd") >= 0) {
      settings.weekendEnd = message.substring(10);
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("warmphaseDuration") >= 0) {
      settings.warmphaseDuration = message.substring(17).toInt();
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("impulsDuration") >= 0) {
      settings.impulsDuration = message.substring(14).toInt();
      // Serial.println(getStateValues());
      notifyClients(getStateValues());
    }
    if (message.indexOf("workdayHeattimes") >= 0) {
      message = message.substring(16);
      while(message.indexOf("workdayHeattime") >= 0) {
        message = message.substring(message.indexOf("workdayHeattime"));
        message = message.substring(15);
        short num = message.substring(0,1).toInt();
        if(num<10) settings.workdayHeattimes[num] = message.substring(1, message.indexOf("workdayHeattime"));
      }
      notifyClients(getStateValues());
    }
    else if (message.indexOf("workdayHeattime") >= 0) {
      short num = message.substring(15,16).toInt();
      if(num>9) return;
      settings.workdayHeattimes[num] = message.substring(16);
      notifyClients(getStateValues());
    }
    if (message.indexOf("Heattimes") >= 0) {
      String type = message.substring(0,7);
      message = message.substring(16);
      while(message.indexOf(type+"Heattime") >= 0) {
        message = message.substring(message.indexOf(type+"Heattime"));
        message = message.substring(15);
        short num = message.substring(0,1).toInt();
        short next = message.indexOf(type+"Heattime");
        if(num<10) (type=="workday"?settings.workdayHeattimes:settings.weekendHeattimes)[num] = next>1 ? message.substring(1, next) : next==-1 ? message.substring(1) : "";
      }
      notifyClients(getStateValues());
    }
    else if (message.indexOf("weekendHeattime") >= 0) {
      short num = message.substring(15,16).toInt();
      if(num>9) return;
      settings.weekendHeattimes[num] = message.substring(16);
      // Serial.println (getStateValues());
      notifyClients(getStateValues());
    }

    if (strcmp((char*)data, "getValues") == 0) {
      notifyClients(getStateValues());
    }
    storeEEPROM();
    lastActivity = millis();
  }
}
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initWebServer() {
  Serial.println("starting webserver");
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css","text/css");
  });
  
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();

  MDNS.addService("http", "tcp", 80);

  Serial.println("webserver started");
  Serial.println();
}


void webserverSetup() {
  initFS();
  initEEPROM();

  initWiFi();
  initOTA();

  initWebSocket();
  initWebServer(); 
}

void webserverLoop() {
  ws.cleanupClients();
  ArduinoOTA.handle();
  MDNS.update();
  timeClient.update();

  reconnectWiFi();
}