#ifndef BUZZER_H_
  #define BUZZER_H_
   #include <stdlib.h>

  #define LONG_BEEP 0
  #define SHORT_BEEP 1

  void initBuzzer();
  void manageBuzzer();   // must be called from loop()
  void beeper(uint8_t count, uint8_t pattern);

#endif