#ifndef WEATHER_H_
#define WEATHER_H_

#include <Arduino.h>

  bool updateWeather();

  bool fetchWttr();
  bool fetchOpenMeteo();
  bool fetchWeatherAPI();
  bool fetchOpenWeather();

  void printWeatherReport();

  uint8_t OpenMeteo_CodeToIcon(int weatherCode, uint8_t isDay);
  uint8_t wttr_TextToIcon(String desc, uint8_t isDay);
  uint8_t WeatherAPI_CodeToIcon(int code, uint8_t isDay);
  
  float getTemparature();
  int getHumidity();
  float getWindSpeed();
  String getWindDirection();
  uint8_t getWeatherIcon();
  bool getIsDay();
#endif