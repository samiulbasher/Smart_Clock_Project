#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_
  #include "dateTime.h"

  void initWebServer();
  void loopWebServer();
  
  bool isWifiConnected();
  bool isWifiAP_Mode();

  const char* getTimeZone();
  bool getTimeFormat();
  DateFormat getDateFormat();
  bool getBeepOnHourChange();

  String getWeatherService();
  uint8_t getWeatherRefreshInterval();

  String getLocation();
  String getLatitude();
  String getLongitude();
  String getWeatherApiKey();
  String getOpenWeatherApiKey();

  String AP_Mode_IP();
  String Wifi_IP();

  int getColorPalet();
  int getBrightness();

  int getNightStartHour();
  int getNightStartMinute();
  int getNightEndHour();
  int getNightEndMinute();

  void updateWeatherStatus(float temperature, int humidity, const String &weatherCode, const String &description);

#endif