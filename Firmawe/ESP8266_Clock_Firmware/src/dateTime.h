#ifndef DATE_TIME_H_
#define DATE_TIME_H_

enum DateFormat {
    MON_DD_YYYY, // MAR20,2025
    YYYY_MM_DD,  // 2025-03-15
    DD_MM_YYYY,  // 15-03-2025
    MM_DD_YYYY,  // 03-15-2025
    YYYY_SLASH_MM_SLASH_DD,  // 2025/03/15
    DD_SLASH_MM_SLASH_YYYY,  // 15/03/2025
    MM_SLASH_DD_SLASH_YYYY   // 03/15/2025
};


void updateDateTime(bool is24HourFormat, DateFormat dateFormat);
void configDateTime();

String getCurrentDate();
String getCurrentTime();
String getCurrentWeekDay();
int getCurrentHour();
int getCurrentHour24();
int getCurrentMinute();
int getCurrentSecond();
String getCurrentAmPm();

#endif