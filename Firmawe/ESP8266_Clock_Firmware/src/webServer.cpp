#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>


#include "config.h"
#include "webServer.h"
#include "dateTime.h"

const char* fallbackSSID = "ClockConfig";  // AP mode SSID
AsyncWebServer server(80);
DNSServer dnsServer;

struct Config {
  String wifi_ssid;
  String wifi_pass;

  int colorPalet;
  int brightness;
  String nightStart;
  String nightEnd;
  String timezone;
  String time_format;
  String date_format;
  String beepOnHourChange;

  int weatherRefreshInterval;
  String weatherService;
  String weatherApiKey;
  String openWeatherApiKey;
  //String postal_code;
  //String country_code;
  String location;
  String latitude;
  String longitude;

  String statusDate;
  String statusTemp;
  String statusHumidity;
  String statusCode;
  String statusDesc;
};

Config config;
bool shouldReboot = false;
unsigned long rebootTime = 0;
bool wifiConnected = false;


// Serve HTML file html file located in .data/index.html uplode using upload_fs.py
void handleRoot(AsyncWebServerRequest *request) {
  request->send(LittleFS, "/index.html", "text/html");
}

// Serve JSON configuration
void handleConfig(AsyncWebServerRequest *request) {
  JsonDocument doc;
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_pass"] = config.wifi_pass;

  doc["colorPalet"] = config.colorPalet;
  doc["brightness"] = config.brightness;
  doc["nightStart"] = config.nightStart;
  doc["nightEnd"] = config.nightEnd;
  doc["timezone"] = config.timezone;
  doc["time_format"] = config.time_format;
  doc["date_format"] = config.date_format;
  doc["beepOnHourChange"] = config.beepOnHourChange;

  doc["weatherRefreshInterval"] = config.weatherRefreshInterval;
  doc["weatherService"] = config.weatherService;
  doc["weatherApiKey"] = config.weatherApiKey;
  doc["openWeatherApiKey"] = config.openWeatherApiKey;
  //doc["postal_code"] = config.postal_code;
  //doc["country_code"] = config.country_code;
  doc["location"] = config.location;
  doc["latitude"] = config.latitude;
  doc["longitude"] = config.longitude;

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}

// Handle saving new configuration
void handleSave(AsyncWebServerRequest *request) {
  if (request->hasParam("wifi_ssid", true)) config.wifi_ssid = request->getParam("wifi_ssid", true)->value();
  if (request->hasParam("wifi_pass", true)) config.wifi_pass = request->getParam("wifi_pass", true)->value();

  if (request->hasParam("colorPalet", true)) config.colorPalet = request->getParam("colorPalet", true)->value().toInt();
  if (request->hasParam("brightness", true)) config.brightness = request->getParam("brightness", true)->value().toInt();
  if (request->hasParam("nightStart", true)) config.nightStart = request->getParam("nightStart", true)->value();
  if (request->hasParam("nightEnd", true)) config.nightEnd = request->getParam("nightEnd", true)->value();
  if (request->hasParam("timezone", true)) config.timezone = request->getParam("timezone", true)->value();
  if (request->hasParam("time_format", true)) config.time_format = request->getParam("time_format", true)->value();
  if (request->hasParam("date_format", true)) config.date_format = request->getParam("date_format", true)->value();
  if (request->hasParam("beepOnHourChange", true)) config.beepOnHourChange = request->getParam("beepOnHourChange", true)->value();
 
  if (request->hasParam("weatherRefreshInterval", true)) config.weatherRefreshInterval = request->getParam("weatherRefreshInterval", true)->value().toInt();
  if (request->hasParam("weatherApiKey", true)) config.weatherApiKey = request->getParam("weatherApiKey", true)->value();
  if (request->hasParam("openWeatherApiKey", true)) config.openWeatherApiKey = request->getParam("openWeatherApiKey", true)->value();
  if (request->hasParam("weatherService", true)) config.weatherService = request->getParam("weatherService", true)->value();
  //if (request->hasParam("postal_code", true)) config.postal_code = request->getParam("postal_code", true)->value();
  //if (request->hasParam("country_code", true)) config.country_code = request->getParam("country_code", true)->value();
  if (request->hasParam("location", true)) config.location = request->getParam("location", true)->value();
  if (request->hasParam("latitude", true)) config.latitude = request->getParam("latitude", true)->value();
  if (request->hasParam("longitude", true)) config.longitude = request->getParam("longitude", true)->value();

  File file = LittleFS.open("/config.json", "w");
  if (file) {
    JsonDocument doc;
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["wifi_pass"] = config.wifi_pass;

    doc["colorPalet"] = config.colorPalet;
    doc["brightness"] = config.brightness;
    doc["nightStart"] = config.nightStart;
    doc["nightEnd"] = config.nightEnd;
    doc["timezone"] = config.timezone;
    doc["time_format"] = config.time_format;
    doc["date_format"] = config.date_format;
    doc["beepOnHourChange"] = config.beepOnHourChange;
   
    doc["weatherRefreshInterval"] = config.weatherRefreshInterval;
    doc["weatherService"] = config.weatherService;
    doc["weatherApiKey"] = config.weatherApiKey;
    doc["openWeatherApiKey"] = config.openWeatherApiKey;
    //doc["postal_code"] = config.postal_code;
    //doc["country_code"] = config.country_code;
    doc["location"] = config.location;
    doc["latitude"] = config.latitude;
    doc["longitude"] = config.longitude;

    serializeJson(doc, file);
    file.close();
    Serial.println("Configuration saved!");
  } else {
    Serial.println("Failed to save configuration!");
  }

  request->send(200, "text/plain", "Settings Saved! Restarting...");
  shouldReboot = true;
  rebootTime = millis();  // Set reboot timestamp
}

// Load stored configuration
void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File file = LittleFS.open("/config.json", "r");
    if (file) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, file);
      if (!error) {
        config.wifi_ssid = doc["wifi_ssid"].as<String>();
        config.wifi_pass = doc["wifi_pass"].as<String>();

        config.colorPalet = doc["colorPalet"].as<int>();
        config.brightness = doc["brightness"].as<int>();
        config.nightStart = doc["nightStart"].as<String>();
        config.nightEnd = doc["nightEnd"].as<String>();
        config.timezone = doc["timezone"].as<String>();
        config.time_format = doc["time_format"].as<String>();
        config.date_format = doc["date_format"].as<String>();
        config.beepOnHourChange = doc["beepOnHourChange"].as<String>();
        
        config.weatherRefreshInterval = doc["weatherRefreshInterval"].as<int>();
        config.weatherService = doc["weatherService"].as<String>();
        config.weatherApiKey = doc["weatherApiKey"].as<String>();
        config.openWeatherApiKey = doc["openWeatherApiKey"].as<String>();
        //config.postal_code = doc["postal_code"].as<String>();
        //config.country_code = doc["country_code"].as<String>();
        config.location = doc["location"].as<String>();
        config.latitude = doc["latitude"].as<String>();
        config.longitude = doc["longitude"].as<String>();

        Serial.println("Loaded configuration.");
      } else {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
      }
      file.close();
    } else {
      Serial.println("Error: Failed to open config.json.");
    }
  } else {
    Serial.println("Config file not found, using default config.");

    // Check if SSID or Password is empty and set default values
    if (config.wifi_ssid.isEmpty()) config.wifi_ssid = String(DEFAULT_SSID);  // No need to explicitly convert
    if (config.wifi_pass.isEmpty()) config.wifi_pass = String(DEFAULT_PASS);  // No need to explicitly convert

    if (config.colorPalet <= 0) config.colorPalet = DEFAULT_COLOR; 
    if (config.brightness <= 0) config.brightness = DAY_BRIGHTNESS; 
    if (config.nightStart.isEmpty()) config.nightStart = NIGHT_START;
    if (config.nightEnd.isEmpty()) config.nightEnd = NIGHT_END;
    if (config.timezone.isEmpty()) config.timezone = DEFAULT_TIME_ZONE;
    if (config.time_format.isEmpty()) config.time_format = DEFAULT_TIME_FORMAT;
    if (config.date_format.isEmpty()) config.date_format = DateFormat(DEFAULT_DATE_FORMAT);
    if (config.beepOnHourChange.isEmpty()) config.beepOnHourChange = DEFAULT_BEEP_ON_HOUR_CHANGE;
    
    if (config.weatherRefreshInterval <= 0) config.weatherRefreshInterval = WEATHER_REFRESH_INTERVAL;
    if (config.weatherService.isEmpty()) config.weatherService = DEFAULT_WEATHER_SERVICE;
    if (config.weatherApiKey.isEmpty()) config.weatherApiKey = WEATHER_API_KEY;
    if (config.openWeatherApiKey.isEmpty()) config.openWeatherApiKey = OPEN_WEATHER_API_KEY;

    if (config.location.isEmpty()) config.location = DEFAULT_LOCATION;
    if (config.latitude.isEmpty()) config.latitude = DEFAULT_LATITUDE;
    if (config.longitude.isEmpty()) config.longitude = DEFAULT_LONGITUDE;
  }
}

// Serve weather status in JSON
void handleWeatherStatus(AsyncWebServerRequest *request) {
  JsonDocument doc;

  doc["date"] = config.statusDate;
  doc["temperature"] = config.statusTemp;
  doc["humidity"] = config.statusHumidity;
  doc["weatherCode"] = config.statusCode;
  doc["description"] = config.statusDesc;

  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}


// Start Access Point (AP) Mode
void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(fallbackSSID);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.print("AP Mode IP: ");
  Serial.println(WiFi.softAPIP());
}

// Function to connect to WiFi (Non-blocking)
void connectToWiFi() {
  delay(1000);
  // Trim any leading or trailing spaces from SSID and Password
  config.wifi_ssid.trim();
  config.wifi_pass.trim();

  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifi_ssid);
  //Serial.println(config.wifi_pass);
  
  
  // WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);  // Ensure it keeps retrying

  if (config.wifi_ssid.length() > 0 && config.wifi_pass.length() > 0) {
    WiFi.begin(config.wifi_ssid.c_str(), config.wifi_pass.c_str());
  } else {
    Serial.println("No WiFi credentials found! Starting AP mode.");
    startAccessPoint();
    return;
  }

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {  // 15 seconds timeout
    Serial.print(".");
    delay(500);
    yield();  // Prevent watchdog reset
  }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nConnected to WiFi: ");
      Serial.println(config.wifi_ssid);
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true;
  } else {
      Serial.println("\nWiFi Connection Failed! Starting AP mode.");
      startAccessPoint();
  }
}

// Initialize Web Server
void initWebServer() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS Mount Failed!");
    return;
  }

  loadConfig();
  WiFi.disconnect();        // Ensure a fresh start
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);  // Set hostname for mDNS and OTA
  MDNS.begin(HOSTNAME);     // Start mDNS with the specified hostname [https://Smart-Clock.local]
  connectToWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/weatherStatus", HTTP_GET, handleWeatherStatus);

  server.begin();
}

void startMDNS() {
  if (MDNS.begin("SMART-CLOCK")) {
    Serial.println("mDNS started: SMART-CLOCK.local");
  } else {
    Serial.println("mDNS failed");
  }
}

// Loop function to handle reboot (Non-blocking)
void loopWebServer() {
  dnsServer.processNextRequest();  // Required for captive portal
  MDNS.update();                   // Update mDNS

  // Handle reboot (Non-blocking)
  if (shouldReboot && millis() - rebootTime > 3000) {  // 3 seconds delay using millis()
    ESP.restart();
  }
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool isWifiAP_Mode() {
  return WiFi.getMode() == WIFI_AP;
}

const char* getTimeZone() {
  return config.timezone.c_str();
}

bool getTimeFormat() {
  //Serial.println("Time Format: " + config.time_format);
  return (config.time_format == "Y") ? true : false;
}

bool getBeepOnHourChange() {
  return (config.beepOnHourChange == "Y") ? true : false;
}

String getWeatherService() {
  return config.weatherService;
}

uint8_t getWeatherRefreshInterval() {
  return config.weatherRefreshInterval;
}

String getLocation() {
  return config.location;
}

String getLatitude() {
  return config.latitude;
}

String getLongitude() {
  return config.longitude;
}

String getWeatherApiKey() {
  return config.weatherApiKey;
}

String getOpenWeatherApiKey() {
  return config.openWeatherApiKey;
}

String AP_Mode_IP() {
  return WiFi.softAPIP().toString();
}

String Wifi_IP() {
  return WiFi.localIP().toString();
}

int getColorPalet() {
  return config.colorPalet;
}

int getBrightness() {
  return config.brightness;
}


int getNightStartHour() {
  return config.nightStart.substring(0, 2).toInt();
}

int getNightStartMinute() {
  return config.nightStart.substring(3, 5).toInt();
}

int getNightEndHour() {
  return config.nightEnd.substring(0, 2).toInt();
}

int getNightEndMinute() {
return config.nightEnd.substring(3, 5).toInt();
}

DateFormat getDateFormat() {
  String formatDate = config.date_format;

  if (formatDate == "MON-DD-YYYY") return MON_DD_YYYY;
  else if (formatDate == "YYYY_MM_DD") return YYYY_MM_DD;
  else if (formatDate == "DD_MM_YYYY") return DD_MM_YYYY;
  else if (formatDate == "MM_DD_YYYY") return MM_DD_YYYY;
  else if (formatDate == "YYYY/MM/DD") return YYYY_SLASH_MM_SLASH_DD;
  else if (formatDate == "DD/MM/YYYY") return DD_SLASH_MM_SLASH_YYYY;
  else if (formatDate == "MM/DD/YYYY") return MM_SLASH_DD_SLASH_YYYY;
  else return DEFAULT_DATE_FORMAT; // Fallback to default
}


// In webServer.cpp

void updateWeatherStatus(float temperature, int humidity, const String &weatherCode, const String &description) {
  config.statusTemp = String(temperature, 1);
  config.statusHumidity = String(humidity);
  config.statusCode = weatherCode;
  config.statusDesc = description;

  // Optionally update timestamp
  config.statusDate = getCurrentDate(); 
}
