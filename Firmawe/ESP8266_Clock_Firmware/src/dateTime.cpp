#include <ESP8266WiFi.h>            // We need WiFi to get internet access
#include <time.h>                   // For time() ctime()

#include "dateTime.h"
#include "config.h"
#include "webServer.h"

uint32_t previousMillis = 0;  // Store last time the function ran
const uint16_t interval = 1000; // Update every 1 sec

char dateStr[50];   // "YYYY-MM-DD" or other formats
char timeStr[20];   // "HH:MM:SS AM/PM"
char weekStr[5];    // "Thu"
char ampmStr[3];    // "AM" or "PM"

int currentHour;    // Store hour as integer
int currentHour24;  // Store 24 hour as integer
int currentMinute;  // Store minute as integer
int currentSecond;  // Store second as integer

DateFormat dateFormat = DEFAULT_DATE_FORMAT;  // Default date format


void configDateTime() {
  configTime(getTimeZone(), NTP_SERVER);
  // By default, the NTP will be started after 60 secs
}


// Update date and time
void updateDateTime(bool is24HourFormat, DateFormat dateFormat) {
  uint32_t currentMillis = millis();

  if (isWifiConnected() && (currentMillis - previousMillis >= interval)) {
    previousMillis = currentMillis;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    static const char *wd[7] = {"SUN", "MON", "TUE", "WED", "THR", "FRI", "SAT"};
    static const char *months[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

    // Weekday format
    snprintf(weekStr, sizeof(weekStr) - 1, "%s", wd[tm->tm_wday]);

    // Format date based on selected enum value
    switch (dateFormat) {
      case MON_DD_YYYY:
        snprintf(dateStr, sizeof(dateStr) - 1, "%s%02d,%04d", months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
        break;
      case YYYY_MM_DD:
        snprintf(dateStr, sizeof(dateStr) - 1, "%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
        break;
      case DD_MM_YYYY:
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d-%02d-%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
        break;
      case MM_DD_YYYY:
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d-%02d-%04d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);
        break;
      case YYYY_SLASH_MM_SLASH_DD:
        snprintf(dateStr, sizeof(dateStr) - 1, "%04d/%02d/%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
        break;
      case DD_SLASH_MM_SLASH_YYYY:
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
        break;
      case MM_SLASH_DD_SLASH_YYYY:
        snprintf(dateStr, sizeof(dateStr) - 1, "%02d/%02d/%04d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900);
        break;
      default:
        snprintf(dateStr, sizeof(dateStr) - 1, "INVALID FORMAT");
    }

    // Format time
    int displayHour = tm->tm_hour;
    const char *amPm = "";

    if (!is24HourFormat) {
      amPm = (displayHour >= 12) ? "PM" : "AM";
      displayHour = (displayHour == 0) ? 12 : (displayHour > 12 ? displayHour - 12 : displayHour);
    }

    if (is24HourFormat) {
      snprintf(timeStr, sizeof(timeStr) - 1, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
      ampmStr[0] = '\0';
    } else {
      snprintf(timeStr, sizeof(timeStr) - 1, "%02d:%02d:%02d %s", displayHour, tm->tm_min, tm->tm_sec, amPm);
      strcpy(ampmStr, amPm);
    }

    currentHour = displayHour;
    currentHour24 = tm->tm_hour;
    currentMinute = tm->tm_min;
    currentSecond = tm->tm_sec;
  }
}


// Getter functions (returning Strings where necessary)
String getCurrentDate() { return String(dateStr); }
String getCurrentTime() { return String(timeStr); }
String getCurrentWeekDay() { return String(weekStr); }
int getCurrentHour() { return currentHour; }
int getCurrentHour24() {return currentHour24;}
int getCurrentMinute() { return currentMinute; }
int getCurrentSecond() { return currentSecond; }
String getCurrentAmPm() { return String(ampmStr); } // Only needed for 12-hour format



/*
updateDateTime(bool is24HourFormat) {
  uint32_t currentMillis = millis();  // Get current time

  if (isWifiConnected() && (currentMillis - previousMillis >= interval)) { // Update every 1 sec
    previousMillis = currentMillis;  // Update last execution time

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    static const char *wd[7] = {"SUN","MON","TUE","WED","THR","FRI","SAT"};

    // Store values in global variables
    snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    snprintf(weekStr, sizeof(weekStr), "%s", wd[tm->tm_wday]);

    // Determine hour format
    int displayHour = tm->tm_hour;
    const char *amPm = "";

    if (!is24HourFormat) { // Convert to 12-hour format
      amPm = (displayHour >= 12) ? "PM" : "AM";
      displayHour = (displayHour == 0) ? 12 : (displayHour > 12 ? displayHour - 12 : displayHour);
    }

    // Format time string based on 12h or 24h format
    if (is24HourFormat) {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
       ampmStr[0] = '\0'; // Empty if 24-hour format
    } else {
      snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d %s", displayHour, tm->tm_min, tm->tm_sec, amPm);
      strcpy(ampmStr, amPm); // Copy AM/PM to global variable
    }

    // Update global time variables
    currentHour = displayHour;
    currentMinute = tm->tm_min;
    currentSecond = tm->tm_sec;
  }
}
*/



/*
#define WIFI_SSID   "hidden network"                    // set your SSID
#define WIFI_PASSWD "ogokoigala"                        // set your wifi password



#define NTP_SERVER "jp.pool.ntp.org"           
#define TIME_ZONE  "JST-9" //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv


#include <ESP8266WiFi.h>            // we need wifi to get internet access
#include <time.h>                   // for time() ctime()


void showDateTime() {
    time_t now;
    struct tm *tm;
    static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

    now = time(NULL);
    tm = localtime(&now);
    Serial.printf("%04d/%02d/%02d(%s) %02d:%02d:%02d\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    delay(1000);
}

void setup() {
  Serial.begin(115200);
  
  // start network
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  Serial.printf("Connecting to %s ",WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print ( "." );
  }
  Serial.printf("\nWiFiConnected, IP address: ");
  Serial.println(WiFi.localIP());

  configTime(TIME_ZONE, NTP_SERVER);
  // by default, the NTP will be started after 60 secs
}

void loop() {
    showDateTime();
}


*/