#include <Arduino.h>

#include <Ticker.h>
#include "PxMatrix.h"
#include "FireFrames.h"

#include "config.h"

#include "dateTime.h"
#include "webServer.h"

#include "weather.h"
#include "weatherIcons.h"

#include "TinyFont.h"
#include "TinyIcons.h"
#include "Digit.h"

#include "buzzer.h"

/*
  *https://github.com/mike-rankin/ESP8266_RGB_Matrix_Cable_Free_Version
  *https://www.instructables.com/RGB-LED-Matrix-With-an-ESP8266/
  *https://github.com/tehniq3/P5_64x32_HUB75
  *https://github.com/gquiring/MorphingClockQ
*/
// Creates a second buffer for backround drawing (i think not working)
#define PxMATRIX_double_buffer true

PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);

Ticker display_ticker;

//Ticker timerInturrupt;

// some other colors
// R G B
uint16_t cc_blk = display.color565(0, 0, 0);         // black
uint16_t cc_wht = display.color565(100, 100, 100);   // white (adjusted)
uint16_t cc_bwht = display.color565(255, 255, 255);  // bright white
uint16_t cc_red = display.color565(50, 0, 0);        // red
uint16_t cc_bred = display.color565(255, 0, 0);      // bright red
uint16_t cc_org = display.color565(255, 100, 0);     // orange (adjusted)
uint16_t cc_borg = display.color565(255, 165, 0);    // bright orange
uint16_t cc_grn = display.color565(0, 100, 0);       // green
uint16_t cc_bgrn = display.color565(0, 255, 0);      // bright green
uint16_t cc_blu = display.color565(0, 0, 150);       // blue (adjusted)
uint16_t cc_bblu = display.color565(0, 0, 255);      // bright blue (adjusted)
uint16_t cc_ylw = display.color565(45, 45, 0);       // yellow
uint16_t cc_bylw = display.color565(255, 255, 0);    // bright yellow
uint16_t cc_gry = display.color565(10, 10, 10);      // gray
uint16_t cc_bgry = display.color565(128, 128, 128);  // bright gray
uint16_t cc_dgr = display.color565(30, 30, 30);      // dark grey (adjusted)
uint16_t cc_cyan = display.color565(0, 150, 150);    // cyan (adjusted)
uint16_t cc_bcyan = display.color565(0, 255, 255);   // bright cyan
uint16_t cc_ppl = display.color565(128, 0, 128);     // purple (adjusted)
uint16_t cc_bppl = display.color565(255, 0, 255);    // bright purple


// Colors for time, wind, date and weather text  (Temperature color varies based on actual temp)
int cc_time;
int cc_wupdt;
int cc_date;
int cc_wtext;

byte hh;
byte mm;
byte ss;

byte prev_hh = 0;
byte prev_mm = 0;
byte prev_ss = 0;


String date;
String week;
String prev_ampm;
String prev_date;


float temparature;
int humidity;
float prev_temp;
int prev_humid;


bool isTime24Hour = false;
volatile bool updateFlag = false;  // Flag to indicate when to update

volatile bool wifiStatus = false;

static bool updatingFromWeatherService = false;
static bool weatherUpdateStarted = false;
int lastTriggeredMinute = -1;

int displayBrightness = DAY_BRIGHTNESS;

// ISR for display refresh
void display_updater() {
  display.display(displayBrightness);//set brightness
  display.showBuffer();  // Double Buffering to Reduce Flicker
}

//=== SEGMENTS ===
// This section determines the position of the HH:MM ss digits onscreen with format digit#(&display, 0, x_offset, y_offset, irrelevant_color)

byte digit_offset_amount;

byte DigitSec_x = 60;
byte Digit_x = 67;

Digit digit0(&display, 0, DigitSec_x - 7 * 1, 14, display.color565(255, 255, 255), 3);
Digit digit1(&display, 0, DigitSec_x - 7 * 2, 14, display.color565(255, 255, 255), 3);
Digit digit2(&display, 0, Digit_x - 4 - 9 * 3, 8, display.color565(255, 255, 255), 6);
Digit digit3(&display, 0, Digit_x - 4 - 9 * 4, 8, display.color565(255, 255, 255), 6);
Digit digit4(&display, 0, Digit_x - 7 - 9 * 5, 8, display.color565(255, 255, 255), 6);
Digit digit5(&display, 0, Digit_x - 7 - 9 * 6, 8, display.color565(255, 255, 255), 6);

// Color palette
void selectColorPalette() {
  int x;
  x = getColorPalet();

  switch (x) {
    case 1:
      cc_time = cc_bwht;
      cc_wupdt = cc_ylw;
      cc_date = cc_wht;
      cc_wtext = cc_wht;
      break;
    case 2:
      cc_time = cc_cyan;
      cc_wupdt = cc_ylw;
      cc_date = cc_grn;
      cc_wtext = cc_wht;
      break;
    case 3:
      cc_time = cc_red;
      cc_wupdt = cc_ylw;
      cc_date = cc_blu;
      cc_wtext = cc_grn;
      break;
    case 4:
      cc_time = cc_blu;
      cc_wupdt = cc_grn;
      cc_date = cc_ylw;
      cc_wtext = cc_wht;
      break;
    case 5:
      cc_time = cc_ylw;
      cc_wupdt = cc_cyan;
      cc_date = cc_blu;
      cc_wtext = cc_grn;
      break;
    case 6:
      cc_time = cc_bblu;
      cc_wupdt = cc_grn;
      cc_date = cc_ylw;
      cc_wtext = cc_grn;
      break;
    case 7:
      cc_time = cc_org;
      cc_wupdt = cc_red;
      cc_date = cc_grn;
      cc_wtext = cc_ylw;
      break;
    case 8:
      cc_time = cc_grn;
      cc_wupdt = cc_ppl;
      cc_date = cc_cyan;
      cc_wtext = cc_ylw;
      break;
    default:
      cc_time = cc_bwht;
      cc_wupdt = cc_ylw;
      cc_date = cc_wht;
      cc_wtext = cc_wht;
      break;
  }
}

void setDigitColor() {
  digit0.SetColor(cc_time);
  digit1.SetColor(cc_time);
  digit2.SetColor(cc_time);
  digit3.SetColor(cc_time);
  digit4.SetColor(cc_time);
  digit5.SetColor(cc_time);
}

void drawWeatherIcons(uint8_t weatherIcon) {
  int xo = 27; // x position of the icon
  int yo =  1; // y position of the icon

  switch (weatherIcon) {
    case 1:  //sunny
      DrawIcon(&display, sunny_ico, xo, yo, 10, 5);
      break;
    case 2:  //cloudy
      DrawIcon(&display, cloudy_ico, xo, yo, 10, 5);
      break;
    case 3:  //overcast
      DrawIcon(&display, ovrcst_ico, xo, yo, 10, 5);
      break;
    case 4:  //rainy
      DrawIcon(&display, rain_ico, xo, yo, 10, 5);
      break;
    case 5:  //thunders
      DrawIcon(&display, thndr_ico, xo, yo, 10, 5);
      break;
    case 6:  //snow
      DrawIcon(&display, snow_ico, xo, yo, 10, 5);
      break;
    case 7:  //mist
      DrawIcon(&display, mist_ico, xo, yo, 10, 5);
      break;
    case 8:  //clear night
      DrawIcon(&display, moony_ico, xo, yo, 10, 5);
      break;
    case 9:  //fog night
      DrawIcon(&display, mistn_ico, xo, yo, 10, 5);
      break;
    case 10:  //partly cloudy night
      DrawIcon(&display, cloudyn_ico, xo, yo, 10, 5);
      break;
    case 11:  //cloudy night
      DrawIcon(&display, ovrcstn_ico, xo, yo, 10, 5);
      break;
    default:
	    DrawText (&display, String("  "), xo, yo, 0);
      break;
  }
}


uint32_t lt = 0;
static int stp = 0;
uint32_t ani_speed = 200;  // Faster animation (200ms per frame)
  
void drawWeatherAnimations(uint8_t weatherIcon) {
  int xo = 27; // x position of the icon
  int yo =  1; // y position of the icon
  int *af = NULL;

  // Update animation step
  uint32_t ct = millis();
  if ((ct - lt) > ani_speed) { 
    lt = ct;
    stp++;
  }

  // Select animation based on weatherType
  switch (weatherIcon) {  
    case 1:  // Sunny
      af = suny_ani[stp % 5];  
      break;
    case 2:  // Cloudy
      af = clod_ani[stp % 10];
      break;
    case 3:  // Overcast
      af = ovct_ani[stp % 5];
      break;
    case 4:  // Rainy
      af = rain_ani[stp % 5];
      break;
    case 5:  // Thunder
      af = thun_ani[stp % 5];
      break;
    case 6:  // Snow
      af = snow_ani[stp % 5];
      break;
    case 7:  // Mist
      af = mist_ani[stp % 4];
      break;
    case 8:  // Clear Night
      af = mony_ani[stp % 17];
      break;
    case 9:  // Fog Night
      af = mistn_ani[stp % 4];
      break;
    case 10:  // Partly Cloudy
      af = clodn_ani[stp % 10];
      break;
    case 11:  // Cloudy Night
      af = ovctn_ani[stp % 1];
      break;
    default:
      af = suny_ani[0];  // Fallback to Sunny if unknown type
      break;
  }

  // Reset `stp` if it exceeds animation length
  if (stp >= 17) stp = 0;

  // Draw animation
  if (af) {
    DrawIcon(&display, af, xo, yo, 10, 5);
  }
}



unsigned long prevMill_Scroll_Text = 0;
void scrollText(String text, int xStart, int yStart, int width, int height, byte speed, uint16_t color) {
  static int xPos = xStart + width;  // Keep track of the position
  int textWidth = text.length() * 6; // Estimate text width (6 pixels per character)

  unsigned long currentMillis_Scroll_Text = millis();
  if (currentMillis_Scroll_Text - prevMill_Scroll_Text >= speed) {
    prevMill_Scroll_Text = currentMillis_Scroll_Text;
    
    // Clear only the text area dynamically to avoid unnecessary filling
    display.fillRect(xStart, yStart, width, height, cc_blk); // Clear scroll area
    
    // Prevent text wrapping for smooth scrollin
    display.setTextWrap(false); 

    // Draw new text position
    //display.setTextSize(1);  // Set text size
    display.setTextColor(color);
    display.setCursor(xPos, yStart);
    display.print(text);
    //display.display();  // Refresh the display
    
    xPos--;  // Move text left

    // Reset position if text is fully scrolled
    if (xPos < xStart - textWidth) {
      xPos = xStart + width;
    }
  }
}

void displayWeather() {
  static uint8_t weatherIcon = 0;
  if(humidity > 0 && temparature > -50.0f && !updatingFromWeatherService) {
    // Draw the humidity
    if(humidity != prev_humid) {
      prev_humid = humidity;
      DrawText(&display, String(humidity) + "%", 2, 1, cc_wtext, false);
    }

    // Draw the temperature
    if(temparature != prev_temp) {
      prev_temp = temparature;
      //DrawText(&display, temparature + "*" + "C", 38, 1, cc_bwht, false);

      // The '0' forces no decimal places
      DrawText_RTL(&display, String(round(temparature), 0) + "*C", 61, 1, cc_wtext);

      weatherIcon = getWeatherIcon();
      //Serial.print(weatherIcon);

      // Draw weather icon
      drawWeatherIcons(weatherIcon);
    }
  }
}

void displayWeatherWithAnimations() {
  //!= bug: it is not print temp humidity every time, sometime missing display so here desable the prev check
  static uint8_t weatherIcon = 0;
  if(humidity > 0 && temparature > -50.0f && !updatingFromWeatherService) {
    // Draw the humidity
    DrawText(&display, String(humidity) + "%", 2, 1, cc_wtext, false);

    // Draw the temperature
    // The '0' forces no decimal places
    DrawText_RTL(&display, String(round(temparature), 0) + "*C", 61, 1, cc_wtext);
    weatherIcon = getWeatherIcon();

    // Draw weather animation
    drawWeatherAnimations(weatherIcon);

    //ScrollText(&display, "Hello!", 15, 1, 30, 8, cc_bred, cc_blk, 50);
  }
}

void loadDisplayClock() {
  prev_ss = ss;
  prev_mm = mm;
  prev_hh = hh;

  digit0.Draw(ss % 10);
  digit1.Draw(ss / 10);
  digit2.Draw(mm % 10);
  digit3.Draw(mm / 10);
  digit4.Draw(hh % 10);
  digit5.Draw(hh / 10);
}

// Function to Get Time and Display Clock
void displayClock() {
  digit3.DrawColon(cc_time);

  //seconds
  if (ss != prev_ss) {
    setDigitColor(); //color set

    int s0 = ss % 10;
    int s1 = ss / 10;    

    //secounds
    if (updatingFromWeatherService) {
      //not display the seconds when updating weather
      digit0.Morph(0);
      digit1.Morph(0);
    } else{
      if (s0 != digit0.Value()) digit0.Morph(s0);
      if (s1 != digit1.Value()) digit1.Morph(s1);
      prev_ss = ss;
    }
  }

  //minutes
  if (mm != prev_mm) {
    int m0 = mm % 10;
    int m1 = mm / 10;
    if (m0 != digit2.Value()) digit2.Morph(m0);
    if (m1 != digit3.Value()) digit3.Morph(m1);
    prev_mm = mm;
  }

  //hours
  if (hh != prev_hh) {
    int h0 = hh % 10;
    int h1 = hh / 10;
    if (h0 != digit4.Value()) digit4.Morph(h0);
    if (h1 != digit5.Value()) digit5.Morph(h1);
    prev_hh = hh;
  }

  // Draw AM/PM
  String ampm = getCurrentAmPm();
  if (!isTime24Hour && ampm != prev_ampm) {
    prev_ampm = ampm;
    DrawText(&display, getCurrentAmPm(), 50, 19, cc_time, false);  
  }

  // Draw the date
  String date_week = date + "(" + week + ")";
  if (date_week.length() && date_week != prev_date) {
    prev_date = date_week; 
    DrawText(&display, date_week, 0, 26, cc_date, true);
  }
}

void getDateTime() {
  isTime24Hour = getTimeFormat();
  updateDateTime(isTime24Hour, getDateFormat()); 
  
  hh = getCurrentHour();
  mm = getCurrentMinute();
  ss = getCurrentSecond();

  date = getCurrentDate();
  week = getCurrentWeekDay();
  //Serial.println(String(hh) + ":" + String(mm) + ":" + String(ss));
}


void getWeather() {
  static bool weatherUpdateSuccess = false;
  static bool retriedOnce = false;
  static bool firstAttemptDone = false;

  uint16_t refreshMinutes = getWeatherRefreshInterval();

  // If no weather data yet, fallback to 1 minute updates
  bool noWeatherData = (humidity <= 0 || temparature <= -50.0f);
  if (noWeatherData) {
    refreshMinutes = 1;
  }

  // Step 1: Trigger only once per aligned minute boundary
  if (ss == 0 && (mm % refreshMinutes == 0) && mm != lastTriggeredMinute) {
    updatingFromWeatherService = true;    // enable updating
    weatherUpdateStarted = false;         // reset
    retriedOnce = false;                  // reset retry flag
    firstAttemptDone = false;
    weatherUpdateSuccess = false;         // reset success
    lastTriggeredMinute = mm;

    // Delay showing the message until next loop cycle (after clock draw)
    // So instead of showing it here, just mark the flag
  }

  // Step 2: Actually show the "WEATHER UPDATE" message
  // Show the "WEATHER UPDATE" message during the 1–9s window
  if (updatingFromWeatherService && ss > 0 && ss < 9) { 
    DrawText(&display, "                     ", 0, 1, cc_wht, false);
    DrawText(&display, "WEATHER UPDATE", 0, 1, cc_wupdt, true);

    // First attempt (run only once)
    if (!weatherUpdateStarted) {        
      Serial.println("Weather update triggered.");
      weatherUpdateSuccess = updateWeather();
      weatherUpdateStarted = true;      // mark as done
      firstAttemptDone = true;
    }
  }

  // Step 3: Retry only AFTER first attempt is done and reply is failed
  if (updatingFromWeatherService && firstAttemptDone && !weatherUpdateSuccess && !retriedOnce) {
    // give it a later window, e.g. 7–9s
    if (ss >= 7 && ss <= 9) {
      Serial.println("Retrying weather update...");
      weatherUpdateSuccess = updateWeather();
      retriedOnce = true;
    }
  }

  // Step 4: Clear update message 10s later
  if (updatingFromWeatherService && ss >= 10) {
    DrawText(&display, "                    ", 0, 1, cc_wht, false);

    if (weatherUpdateSuccess) {
      Serial.println("Weather updated successfully.");
      prev_humid = -1; // force redraw next loop 
      prev_temp = -999.0f; // force redraw next loop
    } else {
      Serial.println("Weather update failed after retry.");
    }
    updatingFromWeatherService = false;

  }

  // Step 5: Regular updates for display
  temparature = getTemparature();
  humidity = getHumidity();

  //Serial.println(humidity);
}

void setBrightness() {
  int currentHour = getCurrentHour24();     // returns 0–23
  int currentMinute = getCurrentMinute();   // returns 0-59
  
  int nightStartHour = getNightStartHour(); // from settings
  int nightEndHour   = getNightEndHour();   // from settings

  int nightStartMinute = getNightStartMinute(); // from settings
  int nightEndMinute   = getNightEndMinute();   // from settings


  // Convert all times to minutes since midnight
  int now   = currentHour * 60 + currentMinute;
  int start = nightStartHour * 60 + nightStartMinute;
  int end   = nightEndHour * 60 + nightEndMinute;

  bool inNightMode = false;

  if (start < end) {
    // Normal same-day range, e.g. 22:00 → 23:59
    inNightMode = (now >= start && now < end);
  } else if (start > end) {
    // Cross-midnight range, e.g. 22:00 → 06:00
    inNightMode = (now >= start || now < end);
  } else {
    // start == end → treat as "always night"
    inNightMode = true;
  }

  if (inNightMode) {
    displayBrightness = NIGHT_BRIGHTNESS;  // e.g. 20
    //Serial.println("Set night mode brightness");
  } else {
    displayBrightness = getBrightness();   // from config
    //Serial.println("Set brightness from config");
  }
}

// Beep on the hour
void playBeep() {
  if (!getBeepOnHourChange()) return; // Exit if beep on hour change is disabled

  static int lastBeepHour = -1;

  int h = getCurrentHour24();
  int m = getCurrentMinute();
  int s = getCurrentSecond();


  if (m == 0 && s == 0  && h != lastBeepHour) {
    lastBeepHour = h;
    beeper(2,SHORT_BEEP);
  }
}


void setup() {
  Serial.begin(BAUDRATE);
  delay(500);  //delay for MCU
  display.begin(16); // 16 is brightness level (0-255)

  display.clearDisplay();

  // Set the color order {RRGGBB, RRBBGG, GGRRBB, GGBBRR, BBRRGG, BBGGRR} (default is RRGGBB)
  display.setColorOrder(RRBBGG); // my p2.5 display is in RBG format

  /* //check display color order is okey or not
  display.drawPixel(1, 0, display.color565(255, 0, 0));  // Should be RED
  display.drawPixel(2, 0, display.color565(0, 255, 0));  // Should be GREEN
  display.drawPixel(3, 0, display.color565(0, 0, 255));  // Should be BLUE
  */

  // Set up the display refresh ISR
  //display_ticker.attach(0.01, display_updater); //10 ms or 100Hz
  //display_ticker.attach(0.005, display_updater); //5 ms or 200Hz
  display_ticker.attach(0.004, display_updater); //4 ms or 250Hz (A bit more CPU usage, but usually still fine on ESP8266)
  //display_ticker.attach(0.003, display_updater); //3 ms or 333Hz (Moderate CPU usage, should be okay for most ESP8266 projects)
  //display_ticker.attach(0.002, display_updater); //2 ms or 500Hz (Very high CPU usage, Risk of starving WiFi stack (ESP8266 needs idle time for background tasks))

  setBrightness(); //set display brightness
  
  //starting message
  DrawText(&display, String("SMART CLOCK"),0 , 1, cc_bgrn, true);

  // Initialize the web server ap mode or wifi mode
  initWebServer();

  if(isWifiAP_Mode()) {
    DrawText(&display, String("AP MODE"), 0, 8, cc_grn, true);
    DrawText(&display, AP_Mode_IP(), 0, 15, cc_ylw, true);
  } 
  else {
    wifiStatus = true;
    DrawText(&display, String("WIFI CONNECTED"), 0, 8, cc_grn, true);
    DrawText(&display, Wifi_IP(), 0, 15, cc_ylw, true);

    // Only Initialize if connected the wifi
    // Initialize the time
    configDateTime();
    delay(5000);  //delay for wellcome screen

    selectColorPalette();
    setDigitColor();

    display.clearDisplay();

    getDateTime();
    getWeather();

    initBuzzer();

    loadDisplayClock();

    beeper(2, LONG_BEEP); //beep to indicate setup done
  }
}

void loop() {
  //Handle the web server
  loopWebServer();

  // Handle buzzer tones
  manageBuzzer();


  if(isWifiConnected()) {
    if(!wifiStatus) {
      wifiStatus = true;
      prev_ampm = prev_date = "0"; //force reset
      prev_humid = -1;
      prev_temp = -999.0f; //force reset
      
      display.clearDisplay();
      loadDisplayClock();
    }
    getDateTime();
    getWeather();

    setBrightness();

    #if DISPLAY_WITH_ANIMATIONS
      displayWeatherWithAnimations();
    #else
      displayWeather();
    #endif

    displayClock(); // Display the clock

    playBeep(); // Beep on the hour
  }
  else {
    if (!isWifiAP_Mode()) {
      if (wifiStatus) {
        wifiStatus = false;
        //display.fillScreen(cc_blk);
        display.clearDisplay();
      }
      //Serial.println("WiFi not connected");

      /*
      display.setCursor(0, 0);
      display.setTextColor(cc_red);
      display.print("Failed to Connect");
      */

      scrollText("Failed to Connect WiFi.", 0, 12, 64, 8, 50, cc_grn);  // Call once per loop
      //scrollText("Failed to Connect WiFi", 20, 12, 30, 7, 50, cc_grn);  // Call once per loop
    }
  }

  /*
  //maybe update here but i use timer interrapt to update
  display_updater();
  //or
  display.showBuffer();  // Double Buffering to Reduce Flicker
  */
}









// #include <ESP8266WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>

// // WiFi credentials
// const char* ssid = "hidden network";
// const char* password = "ogokoigala";


// //AsyncWebServer server(80);
// WiFiClient client;

// const char* weatherserver = "api.open-meteo.com";

// // Coordinates (example: New York City)
// const char* LAT = "37.7510";
// const char* LON = "139.7222";

// // Weather data variables
// float temperature = -999;
// int humidity = -1;
// float wind_speed = -1;
// String wind_dir = "";
// int weatherCode = -1;
// bool isDay = false;

// void getWeather() {
//   String url = "/v1/forecast?latitude=" + String(LAT) +
//                "&longitude=" + String(LON) +
//                "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,is_day";

//   Serial.print("Connecting to ");
//   Serial.println(weatherserver);

//   if (!client.connect(weatherserver, 80)) {
//     Serial.println("Connection failed");
//     return;
//   }

//   // Send HTTP GET request
//   client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//                "Host: " + weatherserver + "\r\n" +
//                "Accept: application/json\r\n" +
//                "Connection: close\r\n\r\n");

//   // Wait for response headers
//   String response = client.readStringUntil('{');
//   response = "{" + client.readString();  // Add back first brace

//   // Close connection
//   client.stop();

//   // Parse JSON
//   StaticJsonDocument<2048> doc;
//   DeserializationError error = deserializeJson(doc, response);
//   if (error) {
//     Serial.print("JSON parse failed: ");
//     Serial.println(error.c_str());
//     return;
//   }

//   JsonObject current = doc["current"];
//   temperature = current["temperature_2m"];
//   humidity = current["relative_humidity_2m"];
//   wind_speed = current["wind_speed_10m"];
//   wind_dir = String(current["wind_direction_10m"].as<int>());
//   weatherCode = current["weather_code"];
//   isDay = current["is_day"];

//   Serial.println("=== Weather ===");
//   Serial.printf("Temp: %.1f°C\n", temperature);
//   Serial.printf("Humidity: %d%%\n", humidity);
//   Serial.printf("Wind Speed: %.1f m/s\n", wind_speed);
//   Serial.printf("Wind Dir: %s°\n", wind_dir.c_str());
//   Serial.printf("Weather Code: %d\n", weatherCode);
//   Serial.printf("Daytime: %s\n", isDay ? "Yes" : "No");
//   Serial.println("================");
// }

// void setup() {
//   Serial.begin(115200);
//   WiFi.begin(ssid, password);
//   Serial.println("Connecting to WiFi...");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nConnected.");

//   getWeather();

//   // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//   //   String msg = "Temp: " + String(temperature) + " C\n";
//   //   msg += "Humidity: " + String(humidity) + "%\n";
//   //   msg += "Wind: " + String(wind_speed) + " m/s " + wind_dir + "°\n";
//   //   msg += "Weather Code: " + String(weatherCode) + "\n";
//   //   msg += "Day: " + String(isDay ? "Yes" : "No");
//   //   request->send(200, "text/plain", msg);
//   // });

//   //server.begin();
// }

// void loop() {
//   // Nothing needed here unless you want to refresh weather periodically

//   getWeather();

//   delay(5000);
// }