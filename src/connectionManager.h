#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <FS.h>

ESP8266WiFiMulti wifiMulti;

const char* dns_name = "heizungssteuerung";

// time client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Week Days
String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


void printWifiStatus();
void connectWiFi();

void initWiFi() {
  Serial.println("Starting WiFi");

  wifiMulti.addAP("NetFrame", "87934hzft9oeu4389nv8o437893hf978");
  wifiMulti.addAP("JCBS-WLAN", "K1,12Wviaa,dgeM,KGzw.");
  wifiMulti.addAP("Movement2", "12345678");

  WiFi.mode(WIFI_STA);
  connectWiFi();
}

void connectWiFi() {
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }

  Serial.println();

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Connection failed");
    return;
  }
  
  Serial.println();

  Serial.println("WiFi connected");
  printWifiStatus();

  Serial.println();

  if(MDNS.begin(dns_name)) {
    Serial.println("DNS gestartet, erreichbar unter: ");
    Serial.println("http://" + String(dns_name) + ".local/");
  } else Serial.println("Error setting up MDNS responder!");

  // Initialize a NTPClient to get time
  timeClient.setTimeOffset(3600);
  timeClient.begin();

  Serial.println();
}

void initOTA() {
  // Start ArduionOTA
  Serial.println("starting OTA");
  ArduinoOTA.setHostname(dns_name);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA started");
  Serial.println();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void reconnectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;
  connectWiFi();
}

// Initialize SPIFFS
void initFS() {
  Serial.println("Starting Filesystem");
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else{
   Serial.println("SPIFFS mounted successfully");
  }
}