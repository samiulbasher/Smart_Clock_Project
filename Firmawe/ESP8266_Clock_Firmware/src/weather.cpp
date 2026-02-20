#include <ESP8266WiFi.h> //Open-Meteo → use plain WiFiClient (HTTP).

//WTTR, weatherAPI → use WiFiClientSecure (HTTPS).
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>


#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "webServer.h"
#include "weather.h"
#include "dateTime.h"


const char* OpenMeteoCodeText[] = {
  "Unknown",              // 0
  "Sunny",                // 1
  "Partly Cloudy",        // 2
  "Cloudy",               // 3
  "Drizzle/Rain",         // 4
  "Thunderstorm",         // 5
  "Snow",                 // 6
  "Mist/Fog",             // 7
  "Clear Night",          // 8
  "Fog Night",            // 9
  "Partly Cloudy Night",  // 10
  "Cloudy Night"          // 11
};



//Global/static values
float _temperature = -999;
int _humidity = -1;
float _windSpeed = -1;
String _windDirection = "";
int _weatherCode = -1;
const char* condText = "Unknown";
uint8_t _isDay = 0;
uint8_t _iconCode = 0;

// Global/static backup values
static float lastTemp = -999;
static int   lastHum  = -1;
static float lastWind = -1;
static String lastDir = "";
static String lastDesc = "Unknown";
static uint8_t lastIcon = 0;


//StaticJsonDocument<2048> doc;
JsonDocument doc; 
//JsonDocument doc = StaticJsonDocument<2048>();

//maeby leater add Open-Meteo with Japan / JMA support, WeatherAPI.com
bool updateWeather() {
  bool success = false;
  const String useWeatherService = getWeatherService();
  //Serial.println(useWeatherService); 
  Serial.println(); 

  if(useWeatherService == "wttr") { 
    success = fetchWttr();
  }
  else if (useWeatherService == "open-meteo") {
    success = fetchOpenMeteo();
  }
  else if (useWeatherService == "weatherAPI") {
    success = fetchWeatherAPI();
  }
  else if (useWeatherService == "openWeather") {
    success = fetchOpenWeather(); 
  }
  else {
    Serial.println("Unknown weather service");
    success = false;
  }


  // After parsing in any fetch function:
  if (_temperature != -999) lastTemp = _temperature;
  else _temperature = lastTemp;

  if (_humidity >= 0) lastHum = _humidity;
  else _humidity = lastHum;

  if (_windSpeed >= 0) lastWind = _windSpeed;
  else _windSpeed = lastWind;

  if (_windDirection.length() > 0) lastDir = _windDirection;
  else _windDirection = lastDir;

  if (String(condText) != "Unknown") lastDesc = condText;
  else condText = lastDesc.c_str();

  if (_iconCode != 0) lastIcon = _iconCode;
  else _iconCode = lastIcon;



  return success;
}

// WTTR: Fetch weather from wttr.in
bool fetchWttr() {
  WiFiClient client;

  // Example: https://wttr.in/yonezawa?format=%C+%t+%h&T
  const char* host = "wttr.in";
  String city = getLocation();
  String url  = "/" + city + "?format=%C+%t+%h&T";  // ASCII only

  Serial.print("Connecting to "); Serial.println(host);
  if (!client.connect(host, 80)) {
    Serial.println("WTTR HTTP connect failed");
    return false;
  }

  // Send request with User-Agent
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: curl/7.58.0\r\n" +   // <-- important!
               "Connection: close\r\n\r\n");

  // Skip headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  // Read response body (only first line needed)
  String payload = client.readStringUntil('\n');
  client.stop();

  if (payload.length() < 5) {
    Serial.println("WTTR: Empty payload");
    return false;
  }

  payload.trim();
  //Serial.print("WTTR raw: "); Serial.println(payload);

  // Example: "Partly cloudy +29°C 66%"
  int tempPos = payload.indexOf('+');
  int humPos  = payload.indexOf('%');
  int degPos  = payload.indexOf("°");

  if (tempPos > 0 && humPos > 0 && degPos > 0) {
    String desc  = payload.substring(0, tempPos - 1);
    String tempS = payload.substring(tempPos + 1, degPos);
    String humS  = payload.substring(payload.lastIndexOf(' ', humPos - 1) + 1, humPos);

    _temperature = tempS.toFloat();
    _humidity    = humS.toInt();
    _isDay       = (getCurrentHour24() >= 6 && getCurrentHour24() < 18);
    _iconCode    = wttr_TextToIcon(desc, _isDay);

    updateWeatherStatus(_temperature, _humidity, String(_iconCode), desc);

    /*
    //Print weather information
    Serial.println("WTTR parsed OK:");
    Serial.print("  Desc: "); Serial.println(desc);
    Serial.print("  Temp: "); Serial.println(_temperature);
    Serial.print("  Hum : "); Serial.println(_humidity);
    Serial.print("  Day? "); Serial.println(_isDay ? "yes" : "no");
    */

    return true;
  }

  Serial.println("WTTR: Parse failed");
  return false;
}


// OPEN_METRO: Fetch weather from Open-Meteo
bool fetchOpenMeteo() {
  // Open-Meteo uses plain HTTP
  WiFiClient client;

  const char* weatherserver = "api.open-meteo.com";
  String latitude = getLatitude();
  String longitude = getLongitude();

  // Example: https://api.open-meteo.com/v1/forecast?latitude=37.916667&longitude=140.116667&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day
  String url = "/v1/forecast?latitude=" + latitude +
          "&longitude=" + longitude +
          "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day";
  
  Serial.print("Connecting to ");
  Serial.println(weatherserver);

  if (!client.connect(weatherserver, 80)) {
    Serial.println("Connection failed");
    return false;
  }

  // Send HTTP GET request
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + weatherserver + "\r\n" +
                "Accept: application/json\r\n" +
                "Connection: close\r\n\r\n");

  client.setTimeout(1000);  // 1s is usually enough for Open-Meteo

  // Skip headers by reading until first '{'
  String headers = client.readStringUntil('{');
  //Serial.println(headers);  //Take this out after debugging, it slows down the clock

  // Read the rest of the JSON
  String payload = "{" + client.readString(); // JSON string does not have a terminator, timeout will end readstring
  //Serial.println(payload);  //Take this out after debugging, it slows down the clock
  
  // Close connection
  client.stop();
  
  if (payload.length() < 10) {
    Serial.println("Open-Meteo: Empty payload");
    return false;
  }

  // Debug: show first part of response
  // Serial.println("Payload snippet:");
  // Serial.println(payload.substring(0, 120));

  // Parse JSON
  if (deserializeJson(doc, payload)) {
    Serial.println("Open-Meteo JSON parse failed");
    _temperature = -999;
    _humidity = -1;
    _windSpeed = -1;
    _windDirection = "";
    _weatherCode = -1;
    _isDay = 0;
    _iconCode = 0;
    return false;
  }
  
  // Extract "current" object
  JsonObject current = doc["current"];
  if (!current.isNull()) {
    
    _temperature   = current["temperature_2m"].is<float>()     ? current["temperature_2m"].as<float>() : -999;
    _humidity      = current["relative_humidity_2m"].is<int>() ? current["relative_humidity_2m"].as<int>() : -1;
    _windSpeed     = current["wind_speed_10m"].is<float>()     ? current["wind_speed_10m"].as<float>() : -1;
    _windDirection = current["wind_direction_10m"].is<int>()   ? String(current["wind_direction_10m"].as<int>()) : "";
    _weatherCode   = current["weather_code"].is<int>()         ? current["weather_code"].as<int>() : -1;
    _isDay         = current["is_day"].is<int>()               ? current["is_day"].as<int>() : 0;

    // Map Open-Meteo code to icon number
    _iconCode = OpenMeteo_CodeToIcon(_weatherCode, _isDay);


    // Pass data to webServer
    updateWeatherStatus(_temperature, _humidity, String(_iconCode), OpenMeteoCodeText[_iconCode]);

    // Debug output
    /*
    Serial.println("WeatherAPI parsed OK:");
    Serial.print("  Temperature: "); Serial.println(_temperature);
    Serial.print("  Humidity : "); Serial.println(_humidity);
    Serial.print("  Weather Code: "); Serial.println(_iconCode);
    Serial.print("  Description : "); Serial.println(_weatherCode);
    */
    
    return true;  // success
  }

  return false;
}


// WEATHERAPI.COM: Fetch weather
bool fetchWeatherAPI() {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();   // No certificate validation, but works
  client->setBufferSizes(512, 512);
  client->setTimeout(3000);

  HTTPClient https;

  String apiKey = getWeatherApiKey();   // stored key
  String latitude  = getLatitude();
  String longitude = getLongitude();

  if (apiKey.length() == 0 || latitude.length() == 0 || longitude.length() == 0) {
    Serial.println("WeatherAPI: Missing API key or coordinates");
    return false;
  }

  // Example: https://api.weatherapi.com/v1/current.json?key=KEY&q=35.68,139.69&aqi=no
  String url = "https://api.weatherapi.com/v1/current.json?key=" + apiKey +
               "&q=" + latitude + "," + longitude +
               "&aqi=no";

  Serial.print("Connecting to: ");
  //Serial.println(url);
  Serial.println("weatherAPI");

  if (!https.begin(*client, url)) {
    Serial.println("WeatherAPI HTTPS begin failed");
    return false;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("WeatherAPI HTTP failed, code: %d\n", httpCode);
    https.end();
    return false;
  }

  String payload = https.getString();
  https.end();

  if (payload.length() < 10) {
    Serial.println("WeatherAPI: Empty payload");
    return false;
  }

  // Debug: show first part of response
  // Serial.println("Payload snippet:");
  // Serial.println(payload.substring(0, 120));

  // Parse JSON
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("WeatherAPI JSON parse failed: ");
    Serial.println(error.c_str());
    return false;
  }

  JsonObject current = doc["current"];
  if (current.isNull()) {
    Serial.println("WeatherAPI: No 'current' object");
    return false;
  }

  // Safe extraction
  _temperature   = current["temp_c"].is<float>()   ? current["temp_c"].as<float>()   : -999;
  _humidity      = current["humidity"].is<int>()   ? current["humidity"].as<int>()   : -1;
  _windSpeed     = current["wind_kph"].is<float>() ? current["wind_kph"].as<float>() : -1;
  _windDirection = current["wind_dir"].is<const char*>() ? String(current["wind_dir"].as<const char*>()) : "";
  _isDay         = current["is_day"].is<int>()     ? current["is_day"].as<int>()     : 0;

  // Condition object
  int code = -1;
  if (current["condition"].is<JsonObject>()) {
    JsonObject cond = current["condition"];
    if (cond["text"].is<const char*>()) condText = cond["text"].as<const char*>();
    if (cond["code"].is<int>()) code = cond["code"].as<int>();
  }

  _iconCode = WeatherAPI_CodeToIcon(code, _isDay);

  // Pass data to webServer
  updateWeatherStatus(_temperature, _humidity, String(_iconCode), String(condText));

  /*
  // Debug output
  Serial.println("WeatherAPI parsed OK:");
  Serial.print("  Desc: "); Serial.println(condText);
  Serial.print("  Temp: "); Serial.println(_temperature);
  Serial.print("  Hum : "); Serial.println(_humidity);
  Serial.print("  Wind: "); Serial.println(_windSpeed);
  Serial.print("  Dir : "); Serial.println(_windDirection);
  Serial.print("  Day? "); Serial.println(_isDay ? "yes" : "no");
  */

  return true;  // success
}


//Open Weather Map
bool fetchOpenWeather() {
  Serial.println("Open Weather Map fetch not implemented yet");
  return false;
}


void printWeatherReport() {
  // Print weather information
  Serial.println("Weather Report:");
  Serial.print("Temperature: "); Serial.print(_temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(_humidity); Serial.println(" %");
  Serial.print("Wind Speed: "); Serial.print(_windSpeed); Serial.println(" km/h");
  Serial.print("Wind Direction: "); Serial.println(_windDirection);
  Serial.print("Weather Type: "); Serial.println(_weatherCode);
  Serial.print("Day: "); Serial.println(_isDay ? "yes" : "no");

  Serial.println();
}

uint8_t OpenMeteo_CodeToIcon(int weatherCode, uint8_t isDay) {
  if (!isDay) {  // Nighttime mappings
    if (weatherCode == 0 || weatherCode == 1) return 8;  // Clear Night
    if (weatherCode == 2) return 10;  // Partly Cloudy Night
    if (weatherCode == 3) return 11;  // Cloudy Night
    if (weatherCode == 45 || weatherCode == 48) return 9;  // Fog Night
    if (weatherCode >= 51 && weatherCode <= 57) return 4;  // Drizzle/Rain (Night)
    if (weatherCode >= 61 && weatherCode <= 67) return 4;  // Rain (Night)
    if (weatherCode >= 71 && weatherCode <= 77) return 6;  // Snow (Night)
    if (weatherCode >= 80 && weatherCode <= 86) return 4;  // Showers (Night)
    if (weatherCode == 95 || weatherCode == 96 || weatherCode == 99) return 5;  // Thunderstorm (Night)
  } else {  // Daytime mappings
    if (weatherCode == 0 || weatherCode == 1) return 1;  // Sunny
    if (weatherCode == 2) return 2;  // Cloudy
    if (weatherCode == 3) return 3;  // Overcast
    if (weatherCode == 45 || weatherCode == 48) return 7;  // Mist
    if (weatherCode >= 51 && weatherCode <= 57) return 4;  // Drizzle/Rain
    if (weatherCode >= 61 && weatherCode <= 67) return 4;  // Rain
    if (weatherCode >= 71 && weatherCode <= 77) return 6;  // Snow
    if (weatherCode >= 80 && weatherCode <= 86) return 4;  // Rain Showers
    if (weatherCode == 95 || weatherCode == 96 || weatherCode == 99) return 5;  // Thunderstorm
  }
  return 0;  // Default case (Unknown)
}

uint8_t wttr_TextToIcon(String desc, uint8_t isDay) {
  desc.toLowerCase();  // make case-insensitive

  if (!isDay) {  // Nighttime mappings
    if (desc.indexOf("clear") >= 0) return 8;        // Clear Night
    if (desc.indexOf("partly") >= 0 || desc.indexOf("cloud") >= 0) return 10; // Partly Cloudy Night
    if (desc.indexOf("overcast") >= 0) return 11;    // Overcast Night
    if (desc.indexOf("fog") >= 0 || desc.indexOf("mist") >= 0) return 9;  // Fog/Mist Night
    if (desc.indexOf("drizzle") >= 0 || desc.indexOf("rain") >= 0 || desc.indexOf("shower") >= 0) return 4; // Rain Night
    if (desc.indexOf("snow") >= 0 || desc.indexOf("sleet") >= 0 || desc.indexOf("ice") >= 0) return 6;      // Snow Night
    if (desc.indexOf("thunder") >= 0 || desc.indexOf("storm") >= 0) return 5; // Thunderstorm Night
  } else {  // Daytime mappings
    if (desc.indexOf("sunny") >= 0 || desc.indexOf("clear") >= 0) return 1;  // Sunny
    if (desc.indexOf("partly") >= 0 || desc.indexOf("cloud") >= 0) return 2; // Partly Cloudy
    if (desc.indexOf("overcast") >= 0) return 3;     // Overcast
    if (desc.indexOf("fog") >= 0 || desc.indexOf("mist") >= 0) return 7; // Mist/Fog
    if (desc.indexOf("drizzle") >= 0 || desc.indexOf("rain") >= 0 || desc.indexOf("shower") >= 0) return 4; // Rain
    if (desc.indexOf("snow") >= 0 || desc.indexOf("sleet") >= 0 || desc.indexOf("ice") >= 0) return 6;      // Snow
    if (desc.indexOf("thunder") >= 0 || desc.indexOf("storm") >= 0) return 5; // Thunderstorm
  }

  return 0;  // Default case (Unknown)
}


uint8_t WeatherAPI_CodeToIcon(int code, uint8_t isDay) {
  if (!isDay) {  // Nighttime mappings
    if (code == 1000) return 8;   // Clear Night
    if (code == 1003) return 10;  // Partly Cloudy Night
    if (code == 1006) return 11;  // Cloudy Night
    if (code == 1009) return 11;  // Overcast Night
    if (code == 1030 || code == 1135 || code == 1147) return 9;  // Mist/Fog Night
    if ((code >= 1063 && code <= 1087) || (code >= 1150 && code <= 1201)) return 4;  // Rain/Showers Night
    if ((code >= 1210 && code <= 1237) || (code >= 1255 && code <= 1264)) return 6;  // Snow Night
    if (code >= 1273 && code <= 1282) return 5;  // Thunderstorms Night
  } else {  // Daytime mappings
    if (code == 1000) return 1;   // Sunny
    if (code == 1003) return 2;   // Partly Cloudy
    if (code == 1006) return 3;   // Cloudy
    if (code == 1009) return 3;   // Overcast
    if (code == 1030 || code == 1135 || code == 1147) return 7;  // Mist/Fog
    if ((code >= 1063 && code <= 1087) || (code >= 1150 && code <= 1201)) return 4;  // Rain/Showers
    if ((code >= 1210 && code <= 1237) || (code >= 1255 && code <= 1264)) return 6;  // Snow
    if (code >= 1273 && code <= 1282) return 5;  // Thunderstorms
  }

  return 0;  // Default case (Unknown)
}



float getTemparature() {
  return _temperature;
}

int getHumidity() {
  return _humidity;
}

float getWindSpeed() {
  return _windSpeed;
}

String getWindDirection() {
  return _windDirection;
}

uint8_t getWeatherIcon() {
  return _iconCode;
}

bool getIsDay() {
  return _isDay;
}



/*
  if(useWeatherService == "wttr") {  //WTTR

    // WTTR requires HTTPS
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(2000);

    String city = getLocation();
    city.replace(" ", "%20"); // URL encode spaces

    if (!client.connect("wttr.in", 443)) {
      Serial.println("WTTR HTTPS connect failed");
      return;
    }

    client.print(String("GET /") + city + "?format=j2 HTTP/1.1\r\n" +
                 "Host: wttr.in\r\n" +
                 "User-Agent: Mozilla/5.0 (ESP8266)\r\n" +
                 "Accept: application/json\r\n" +
                 "Connection: close\r\n\r\n");

    // Skip headers until first '{'
    String headers = client.readStringUntil('{');
    //Serial.println(headers);  //Take this out after debugging, it slows down the clock
    String payload = "{" + client.readString();
    //Serial.println(payload);  //Take this out after debugging, it slows down the clock
    client.stop();

    if (payload.length() < 10) {
      Serial.println("WTTR: Empty payload");
      return;
    }

    if (deserializeJson(doc, payload)) {
      Serial.println("WTTR JSON parse failed");
      _temperature = -999;
      _humidity = -1;
      _windSpeed = -1;
      _windDirection = "";
      _weatherCode = -1;
      _isDay = false;
      _iconCode = 0;
      return;
    }

    JsonArray currentArr = doc["current_condition"];
    if (!currentArr.isNull() && currentArr.size() > 0) {
      JsonObject current = currentArr[0];
      _temperature   = current.containsKey("temp_C")          ? current["temp_C"].as<float>() : -999;
      _humidity      = current.containsKey("humidity")        ? current["humidity"].as<int>() : -1;
      _windSpeed     = current.containsKey("windspeedKmph")   ? current["windspeedKmph"].as<float>() : -1;
      _windDirection = current.containsKey("winddir16Point")  ? current["winddir16Point"].as<String>() : "";
      
      // _weatherCode   = 1;//-1;  // WTTR does not provide numeric code, map manually if needed
      
      String weatherText = "";
      // if (current.containsKey("weatherDesc") && current["weatherDesc"][0].containsKey("value")) {
      //     weatherText = current["weatherDesc"][0]["value"].as<String>();
      // }

      weatherText = current.containsKey("weatherDesc") ? current["weatherDesc"][0]["value"].as<String>() : "";
      
      _isDay         = true; // assume day, or parse from "weatherDesc" if you want
      _iconCode      = wttr_TextToIcon(weatherText, _isDay);


      // Print weather information
      printWeatherReport();
    }
  }
*/

/*
void updateWeather() {
  // uint32_t currentMillis = millis();  // Get current time

  // Only fetch the weather if WiFi is connected and the interval has passed
  // if (currentMillis - previousMillis_weather >= WEATHER_REFRESH_INTERVAL) {
  //   previousMillis_weather = currentMillis;

  const String useWeatherService = getWeatherService();
  //Serial.println(useWeatherService);  
  // WTTR: Fetch weather from wttr.in
  if(useWeatherService == "wttr") {  //WTTR
    // const String city = getLocation();

    // String url = "https://wttr.in/" + city + "?format=j2";

    // Serial.println("\nFetching weather data from wttr...");

    // // Initialize HTTPClient
    // http.begin(client, url);  // Use HTTPS
    // int httpCode = http.GET();  // Make the HTTP GET request

    // if (httpCode == HTTP_CODE_OK) {
    //   String payload = http.getString();  // Get the response
    //   //Serial.println("Full JSON Response:");
    //   //Serial.println(payload);

    //   // Parse the JSON response
    //   //DynamicJsonDocument doc(1024);  // Set buffer size
    //   JsonDocument doc;  // Use JsonDocument instead of DynamicJsonDocument
    //   DeserializationError error = deserializeJson(doc, payload);

    //   if (error) {
    //     Serial.print("JSON Parsing failed: ");
    //     Serial.println(error.c_str());
    //     http.end();
    //     return;
    //   }

    //   JsonObject current_condition = doc["current_condition"][0];

    //   _temperature = current_condition["temp_C"];  // Temperature in Celsius
    //   _humidity = current_condition["humidity"];  // Humidity
    //   _windSpeed = current_condition["windspeedKmph"].as<float>();  // Wind speed in km/h
    //   _windDirection = current_condition["winddir16Point"].as<String>();  // Wind direction

    //   String feels_like = current_condition["FeelsLikeC"];  // Feels like temperature in Celsius
    //   String weather_description = current_condition["weatherDesc"][0]["value"];  // Weather description

    //   // Print weather information
    //   //printWeatherReport();
    // } else {
    //   Serial.print("Error fetching weather from wttr.in: ");
    //   Serial.println(httpCode);
    // }
  }

  // OPEN_METRO: Fetch weather from Open-Meteo
  else if(useWeatherService == "open-meteo") { // OPEN_METEO
          
    const char* weatherserver = "api.open-meteo.com";
    String latitude = getLatitude();
    String longitude = getLongitude();

    String url = "/v1/forecast?latitude=" + latitude +
            "&longitude=" + longitude +
            "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day";
    
    
    Serial.print("Connecting to ");
    Serial.println(weatherserver);

    client.setTimeout(3000);  // Set 1s timeout

    if (!client.connect(weatherserver, 80)) {
      Serial.println("Connection failed");
      return;
    }
  
    // Send HTTP GET request
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + weatherserver + "\r\n" +
    "Accept: application/json\r\n" +
    "Connection: close\r\n\r\n");

    // Skip HTTP headers
    client.readStringUntil('{');

    // Read full JSON payload
    String payload = "{";
    while (client.connected() || client.available()) {
      if (client.available()) {
        payload += char(client.read());
      } else {
        yield();  // background tasks run
        delay(1); // slow down tight loop slightly
      }
    }

    // Close connection
    client.stop();
  
    // Parse JSON
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON parse failed: ");
      Serial.println(error.c_str());
      return;
    }

    JsonObject current = doc["current"];
    _temperature = current["temperature_2m"];
    _humidity = current["relative_humidity_2m"];
    _windSpeed = current["wind_speed_10m"];
    _windDirection = String(current["wind_direction_10m"].as<int>());
    _weatherCode = current["weather_code"];
    _isDay = current["is_day"];

    // Map Open-Meteo code to icon number
    _iconCode = mapWeatherCodeToIcon(_weatherCode, _isDay);
  
    // Serial.print("Temperature: "); Serial.print(_temperature); Serial.println(" °C");
    // Serial.print("Humidity: "); Serial.print(_humidity); Serial.println(" %");
    // Serial.println();
    //printWeatherReport();
  }
  else {
      Serial.println("Error");
  }
}
*/


/*
void updateWeather() {
  uint32_t currentMillis = millis();  // Get current time

  // Only fetch the weather if WiFi is connected and the interval has passed
  if (currentMillis - previousMillis_weather >= WEATHER_REFRESH_INTERVAL) {
    previousMillis_weather = currentMillis;

    const String useWeatherService = getWeatherService();
    //Serial.println(useWeatherService);  
    // WTTR: Fetch weather from wttr.in
    if(useWeatherService == "wttr") {  //WTTR
      const String city = getLocation();

      String url = "https://wttr.in/" + city + "?format=j2";

      Serial.println("\nFetching weather data from wttr...");

      // Initialize HTTPClient
      http.begin(client, url);  // Use HTTPS
      int httpCode = http.GET();  // Make the HTTP GET request

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();  // Get the response
        //Serial.println("Full JSON Response:");
        //Serial.println(payload);

        // Parse the JSON response
        //DynamicJsonDocument doc(1024);  // Set buffer size
        JsonDocument doc;  // Use JsonDocument instead of DynamicJsonDocument
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
          Serial.print("JSON Parsing failed: ");
          Serial.println(error.c_str());
          http.end();
          return;
        }

        JsonObject current_condition = doc["current_condition"][0];

        _temperature = current_condition["temp_C"];  // Temperature in Celsius
        _humidity = current_condition["humidity"];  // Humidity
        _windSpeed = current_condition["windspeedKmph"].as<float>();  // Wind speed in km/h
        _windDirection = current_condition["winddir16Point"].as<String>();  // Wind direction

        String feels_like = current_condition["FeelsLikeC"];  // Feels like temperature in Celsius
        String weather_description = current_condition["weatherDesc"][0]["value"];  // Weather description

        // Print weather information
        //printWeatherReport();
      } else {
        Serial.print("Error fetching weather from wttr.in: ");
        Serial.println(httpCode);
      }
    }

    // OPEN_METRO: Fetch weather from Open-Meteo
    else if(useWeatherService == "open-meteo") { // OPEN_METEO
      String latitude = getLatitude();
      String longitude = getLongitude();
      // Construct the URL for Open-Meteo
      // String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude) + "&longitude=" + String(longitude) + "&current_weather=true";
      String url = "https://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + 
                   "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day";
      
      Serial.println("\nFetching weather data from open-meteo...");

      // Initialize HTTPClient
      http.begin(client, url);  // Use HTTPS
      int httpCode = http.GET();  // Make the HTTP GET request
      for (int i = 0; i < 10; i++) {
        //display.showBuffer();
        yield();  // Prevent watchdog reset
        delay(10);  // Small pause to let display update
      }
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();  // Get the response
        //Serial.println("Full JSON Response:");
        //Serial.println(payload);

        // Parse the JSON response
        //DynamicJsonDocument doc(1024);  // Set buffer size
        JsonDocument doc;  // Use JsonDocument instead of DynamicJsonDocument
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
          Serial.print("JSON Parsing failed: ");
          Serial.println(error.c_str());
          http.end();
          return;
        }
        
        // Extract weather data from Open-Meteo API
        _temperature = doc["current"]["temperature_2m"].as<float>();
        _humidity = doc["current"]["relative_humidity_2m"].as<int>();
        _windSpeed = doc["current"]["wind_speed_10m"].as<float>();
        _windDirection = doc["current"]["wind_direction_10m"].as<String>();
        _weatherCode = doc["current"]["weather_code"];
        _isDay = doc["current"]["is_day"].as<bool>();

        // Map Open-Meteo code to icon number
        _iconCode = mapWeatherCodeToIcon(_weatherCode, _isDay);


        //printWeatherReport();
      } else {
        Serial.print("Error fetching weather from Open-Meteo: ");
        Serial.println(httpCode);
      }
    }
    else {
        Serial.println("Error");
    }
    http.end();  // Close the HTTP connection
  }
}

*/

/*
void updateWeather() {
  switch (weatherState) {
    case IDLE:
      if (millis() - lastWeatherUpdate > WEATHER_INTERVAL && isWifiConnected()) {
        lastWeatherUpdate = millis();

        // Determine which service to use
        currentWeatherService = getWeatherService();

        if (currentWeatherService == "wttr") {
          const String city = getLocation();
          weatherUrl = "https://wttr.in/" + city + "?format=j2";
        }
        else if (currentWeatherService == "open-meteo") {
          const String lat = getLatitude();
          const String lon = getLongitude();
          weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=" + lat + "&longitude=" + lon +
                        "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day";
        } else {
          Serial.println("Unknown weather service!");
          weatherState = DONE;
          return;
        }

        Serial.println("Starting weather request from: " + currentWeatherService);
        weatherState = START_REQUEST;
      }
      break;

      case START_REQUEST:
        http.begin(client, weatherUrl);
        weatherState = WAIT_RESPONSE;
        break;

      case WAIT_RESPONSE: {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
          weatherPayload = http.getString();
          weatherState = PARSE_RESPONSE;
        } else {
          Serial.println("HTTP error code: " + String(httpCode));
          http.end();
          weatherState = DONE;
        }
        break;
      }

      case PARSE_RESPONSE: {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, weatherPayload);
        if (error) {
          Serial.print("JSON error: ");
          Serial.println(error.c_str());
          http.end();
          weatherState = DONE;
          return;
        }

        if (currentWeatherService == "wttr") {
          JsonObject current_condition = doc["current_condition"][0];

          _temperature = current_condition["temp_C"].as<float>();
          _humidity = current_condition["humidity"].as<int>();
          _windSpeed = current_condition["windspeedKmph"].as<float>();
          _windDirection = current_condition["winddir16Point"].as<String>();
          // Additional parsing like feels_like or weather_description can go here
        }
        else if (currentWeatherService == "open-meteo") {
          _temperature = doc["current"]["temperature_2m"].as<float>();
          _humidity = doc["current"]["relative_humidity_2m"].as<int>();
          _windSpeed = doc["current"]["wind_speed_10m"].as<float>();
          _windDirection = doc["current"]["wind_direction_10m"].as<String>();
          _weatherCode = doc["current"]["weather_code"];
          _isDay = doc["current"]["is_day"].as<bool>();
          _iconCode = mapWeatherCodeToIcon(_weatherCode, _isDay);
        }

        http.end();
        weatherState = DONE;
        break;
      }

      case DONE:
        weatherState = IDLE;  // reset for next cycle
        break;
  }
}  
*/

/*
void getWeather() {
  uint32_t currentMillis = millis();  // Get current time

  // Only fetch the weather if WiFi is connected and the interval has passed
  if (isWifiConnected() && (currentMillis - previousMillis_weather >= interval_weather)) {
    previousMillis_weather = currentMillis;  // Update last execution time

    // Construct the URL for the weather data
    
    //   %C → Weather condition (e.g., "Clear", "Cloudy")
    //   %t → Actual temperature (e.g., "+5°C")
    //   %f → "Feels like" temperature (apparent temperature)
    

      https://wttr.in/$yonezawa?format=:+%c+%l+is+currently+%C+with+a+temperature+of+%t+(+%f).+The+wind+is+blowing+from+%w.+The+Humidity+is+currently+%h
    //String url = "https://wttr.in/" + city + "?format=%C+%t"; 
    //String url = "https://wttr.in/" + String(city) + "?format=%t&no-redirect";  // Use HTTPS here
    //String url = "http://wttr.in/yonezawa?format=%t&no-redirect";
    //String url = "https://wttr.in/" + city + "?format=%C+%t+Feels_like:%f&no-cache";
    //String url = "https://wttr.in/" + city + "?format=%C+%t&no-redirect&no-cache";  
    String url = "https://wttr.in/" + city + "?format=%C+%t&no-redirect&no-cache";
    
    Serial.println("Fetching weather data...");
    
    // Initialize HTTPClient
    http.begin(client, url);    // Use HTTPS
    int httpCode = http.GET();  // Make the HTTP GET request

    if (httpCode == HTTP_CODE_OK) {
      String temperature = http.getString();  // Get the response
      Serial.print("Temperature in ");
      Serial.print(city);
      Serial.print(": ");
      Serial.println(temperature);
    } else {
      String response = http.getString();
      Serial.print("Error getting temperature: ");
      Serial.println(response);  // Print the error message from the server
    }

    http.end();  // Close the HTTP connection
  }
}
*/




// void getWeather() {
//   uint32_t currentMillis = millis();  // Get current time

//   // Only fetch the weather if WiFi is connected and the interval has passed
//   if (isWifiConnected() && (currentMillis - previousMillis_weather >= interval_weather)) {
//     previousMillis_weather = currentMillis;  // Update last execution time

//     // Construct the URL for the weather data
//     /*
//       %C → Weather condition (e.g., "Clear", "Cloudy")
//       %t → Actual temperature (e.g., "+5°C")
//       %f → "Feels like" temperature (apparent temperature)
//       */

//     //https://wttr.in/$yonezawa?format=:+%c+%l+is+currently+%C+with+a+temperature+of+%t+(+%f).+The+wind+is+blowing+from+%w.+The+Humidity+is+currently+%h
    
//     //String url = "https://wttr.in/" + city + "?format=%C+%t"; 
//     //String url = "https://wttr.in/" + String(city) + "?format=%t&no-redirect";  // Use HTTPS here
//     //String url = "http://wttr.in/yonezawa?format=%t&no-redirect";
//     //String url = "https://wttr.in/" + city + "?format=%C+%t+Feels_like:%f&no-cache";
//     //String url = "https://wttr.in/" + city + "?format=%C+%t&no-redirect&no-cache";  
//     String url = "https://wttr.in/" + city + "?format=j1";  


//     Serial.println("Fetching weather data...");
    
//     // Initialize HTTPClient
//     http.begin(client, url);    // Use HTTPS
//     int httpCode = http.GET();  // Make the HTTP GET request
//     Serial.print("HTTP Response Code: ");
//     Serial.println(httpCode);

//     if (httpCode == 200) { // HTTP OK
//       String payload = http.getString();
//       Serial.println("HTTP Response:");
//       Serial.println(payload);

//       // Parse JSON
//       //JsonDocument doc; // Increase buffer size for large payloads
//       StaticJsonDocument<2048> doc;
//       DeserializationError error = deserializeJson(doc, payload);

//       if (error) {
//         Serial.print("JSON Parsing failed: ");
//         Serial.println(error.c_str());

//         Serial.println("Error parsing");
//         Serial.println("weather data!");

//       } else {
//         // Extract weather data
//         String weather = doc["current_condition"][0]["weatherDesc"][0]["value"].as<String>();
//         String temp = doc["current_condition"][0]["temp_C"].as<String>();

//         // Truncate long weather descriptions
//         if (weather.length() > 20) {
//           weather = weather.substring(0, 20) + "...";
//         }


//         // Display weather and room data on OLED
//         Serial.println("Dresden Weather:");
//         Serial.print("Temp: ");
//         Serial.print(temp);
//         Serial.println(" C");
//         Serial.print("Condition: ");
//         Serial.println(weather);

//       }
//     } else {
//       // Print error
//       Serial.print("HTTP Error: ");
//       Serial.println(http.errorToString(httpCode).c_str());
//     }
//     http.end();
//   }
// }