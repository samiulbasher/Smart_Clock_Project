
#define BAUDRATE 115200


// ESP8266 pins for P2.5 LED Matrix
#ifdef NodeMCU_LoLin_V3
  #define P_LAT 16
  #define P_A 5
  #define P_B 0
  #define P_C 15
  #define P_D 4
  #define P_OE 2
  #define BUZZER_PIN 12  // D6 pin
#endif

#ifdef WeMos_D1_MINI
  #define P_LAT 16
  #define P_A 5
  #define P_B 0
  #define P_C 15
  #define P_D 4
  #define P_OE 2
  #define BUZZER_PIN 12  // D6 pin
#endif


#define DEFAULT_SSID   "yourSSID"                     // set your SSID
#define DEFAULT_PASS   "yourPassword"                 // set your wifi password

#define HOSTNAME "SMART-CLOCK"  // for mDNS and OTA


/* Configuration of NTP */
#define NTP_SERVER "jp.pool.ntp.org"           
#define DEFAULT_TIME_ZONE  "JST-9" //https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define FORMAT_12H "N"
#define FORMAT_24H "Y"
#define DEFAULT_TIME_FORMAT FORMAT_12H  // 12 or 24 hour format
#define DEFAULT_DATE_FORMAT DD_MM_YYYY  // Use the enum value directly
#define DEFAULT_BEEP_ON_HOUR_CHANGE "Y" // "Y": Yes, "N": No

#define DEFAULT_COLOR 1 // 1:white, 2:Cyan, 3:Red, 4:Blue, 5:Yellow, 6:Bright Blue, 7:Orange, 8:Green

#define DAY_BRIGHTNESS 60   // Default: 60
#define DIM_BRIGHTNESS 20
#define NIGHT_BRIGHTNESS 30 // Default: 30

#define NIGHT_START "22:00"
#define NIGHT_END "06:00"

#define WEATHER_REFRESH_INTERVAL 15  // in minutes

// Select weather fetching site
//#define DEFAULT_WEATHER_SERVICE "wttr" //not a good weather service
#define DEFAULT_WEATHER_SERVICE "open-meteo"
//#define DEFAULT_WEATHER_SERVICE "weatherAPI"
//#define DEFAULT_WEATHER_SERVICE "openWeather"

#define DEFAULT_LOCATION  "yonezawa"  // Change this to your city

#define WEATHER_API_KEY "yourWeatherApikey" //"yourWeatherApikey"
#define OPEN_WEATHER_API_KEY "yourOpenWeatherApiKey" //"yourOpenWeatherApiKey"

// Coordinates for Yonezawa
#define DEFAULT_LATITUDE 37.916667
#define DEFAULT_LONGITUDE 140.116667

#define DISPLAY_WITH_ANIMATIONS true  // Set to true to enable weather animations